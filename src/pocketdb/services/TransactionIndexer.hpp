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

#include "pocketdb/pocketnet.h"
#include "pocketdb/consensus.h"
#include "pocketdb/helpers/TypesHelper.hpp"
#include "pocketdb/helpers/TransactionHelper.hpp"

namespace PocketServices
{
    using namespace PocketTx;
    using namespace PocketDb;
    using namespace PocketHelpers;

    using std::vector;
    using std::tuple;
    using std::make_tuple;

    class TransactionIndexer
    {
    public:
        static bool Index(const CBlock& block, int height)
        {
            auto result = true;

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
            // Type, Unique Id, Value
            map<RatingType, map<int, int>> values;

            // Actual consensus checker instance by current height
            auto reputationConsensus = PocketConsensus::ReputationConsensusFactoryInst.Instance(height);

            for (const auto& tx : block.vtx)
            {
                auto[parseResult, scoreData] = PocketHelpers::ParseScore(tx);
                if (!parseResult) continue;

                // Only scores allowed in calculating ratings
                if (scoreData->Type != PocketTxType::ACTION_SCORE_POST)
                    continue;
                if (scoreData->Type != PocketTxType::ACTION_SCORE_COMMENT)
                    continue;

                // Need select content id for saving rating
                auto[scoreIdsResult, scoreIds] = PocketDb::TransRepoInst.GetScoreIds(
                    scoreData->Hash,
                    scoreData->From,
                    scoreData->To
                );
                if (!scoreIdsResult) continue;

                // Check whether the current rating has the right to change the recipient's reputation
                auto allowModifyReputation = reputationConsensus->AllowModifyReputation(
                    scoreData->Type,
                    tx,
                    scoreData->From,
                    scoreData->To,
                    height,
                    false
                );

                if (allowModifyReputation)
                {
                    // TODO (brangr): implement

                    // posts

                    // Scores to old posts not modify reputation
                    bool modify_block_old_post = (tx->nTime - postItm["time"].As<int64_t>()) < GetActualLimit(Limit::scores_depth_modify_reputation, pindex->nHeight - 1);

                    // USER & POST reputation
                    if (modify_by_user_reputation && modify_block_old_post) {

                        // User reputation
                        if (userReputations.find(post_address) == userReputations.end()) userReputations.insert(std::make_pair(post_address, 0));
                        userReputations[post_address] += (scoreVal - 3) * 10; // Reputation between -20 and 20 - user reputation saved in int 21 = 2.1

                        // Post reputation
                        if (postReputations.find(posttxid) == postReputations.end()) postReputations.insert(std::make_pair(posttxid, 0));
                        postReputations[posttxid] += scoreVal - 3; // Reputation between -2 and 2

                        // Save distinct liker for user
                        if (scoreVal == 4 or scoreVal == 5) {
                            if (userLikers.find(post_address) == userLikers.end()) {
                                std::vector<std::string> likers;
                                userLikers.insert(std::make_pair(post_address, likers));
                            }

                            if (std::find(userLikers[post_address].begin(), userLikers[post_address].end(), score_address) == userLikers[post_address].end())
                                userLikers[post_address].push_back(score_address);
                        }
                    }

                    // ---------------------------------------------

                    // comment
                    // User reputation
                    if (userReputations.find(comment_address) == userReputations.end()) userReputations.insert(std::make_pair(comment_address, 0));
                    userReputations[comment_address] += scoreVal; // Reputation equals -0.1 or 0.1

                    // Comment reputation
                    if (commentReputations.find(commentid) == commentReputations.end()) commentReputations.insert(std::make_pair(commentid, 0));
                    commentReputations[commentid] += scoreVal;

                    // Save distinct liker for user
                    if (scoreVal == 1) {
                        if (userLikers.find(comment_address) == userLikers.end()) {
                            std::vector<std::string> likers;
                            userLikers.insert(std::make_pair(comment_address, likers));
                        }

                        if (std::find(userLikers[comment_address].begin(), userLikers[comment_address].end(), score_address) == userLikers[comment_address].end())
                            userLikers[comment_address].push_back(score_address);
                    }
                }
            }
        }


    private:

    };

} // namespace PocketServices

#endif // POCKETDB_TRANSACTIONINDEXER_HPP
