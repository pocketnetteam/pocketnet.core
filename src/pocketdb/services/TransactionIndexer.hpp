// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 Bitcoin developers
// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETDB_TRANSACTIONINDEXER_HPP
#define POCKETDB_TRANSACTIONINDEXER_HPP

#include "util.h"
#include "chain.h"
#include "primitives/block.h"

#include "pocketdb/consensus.h"
#include "pocketdb/helpers/TransactionHelper.hpp"
#include "pocketdb/pocketnet.h"

namespace PocketServices
{
    using namespace PocketTx;
    using namespace PocketDb;
    using namespace PocketHelpers;

    using std::make_tuple;
    using std::tuple;
    using std::vector;
    using std::find;

    class TransactionIndexer
    {
    public:
        static void Index(const CBlock& block, int height)
        {
            auto txs = PrepareTransactions(block);
            
            int64_t nTime1 = GetTimeMicros();

            IndexChain(block.GetHash().GetHex(), height, txs);

            int64_t nTime2 = GetTimeMicros();
            LogPrint(BCLog::BENCH, "    - IndexChain: %.2fms _ %d\n", 0.001 * (nTime2 - nTime1), height);

            IndexRatings(height, block);

            int64_t nTime3 = GetTimeMicros();
            LogPrint(BCLog::BENCH, "    - IndexRatings: %.2fms _ %d\n", 0.001 * (nTime3 - nTime2), height);
        }

        static bool Rollback(const CBlock& block, int height)
        {
            auto txs = PrepareTransactions(block);
            return PocketDb::ChainRepoInst.RollbackBlock(height, txs);
        }

    protected:

        static vector<TransactionIndexingInfo> PrepareTransactions(const CBlock& block)
        {
            vector<TransactionIndexingInfo> txs;
            for (int i = 0; i < (int)block.vtx.size(); i++)
            {
                auto& tx = block.vtx[i];

                if (ParseType(tx) == PocketTxType::NOT_SUPPORTED)
                    continue;

                TransactionIndexingInfo txInfo;
                txInfo.Hash = tx->GetHash().GetHex();
                txInfo.BlockNumber = i;
                txInfo.Type = ParseType(tx);

                if (!tx->IsCoinBase())
                {
                    for (const auto& inp : tx->vin)
                        txInfo.Inputs.emplace_back(inp.prevout.hash.GetHex(), inp.prevout.n);
                }

                txs.push_back(txInfo);
            }
        }

        // Set block height for all transactions in block
        static void IndexChain(const string& blockHash, int height, vector<TransactionIndexingInfo>& txs)
        {
            PocketDb::ChainRepoInst.IndexBlock(blockHash, height, txs);
        }

        static void IndexRatings(int height, const CBlock& block)
        {
            map<RatingType, map<int, int>> ratingValues;
            map<int, vector<int>> accountLikers;

            // Actual consensus checker instance by current height
            auto reputationConsensus = PocketConsensus::ReputationConsensusFactoryInst.Instance(height);

            // Loop all transactions for find scores and increase ratings for accounts and contents
            for (const auto& tx : block.vtx)
            {
                auto txType = PocketHelpers::ParseType(tx);

                // Only scores allowed in calculating ratings
                if (txType != PocketTxType::ACTION_SCORE_CONTENT &&
                    txType != PocketTxType::ACTION_SCORE_COMMENT)
                    continue;

                // Need select content id for saving rating
                auto scoreData = PocketDb::ConsensusRepoInst.GetScoreData(tx->GetHash().GetHex());
                if (!scoreData)
                    throw std::runtime_error(strprintf("%s: Failed get score data for tx: %s\n",
                        __func__, tx->GetHash().GetHex()));

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
                    tx,
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

                        ratingValues[RatingType::RATING_POST][scoreData->ContentId] +=
                            scoreData->ScoreValue - 3;

                        if (scoreData->ScoreValue == 4 || scoreData->ScoreValue == 5)
                            ExtendAccountLikers(scoreData, accountLikers);

                        break;

                    case PocketTx::ACTION_SCORE_COMMENT:
                        ratingValues[RatingType::RATING_ACCOUNT][scoreData->ContentAddressId] +=
                            scoreData->ScoreValue;

                        ratingValues[RatingType::RATING_COMMENT][scoreData->ContentId] +=
                            scoreData->ScoreValue;

                        if (scoreData->ScoreValue == 1)
                            ExtendAccountLikers(scoreData, accountLikers);

                        break;

                        // Not supported score type
                    default:
                        break;
                }
            }

            // Prepare all ratings model records for increase Rating
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

            // Prepare all ratings model records for Liker type
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


    private:

        void static ExtendAccountLikers(const shared_ptr<ScoreDataDto>& scoreData, map<int, vector<int>>& accountLikers)
        {
            auto found = find(
                accountLikers[scoreData->ContentAddressId].begin(),
                accountLikers[scoreData->ContentAddressId].end(),
                scoreData->ScoreAddressId
            );

            if (found == accountLikers[scoreData->ContentAddressId].end())
                accountLikers[scoreData->ContentAddressId].push_back(scoreData->ScoreAddressId);
        }

    };

} // namespace PocketServices

#endif // POCKETDB_TRANSACTIONINDEXER_HPP
