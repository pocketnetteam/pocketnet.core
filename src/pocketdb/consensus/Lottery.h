// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETCONSENSUS_LOTTERY_H
#define POCKETCONSENSUS_LOTTERY_H

#include "streams.h"
#include <boost/algorithm/string.hpp>
#include "pocketdb/consensus/Base.h"
#include "pocketdb/consensus/Reputation.h"
#include "pocketdb/helpers/TransactionHelper.h"

namespace PocketConsensus
{
    using namespace std;
    using namespace PocketTx;
    using namespace PocketHelpers;

    struct LotteryWinners
    {
        vector<string> PostWinners;
        vector<string> PostReferrerWinners;
        vector<string> CommentWinners;
        vector<string> CommentReferrerWinners;
    };

    // ---------------------------------------
    // Lottery base class
    class LotteryConsensus : public BaseConsensus
    {
    protected:
        void SortWinners(map<string, int>& candidates, CDataStream& hashProofOfStakeSource, vector<string>& winners)
        {
            vector<pair<string, pair<int, arith_uint256>>> candidatesSorted;
            for (auto& it: candidates)
            {
                CDataStream ss(hashProofOfStakeSource);
                ss << it.first;
                arith_uint256 hashSortRating = UintToArith256(Hash(ss)) / it.second;
                candidatesSorted.emplace_back(std::make_pair(it.first, std::make_pair(it.second, hashSortRating)));
            }

            if (!candidatesSorted.empty())
            {
                std::sort(candidatesSorted.begin(), candidatesSorted.end(), [](auto& a, auto& b)
                {
                    return a.second.second < b.second.second;
                });

                if (candidatesSorted.size() > Limits.Get("max_winners_counts"))
                    candidatesSorted.resize(Limits.Get("max_winners_counts"));

                for (auto& it : candidatesSorted)
                    winners.push_back(it.first);
            }
        }

        virtual void ExtendReferrer(const ScoreDataDtoRef& scoreData, map<string, string>& refs) {}
        virtual void ExtendReferrers() {}

    public:
        LotteryConsensus() : BaseConsensus()
        {
            Limits.Set("max_winners_counts", 25, 25, 25);
        }

        // Get all lottery winner
        virtual LotteryWinners Winners(const CBlock& block, CDataStream& hashProofOfStakeSource)
        {
            auto reputationConsensus = PocketConsensus::ConsensusFactoryInst_Reputation.Instance(Height);

            LotteryWinners _winners;

            map<string, int> postCandidates;
            map <string, string> postReferrersCandidates;

            map<string, int> commentCandidates;
            map <string, string> commentReferrersCandidates;

            for (const auto& tx : block.vtx)
            {
                // Get destination address and score value
                // In lottery allowed only likes to posts and comments
                // Also in lottery allowed only positive scores
                auto[parseScoreOk, scoreTxData] = TransactionHelper::ParseScore(tx);
                if (!parseScoreOk)
                    continue;

                if (scoreTxData->ScoreType == ACTION_SCORE_COMMENT
                    && scoreTxData->ScoreValue != 1)
                    continue;

                if (scoreTxData->ScoreType == ACTION_SCORE_CONTENT
                    && scoreTxData->ScoreValue != 4 && scoreTxData->ScoreValue != 5)
                    continue;

                auto scoreData = PocketDb::ConsensusRepoInst.GetScoreData(tx->GetHash().GetHex());
                if (!scoreData)
                {
                    LogPrintf("%s: Failed get score data for tx: %s\n", __func__, tx->GetHash().GetHex());
                    continue;
                }

                if (!reputationConsensus->AllowModifyReputation(
                    scoreData,
                    true
                ))
                    continue;

                if (scoreData->ScoreType == ACTION_SCORE_CONTENT)
                {
                    postCandidates[scoreData->ContentAddressHash] += (scoreData->ScoreValue - 3);
                    ExtendReferrer(scoreData, postReferrersCandidates);
                }

                if (scoreData->ScoreType == ACTION_SCORE_COMMENT)
                {
                    commentCandidates[scoreData->ContentAddressHash] += scoreData->ScoreValue;
                    ExtendReferrer(scoreData, commentReferrersCandidates);
                }
            }

            // Sort founded users
            SortWinners(postCandidates, hashProofOfStakeSource, _winners.PostWinners);
            SortWinners(commentCandidates, hashProofOfStakeSource, _winners.CommentWinners);

            // Extend referrers
            {
                if (!postReferrersCandidates.empty())
                    for (auto& winner : _winners.PostWinners)
                        if (postReferrersCandidates.find(winner) != postReferrersCandidates.end())
                            _winners.PostReferrerWinners.push_back(postReferrersCandidates[winner]);

                if (!commentReferrersCandidates.empty())
                    for (auto& winner : _winners.CommentWinners)
                        if (commentReferrersCandidates.find(winner) != commentReferrersCandidates.end())
                            _winners.CommentReferrerWinners.push_back(commentReferrersCandidates[winner]);

                // Find referrers for final winners list
                // This function replaced function ExtendReferrer with wrong logic (Time -> Height)
                ExtendReferrers();
            }

            return move(_winners);
        }

