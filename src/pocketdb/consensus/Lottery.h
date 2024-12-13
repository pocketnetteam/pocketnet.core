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
        vector<string> ModerationVoteWinners;
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

        virtual bool FilterScore(const ScoreDataDtoRef& scoreData)
        {
            if (scoreData->ScoreType == ACTION_SCORE_COMMENT && scoreData->ScoreValue == 1)
                return true;

            if (scoreData->ScoreType == ACTION_SCORE_CONTENT && (scoreData->ScoreValue == 4 || scoreData->ScoreValue == 5))
                return true;

            return false;
        }

        virtual void ExtendCandidates(
            const ScoreDataDtoRef& scoreData,
            map<string, int>& postCandidates,
            map<string, string>& postReferrersCandidates,
            map<string, int>& commentCandidates,
            map<string, string>& commentReferrersCandidates)
        {
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

    public:
        LotteryConsensus() : BaseConsensus()
        {
            Limits.Set("max_winners_counts", 25, 25, 25);
        }

        // Get all lottery winner
        virtual LotteryWinners Winners(const CBlock& block, CDataStream& hashProofOfStakeSource)
        {
            auto reputationConsensus = PocketConsensus::ConsensusFactoryInst_Reputation.Instance(Height);

            auto scoresData = ConsensusRepoInst.GetScoresData(
                Height,
                reputationConsensus->GetConsensusLimit(ConsensusLimit_scores_one_to_one_depth)
            );

            vector<string> accountsAddresses;
            for (auto& scoreData : scoresData)
                accountsAddresses.push_back(reputationConsensus->SelectAddressScoreContent(scoreData.second, true));
            auto accountsData = ConsensusRepoInst.GetAccountsData(accountsAddresses);

            LotteryWinners _winners;

            map<string, int> postCandidates;
            map<string, string> postReferrersCandidates;

            map<string, int> commentCandidates;
            map<string, string> commentReferrersCandidates;

            for (const auto& tx : block.vtx)
            {
                // Parse data for moderation votes
                auto[parseModerationVoteOk, moderationVoteTxData] = TransactionHelper::ParseModerationVote(tx);
                if (parseModerationVoteOk)
                {
                    _winners.ModerationVoteWinners.push_back(moderationVoteTxData->AddressHash);
                    continue;
                }

                // Get destination address and score value
                // In lottery allowed only likes to posts and comments
                // Also in lottery allowed only positive scores
                auto[parseScoreOk, scoreTxData] = TransactionHelper::ParseScore(tx);
                if (!parseScoreOk)
                    continue;

                if (!FilterScore(scoreTxData))
                    continue;

                auto& scoreData = scoresData[scoreTxData->ScoreTxHash];
                auto& accountData = accountsData[reputationConsensus->SelectAddressScoreContent(scoreData, true)];

                if (!reputationConsensus->AllowModifyReputation(scoreData, accountData, true))
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
            }

            return _winners;
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
            // 5 - 100%; 2.0 - nodes; 3.0 - all for lottery;
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

            auto regTime = PocketDb::ConsensusRepoInst.GetAccountRegistrationTime(scoreData->ContentAddressHash);
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
            // 5 - 100%; 4.75 - nodes; 0.25 - all for lottery;
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
            // 5 - 100%; 4.875 - nodes; 0.125 - all for lottery;
            if (code == OP_WINNER_POST) return nCredit * 0.002;
            if (code == OP_WINNER_POST_REFERRAL) return nCredit * 0.002;
            if (code == OP_WINNER_COMMENT) return nCredit * 0.0005;
            if (code == OP_WINNER_COMMENT_REFERRAL) return nCredit * 0.0005;
            return 0;
        }
    };


    class LotteryConsensus_pip_100 : public LotteryConsensus_checkpoint_1180000
    {
    public:
        LotteryConsensus_pip_100() : LotteryConsensus_checkpoint_1180000()
        {
            Limits.Set("max_winners_counts", 5, 5, 5);
        }
            
        // ----------------------------------------
        // Lottery checkpoint at 2162400 block
        // Disable lottery payments for likes to comments.
        // Also disable referral program
        // 5 - 100%; 4.875 - nodes; 0.125 (2.5%) - all for lottery;
        CAmount RatingReward(CAmount nCredit, opcodetype code) override
        {
            if (code == OP_WINNER_POST) return nCredit * 0.025;
            return 0;
        }

    protected:
        void ExtendReferrer(const ScoreDataDtoRef& scoreData, map<string, string>& refs) override { }

        bool FilterScore(const ScoreDataDtoRef& scoreData) override
        {
            if (scoreData->ScoreType == ACTION_SCORE_CONTENT && (scoreData->ScoreValue == 4 || scoreData->ScoreValue == 5))
                return true;

            return false;
        }
    };

    class LotteryConsensus_pip_110 : public LotteryConsensus_checkpoint_1180000
    {
    public:
        LotteryConsensus_pip_110() : LotteryConsensus_checkpoint_1180000()
        {
            Limits.Set("max_winners_counts", 5, 5, 5);
        }

        // 5 - 100%; 4.375 (87.5%) - nodes; 0.125 (2.5%) - for posts; 0.5 (10%) - for moderation votes;
        CAmount RatingReward(CAmount nCredit, opcodetype code) override
        {
            if (code == OP_WINNER_POST) return nCredit * 0.025;
            if (code == OP_WINNER_MODERATION_VOTE) return nCredit * 0.1;
            return 0;
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
            Checkpoint({ 1180000,       0, -1, make_shared<LotteryConsensus_checkpoint_1180000>() });
            Checkpoint({ 2162400, 1650652, -1, make_shared<LotteryConsensus_pip_100>() });
            Checkpoint({ 9999999, 3500000,  0, make_shared<LotteryConsensus_pip_110>() });
        }
    };

    static LotteryConsensusFactory ConsensusFactoryInst_Lottery;
}

#endif // POCKETCONSENSUS_LOTTERY_H
