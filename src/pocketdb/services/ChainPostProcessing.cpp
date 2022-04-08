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
        map <RatingType, map<int, int>> ratingValues;
        map<int, vector<int>> accountLikersSrc;

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
            // Rating for users over posts = equals -20 and 20 - saved in int 21 = 2.1
            // Rating for users over comments = equals -0.1 or 0.1
            // Rating for posts equals between -2 and 2
            // Rating for comments equals between -1 and 2 - as is
            switch (scoreData->ScoreType)
            {
                case PocketTx::ACTION_SCORE_CONTENT:
                    ratingValues[RatingType::RATING_ACCOUNT][scoreData->ContentAddressId] +=
                        (scoreData->ScoreValue - 3) * 10;

                    ratingValues[RatingType::RATING_CONTENT][scoreData->ContentId] +=
                        scoreData->ScoreValue - 3;

                    if (scoreData->ScoreValue == 4 || scoreData->ScoreValue == 5)
                        BuildAccountLikers(scoreData, accountLikersSrc);

                    break;

                case PocketTx::ACTION_SCORE_COMMENT:
                    ratingValues[RatingType::RATING_ACCOUNT][scoreData->ContentAddressId] +=
                        scoreData->ScoreValue;

                    ratingValues[RatingType::RATING_COMMENT][scoreData->ContentId] +=
                        scoreData->ScoreValue;

                    if (scoreData->ScoreValue == 1)
                        BuildAccountLikers(scoreData, accountLikersSrc);

                    break;

                    // Not supported score type
                default:
                    break;
            }
        }

        // Prepare all ratings model records for increase Rating
        shared_ptr <vector<Rating>> ratings = make_shared < vector < Rating >> ();
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

        // Prepare all ratings model records for Liker type
        map<int, vector<int>> accountLikers;
        reputationConsensus->PrepareAccountLikers(accountLikersSrc, accountLikers);

        // Save likers in db
        for (const auto& acc : accountLikers)
        {
            for (const auto& lkrId : acc.second)
            {
                Rating rtg;
                rtg.SetType(RatingType::RATING_ACCOUNT_LIKERS);
                rtg.SetHeight(height);
                rtg.SetId(acc.first);
                rtg.SetValue(lkrId);

                ratings->push_back(rtg);
            }
        }

        if (ratings->empty())
            return;

        // Save all ratings in one transaction
        PocketDb::RatingsRepoInst.InsertRatings(ratings);
    }

    void ChainPostProcessing::BuildAccountLikers(const shared_ptr <ScoreDataDto>& scoreData, map<int, vector<int>>& accountLikers)
    {
        auto found = find(
            accountLikers[scoreData->ContentAddressId].begin(),
            accountLikers[scoreData->ContentAddressId].end(),
            scoreData->ScoreAddressId
        );

        if (found == accountLikers[scoreData->ContentAddressId].end())
            accountLikers[scoreData->ContentAddressId].push_back(scoreData->ScoreAddressId);
    }
    
} // namespace PocketServices