        virtual CAmount RatingReward(CAmount nCredit, opcodetype code)
        {
            if (code == OP_WINNER_COMMENT) return nCredit * 0.5 / 10;
            return nCredit * 0.5;
        }
        
        virtual void ExtendWinnerTypes(opcodetype type, std::vector<opcodetype>& winner_types) {}
    };

    // ---------------------------------------
    // Lottery checkpoint at 514185 block
    class LotteryConsensus_checkpoint_514185 : public LotteryConsensus
    {
    public:
        LotteryConsensus_checkpoint_514185() : LotteryConsensus() {}

        void ExtendWinnerTypes(opcodetype type, std::vector<opcodetype>& winner_types) override
        {
            winner_types.push_back(type);
        }

        CAmount RatingReward(CAmount nCredit, opcodetype code) override
        {
            // Referrer program 5 - 100%; 2.0 - nodes; 3.0 - all for lottery;
            // 2.0 - posts; 0.4 - referrer over posts (20%); 0.5 - comment; 0.1 - referrer over comment (20%);
            if (code == OP_WINNER_POST) return nCredit * 0.40;
            if (code == OP_WINNER_POST_REFERRAL) return nCredit * 0.08;
            if (code == OP_WINNER_COMMENT) return nCredit * 0.10;
            if (code == OP_WINNER_COMMENT_REFERRAL) return nCredit * 0.02;
            return 0;
        }

    protected:
        void ExtendReferrer(const ScoreDataDtoRef& scoreData, map<string, string>& refs) override
        {
            if (refs.find(scoreData->ContentAddressHash) != refs.end())
                return;

            auto[ok, referrer] = PocketDb::ConsensusRepoInst.GetReferrer(scoreData->ContentAddressHash);
            if (!ok || referrer == scoreData->ScoreAddressHash) return;

            refs.emplace(scoreData->ContentAddressHash, referrer);
        }
    };

    // ---------------------------------------
    // Lottery checkpoint at 1035000 block
    class LotteryConsensus_checkpoint_1035000 : public LotteryConsensus_checkpoint_514185
    {
    public:
        LotteryConsensus_checkpoint_1035000() : LotteryConsensus_checkpoint_514185() {}

    protected:
        void ExtendReferrer(const ScoreDataDtoRef& scoreData, map<string, string>& refs) override
        {
            if (refs.find(scoreData->ContentAddressHash) != refs.end())
                return;

            auto regTime = PocketDb::ConsensusRepoInst.GetAccountRegistrationTime(scoreData->ContentAddressId);
            if (regTime < (scoreData->ScoreTime - GetConsensusLimit(ConsensusLimit_lottery_referral_depth))) return;

            auto[ok, referrer] = PocketDb::ConsensusRepoInst.GetReferrer(scoreData->ContentAddressHash);
            if (!ok || referrer == scoreData->ScoreAddressHash) return;

            refs.emplace(scoreData->ContentAddressHash, referrer);
        }
    };

    // ---------------------------------------
    // Lottery checkpoint at 1124000 block
    class LotteryConsensus_checkpoint_1124000 : public LotteryConsensus_checkpoint_1035000
    {
    public:
        LotteryConsensus_checkpoint_1124000() : LotteryConsensus_checkpoint_1035000() {}

        CAmount RatingReward(CAmount nCredit, opcodetype code) override
        {
            // Referrer program 5 - 100%; 4.75 - nodes; 0.25 - all for lottery;
            // .1 - posts (2%); .1 - referrer over posts (2%); 0.025 - comment (.5%); 0.025 - referrer over comment (.5%);
            if (code == OP_WINNER_POST) return nCredit * 0.02;
            if (code == OP_WINNER_POST_REFERRAL) return nCredit * 0.02;
            if (code == OP_WINNER_COMMENT) return nCredit * 0.005;
            if (code == OP_WINNER_COMMENT_REFERRAL) return nCredit * 0.005;
            return 0;
        }
    };

