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
#include "pocketdb/helpers/TypesHelper.hpp"
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
        static bool Index(const CBlock& block, int height)
        {
            auto result = true;

            result &= RollbackChain(height);
            result &= IndexChain(block, height);
            result &= IndexRatings(block, height);

            return result;
        }

        static bool Rollback(int height)
        {
            auto result = true;
            result &= RollbackChain(height);
            return result;
        }

    protected:
        // Delete all calculated records for this height
        static bool RollbackChain(int height)
        {
            return PocketDb::ChainRepoInst.RollbackBlock(height);
        }

        // Set block height for all transactions in block
        static bool IndexChain(const CBlock& block, int height)
        {
            // transaction with all inputs
            vector<TransactionIndexingInfo> txs;
            for (const auto& tx : block.vtx)
            {
                TransactionIndexingInfo txInfo;
                txInfo.Hash = tx->GetHash().GetHex();
                txInfo.Type = ParseType(tx);

                if (!tx->IsCoinBase())
                {
                    for (const auto& inp : tx->vin)
                        txInfo.Inputs.emplace(inp.prevout.hash.GetHex(), inp.prevout.n);
                }

                txs.push_back(txInfo);
            }

            return PocketDb::ChainRepoInst.UpdateHeight(block.GetHash().GetHex(), height, txs);
        }

        static bool IndexRatings(const CBlock& block, int height)
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
                if (txType != PocketTxType::ACTION_SCORE_POST &&
                    txType != PocketTxType::ACTION_SCORE_COMMENT)
                    continue;

                // Need select content id for saving rating
                auto[scoreDataResult, scoreData] = PocketDb::TransRepoInst.GetScoreData(tx->GetHash().GetHex());
                if (!scoreDataResult) continue;

                // Old posts denied change reputation
                if (!reputationConsensus->AllowModifyOldPosts(
                    scoreData->ScoreTime,
                    scoreData->ContentTime,
                    scoreData->ContentType))
                {
                    continue;
                }

                // Check whether the current rating has the right to change the recipient's reputation
                if (!reputationConsensus->AllowModifyReputation(
                    scoreData->ScoreType,
                    tx,
                    scoreData->ScoreAddressHash,
                    scoreData->ContentAddressHash,
                    height,
                    false))
                {
                    continue;
                }

                // Calculate ratings values
                // Rating for users over posts = equals -20 and 20 - saved in int 21 = 2.1
                // Rating for users over comments = equals -0.1 or 0.1
                // Rating for posts equals between -2 and 2
                // Rating for comments equals between -1 and 2 - as is
                switch (scoreData->ScoreType)
                {
                    case PocketTx::ACTION_SCORE_POST:
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

            // Prepare all ratings model records from source values
            shared_ptr<vector<Rating>> ratings = make_shared<vector<Rating>>();
            for (const auto& tp : ratingValues)
            {
                for (const auto& itm : tp.second)
                {
                    Rating rtg;
                    rtg.SetType(tp.first);
                    rtg.SetHeight(height);
                    rtg.SetId(itm.first);
                    rtg.SetValue(itm.second);

                    ratings->push_back(rtg);
                }
            }

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
                return true;

            // Save all ratings in one transaction
            return PocketDb::ChainRepoInst.InsertRatings(ratings);
        }


    private:

        void static ExtendAccountLikers(shared_ptr<ScoreDataDto> scoreData, map<int, vector<int>>& accountLikers)
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
