// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include "pocketdb/services/ChainPostProcessing.h"

namespace PocketServices
{
    void ChainPostProcessing::Index(const CBlock& block, int height)
    {
        vector<TransactionIndexingInfo> txs;
        PrepareTransactions(block, txs);

        int64_t nTime1 = GetTimeMicros();

        IndexChain(block.GetHash().GetHex(), height, txs);

        int64_t nTime2 = GetTimeMicros();
        LogPrint(BCLog::BENCH, "    - IndexChain: %.2fms _ %d\n", 0.001 * (double)(nTime2 - nTime1), height);

        IndexRatings(height, txs);

        int64_t nTime3 = GetTimeMicros();
        LogPrint(BCLog::BENCH, "    - IndexRatings: %.2fms _ %d\n", 0.001 * (double)(nTime3 - nTime2), height);
    }

    bool ChainPostProcessing::Rollback(int height)
    {
        LogPrint(BCLog::SYNC, "Rollback current block to prev at height %d\n", height - 1);
        return PocketDb::ChainRepoInst.Rollback(height);
    }

    void ChainPostProcessing::PrepareTransactions(const CBlock& block, vector<TransactionIndexingInfo>& txs)
    {
        for (size_t i = 0; i < block.vtx.size(); i++)
        {
            auto& tx = block.vtx[i];
            auto txType = PocketHelpers::TransactionHelper::ParseType(tx);

                TransactionIndexingInfo txInfo;
                txInfo.Hash = tx->GetHash().GetHex();
                txInfo.BlockNumber = (int) i;
                txInfo.Time = tx->nTime;
                txInfo.Type = txType;

                if (!tx->IsCoinBase())
                {
                    for (const auto& inp : tx->vin)
                        txInfo.Inputs.emplace_back(inp.prevout.hash.GetHex(), inp.prevout.n);
                }

                txs.emplace_back(txInfo);
            }
        }

    // Set block height for all transactions in block
    void ChainPostProcessing::IndexChain(const string& blockHash, int height, vector<TransactionIndexingInfo>& txs)
    {
        PocketDb::ChainRepoInst.IndexBlock(blockHash, height, txs);
    }

    void ChainPostProcessing::IndexRatings(int height, vector<TransactionIndexingInfo>& txs)
    {
        map<RatingType, map<int, int>> ratingValues;
        vector<ScoreDataDtoRef> distinctScores;
        map<RatingType, map<int, vector<int>>> likersValues;
        // map<int, vector<int>> accountLikersSrc;

        // Actual consensus checker instance by current height
        auto reputationConsensus = PocketConsensus::ReputationConsensusFactoryInst.Instance(height);

        // Loop all transactions for find scores and increase ratings for accounts and contents
        for (const auto& txInfo : txs)
        {
            // Only scores allowed in calculating ratings
            if (!txInfo.IsActionScore())
                continue;

            // Need select content id for saving rating
            auto scoreData = PocketDb::ConsensusRepoInst.GetScoreData(txInfo.Hash);
            if (!scoreData)
                throw std::runtime_error(strprintf("%s: Failed get score data for tx: %s\n", __func__, txInfo.Hash));

            // Old posts denied change reputation
            auto allowModifyOldPosts = reputationConsensus->AllowModifyOldPosts(
                scoreData->ScoreTime,
                scoreData->ContentTime,
                scoreData->ContentType);

            if (!allowModifyOldPosts)
                continue;

            // Check whether the current rating has the right to change the recipient's reputation
            auto allowModifyReputation = reputationConsensus->AllowModifyReputation(
                scoreData,
                false);

            if (!allowModifyReputation)
                continue;

            // Calculate ratings values
            // Rating for users over posts = equals -0.2 and 20 - saved in int 20 = 2.0
            // Rating for users over comments = equals -0.1 or 0.1
            // Rating for posts equals between -2 and 2 - as is
            // Rating for comments equals between -1 and 2 - as is
            switch (scoreData->ScoreType)
            {
                case PocketTx::ACTION_SCORE_CONTENT:
                    ratingValues[RatingType::RATING_ACCOUNT][scoreData->ContentAddressId] +=
                        reputationConsensus->GetScoreContentAuthorValue(scoreData->ScoreValue);

                    ratingValues[RatingType::RATING_CONTENT][scoreData->ContentId] +=
                        scoreData->ScoreValue - 3;

                    break;

                case PocketTx::ACTION_SCORE_COMMENT:
                    ratingValues[RatingType::RATING_ACCOUNT][scoreData->ContentAddressId] +=
                        reputationConsensus->GetScoreCommentAuthorValue(scoreData->ScoreValue);

                    ratingValues[RatingType::RATING_COMMENT][scoreData->ContentId] +=
                        scoreData->ScoreValue;

                    break;

                // Not supported score type
                default:
                    break;
            }
            
            // Extend list of ratings with likers values
            reputationConsensus->DistinctScores(scoreData, distinctScores);
        }

        // Filter all distinct records
        for (const auto& _scoreData : distinctScores)
        {
            reputationConsensus->ValidateAccountLiker(_scoreData, likersValues, ratingValues);
        }
            
        // Prepare all ratings model records for increase ratings
        shared_ptr<vector<Rating>> ratings = make_shared<vector<Rating>>();
        for (const auto& tp : ratingValues)
        {
            for (const auto& itm : tp.second)
            {
                // Skip not changed reputations - e.g. +2 and -2
                if (itm.second == 0)
                    continue;

                Rating rtg;
                rtg.SetType(tp.first);
                rtg.SetHeight(height);
                rtg.SetId(itm.first);
                rtg.SetValue(itm.second);

                ratings->push_back(rtg);
            }
        }

        // Prepare likers models
        for (const auto& tp : likersValues)
        {
            for (const auto& cnt : tp.second)
            {
                for (const auto& lkr : cnt.second)
                {
                    // Skip not changed likers count
                    if (lkr == 0 && (
                        tp.first == RatingType::ACCOUNT_LIKERS_POST_LAST ||
                        tp.first == RatingType::ACCOUNT_LIKERS_COMMENT_ROOT_LAST ||
                        tp.first == RatingType::ACCOUNT_LIKERS_COMMENT_ANSWER_LAST ||
                        tp.first == RatingType::ACCOUNT_DISLIKERS_COMMENT_ANSWER_LAST
                    ))
                        continue;
                        
                    Rating rtg;
                    rtg.SetType(tp.first);
                    rtg.SetHeight(height);
                    rtg.SetId(cnt.first);
                    rtg.SetValue(lkr);

                    ratings->push_back(rtg);
                }
            }
        }

        if (ratings->empty())
            return;

        // Save all ratings in one transaction
        PocketDb::RatingsRepoInst.InsertRatings(ratings);
    }

} // namespace PocketServices