    // ---------------------------------------
    // Lottery checkpoint at 1180000 block
    class LotteryConsensus_checkpoint_1180000 : public LotteryConsensus_checkpoint_1124000
    {
    public:
        LotteryConsensus_checkpoint_1180000() : LotteryConsensus_checkpoint_1124000() {}
        CAmount RatingReward(CAmount nCredit, opcodetype code) override
        {
            // Reduce all winnings by 10 times
            // Referrer program 5 - 100%; 4.975 - nodes; 0.025 - all for lottery;
            if (code == OP_WINNER_POST) return nCredit * 0.002;
            if (code == OP_WINNER_POST_REFERRAL) return nCredit * 0.002;
            if (code == OP_WINNER_COMMENT) return nCredit * 0.0005;
            if (code == OP_WINNER_COMMENT_REFERRAL) return nCredit * 0.0005;
            return 0;
        }
    };

    // ---------------------------------------
    class LotteryConsensus_bip_100 : public LotteryConsensus_checkpoint_1180000
    {
    public:
        LotteryConsensus_bip_100() : LotteryConsensus_checkpoint_1180000()
        {
            Limits.Set("max_winners_counts", 5, 5, 5);
        }
            
        // ----------------------------------------
        // Lottery checkpoint at 2162400 block
        // Disable lottery payments for likes to comments.
        // Also disable referral program
        CAmount RatingReward(CAmount nCredit, opcodetype code)
        {
            if (code == OP_WINNER_POST) return nCredit * 0.025;
            return 0;
        }

        LotteryWinners Winners(const CBlock& block, CDataStream& hashProofOfStakeSource)
        {
            auto reputationConsensus = PocketConsensus::ConsensusFactoryInst_Reputation.Instance(Height);

            LotteryWinners _winners;
            map<string, int> postCandidates;

            for (const auto& tx : block.vtx)
            {
                // Get destination address and score value
                // In lottery allowed only likes to posts and comments
                // Also in lottery allowed only positive scores
                auto[parseScoreOk, scoreTxData] = TransactionHelper::ParseScore(tx);
                if (!parseScoreOk)
                    continue;

                // BIP100: Only scores to content allowed
                if (scoreTxData->ScoreType == ACTION_SCORE_COMMENT)
                    continue;
                if (scoreTxData->ScoreType == ACTION_SCORE_CONTENT
                    && scoreTxData->ScoreValue != 4 && scoreTxData->ScoreValue != 5)
                    continue;

                auto scoreData = PocketDb::ConsensusRepoInst.GetScoreData(tx->GetHash().GetHex());
                if (!scoreData)
                {
                    LogPrintf("%s: Failed get score data for tx: %s\n", __func__, tx->GetHash().GetHex());
                    continue;
                }

                if (!reputationConsensus->AllowModifyReputation(
                    scoreData,
                    true
                ))
                    continue;

                postCandidates[scoreData->ContentAddressHash] += (scoreData->ScoreValue - 3);
            }

            // Sort founded users
            SortWinners(postCandidates, hashProofOfStakeSource, _winners.PostWinners);

            return move(_winners);
        }
    };

    //  Factory for select actual rules version
    class LotteryConsensusFactory : public BaseConsensusFactory<LotteryConsensus>
    {
    public:
        LotteryConsensusFactory()
        {
            Checkpoint({       0,      -1, -1, make_shared<LotteryConsensus>() });
            Checkpoint({  514185,      -1, -1, make_shared<LotteryConsensus_checkpoint_514185>() });
            Checkpoint({ 1035000,      -1, -1, make_shared<LotteryConsensus_checkpoint_1035000>() });
            Checkpoint({ 1124000,      -1, -1, make_shared<LotteryConsensus_checkpoint_1124000>() });
            Checkpoint({ 1180000,       0,  0, make_shared<LotteryConsensus_checkpoint_1180000>() });
            Checkpoint({ 2162400, 1650652,  0, make_shared<LotteryConsensus_bip_100>() });
        }
    };

    static LotteryConsensusFactory ConsensusFactoryInst_Lottery;
}

#endif // POCKETCONSENSUS_LOTTERY_H
