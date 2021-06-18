// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 Bitcoin developers
// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETCONSENSUS_LOTTERY_HPP
#define POCKETCONSENSUS_LOTTERY_HPP

#include "streams.h"
#include <boost/algorithm/string.hpp>

#include "pocketdb/consensus/Base.hpp"
#include "pocketdb/consensus/Reputation.hpp"
#include "pocketdb/helpers/TransactionHelper.hpp"

namespace PocketConsensus
{
    using std::map;
    using std::vector;
    using std::string;
    using std::pair;
    using std::find;

    struct LotteryWinners
    {
        vector<string> PostWinners;
        vector<string> PostReferrerWinners;
        vector<string> CommentWinners;
        vector<string> CommentReferrerWinners;
    };

    /*******************************************************************************************************************
    *
    *  Lottery base class
    *
    *******************************************************************************************************************/
    class LotteryConsensus : public BaseConsensus
    {
    protected:
        LotteryWinners _winners;
        virtual int MaxWinnersCount() const { return 25; }

        void SortWinners(map<string, int>& candidates, CDataStream& hashProofOfStakeSource, vector<string>& winners)
        {
            vector<pair<string, pair<int, arith_uint256>>> candidatesSorted;
            for (auto& it: candidates)
            {
                CDataStream ss(hashProofOfStakeSource);
                ss << it.first;
                arith_uint256 hashSortRating = UintToArith256(Hash(ss.begin(), ss.end())) / it.second;
                candidatesSorted.push_back(std::make_pair(it.first, std::make_pair(it.second, hashSortRating)));
            }

            if (!candidatesSorted.empty())
            {
                std::sort(candidatesSorted.begin(), candidatesSorted.end(), [](auto& a, auto& b)
                {
                    return a.second.second < b.second.second;
                });

                if ((int) candidatesSorted.size() > MaxWinnersCount())
                    candidatesSorted.resize(MaxWinnersCount());

                for (auto& it : candidatesSorted)
                    winners.push_back(it.first);
            }
        }
        virtual void ExtendReferrer(const string& contentAddress, int64_t txTime, map<string, string>& refs) {}
        virtual void ExtendReferrers() {}

    public:
        LotteryConsensus(int height) : BaseConsensus(height) {}

        // Get all lottery winner
        virtual LotteryWinners& Winners(const CBlock& block, CDataStream& hashProofOfStakeSource,
            shared_ptr <ReputationConsensus> reputationConsensus)
        {
            map<string, int> postCandidates;
            map<string, string> postReferrersCandidates;

            map<string, int> commentCandidates;
            map<string, string> commentReferrersCandidates;

            for (const auto& tx : block.vtx)
            {
                // Get destination address and score value
                // In lottery allowed only likes to posts and comments
                // Also in lottery allowed only positive scores
                auto txType = PocketHelpers::ParseType(tx);
                if (txType != PocketTxType::ACTION_SCORE_POST &&
                    txType != PocketTxType::ACTION_SCORE_COMMENT)
                    continue;

                auto scoreData = PocketDb::TransRepoInst.GetScoreData(tx->GetHash().GetHex());
                if (!scoreData)
                    throw std::runtime_error(strprintf("%s: Failed get score data for tx: %s\n",
                        __func__, tx->GetHash().GetHex()));

                //LogPrintf("@@@ 2.1 Winners - tx:%s - score data - %s\n", tx->GetHash().GetHex(),
                //    scoreData->Serialize()->write());

                if (scoreData->ScoreType == PocketTx::PocketTxType::ACTION_SCORE_COMMENT && scoreData->ScoreValue != 1)
                    continue;

                if (scoreData->ScoreType == PocketTx::PocketTxType::ACTION_SCORE_POST &&
                    scoreData->ScoreValue != 4 && scoreData->ScoreValue != 5)
                    continue;

                if (!reputationConsensus->AllowModifyReputation(
                    scoreData,
                    tx,
                    Height,
                    true
                ))
                    continue;

                if (scoreData->ScoreType == PocketTx::PocketTxType::ACTION_SCORE_POST)
                {
                    postCandidates[scoreData->ContentAddressHash] += (scoreData->ScoreValue - 3);
                    ExtendReferrer(scoreData->ContentAddressHash, (int64_t) tx->nTime, postReferrersCandidates);
                }

                if (scoreData->ScoreType == PocketTx::PocketTxType::ACTION_SCORE_COMMENT)
                {
                    commentCandidates[scoreData->ContentAddressHash] += scoreData->ScoreValue;
                    ExtendReferrer(scoreData->ContentAddressHash, (int64_t) tx->nTime, commentReferrersCandidates);
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

            return _winners;
        }

        virtual CAmount RatingReward(CAmount nCredit, opcodetype code)
        {
            if (code == OP_WINNER_COMMENT) return nCredit * 0.5 / 10;
            return nCredit * 0.5;
        }

        virtual void ExtendWinnerTypes(opcodetype type, std::vector<opcodetype>& winner_types) {}
    };


    /*******************************************************************************************************************
    *
    *  Lottery checkpoint at 514185 block
    *
    *******************************************************************************************************************/
    class LotteryConsensus_checkpoint_514185 : public LotteryConsensus
    {
    protected:
        int CheckpointHeight() override { return 514185; }
        virtual int GetLotteryReferralDepth() { return 0; }
        void ExtendReferrer(const string& contentAddress, int64_t txTime, map<string, string>& refs) override
        {
            if (refs.find(contentAddress) != refs.end())
                return;

            auto referrer = PocketDb::TransRepoInst.GetReferrer(contentAddress, txTime - GetLotteryReferralDepth());
            if (!referrer) return;

            refs.emplace(contentAddress, *referrer);
        }

    public:
        LotteryConsensus_checkpoint_514185(int height) : LotteryConsensus(height) {}
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
    };


    /*******************************************************************************************************************
    *
    *  Lottery checkpoint at 1035000 block
    *
    *******************************************************************************************************************/
    class LotteryConsensus_checkpoint_1035000 : public LotteryConsensus_checkpoint_514185
    {
    protected:
        int CheckpointHeight() override { return 1035000; }
        int GetLotteryReferralDepth() override { return 30 * 24 * 3600; }

    public :
        LotteryConsensus_checkpoint_1035000(int height) : LotteryConsensus_checkpoint_514185(height) {}
    };


    /*******************************************************************************************************************
    *
    *  Lottery checkpoint at 1124000 block
    *
    *******************************************************************************************************************/
    class LotteryConsensus_checkpoint_1124000 : public LotteryConsensus_checkpoint_1035000
    {
    protected:
        int CheckpointHeight() override { return 1124000; }

    public:
        LotteryConsensus_checkpoint_1124000(int height) : LotteryConsensus_checkpoint_1035000(height) {}
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


    /*******************************************************************************************************************
    *
    *  Lottery checkpoint at 1180000 block
    *
    *******************************************************************************************************************/
    class LotteryConsensus_checkpoint_1180000 : public LotteryConsensus_checkpoint_1124000
    {
    protected:
        int CheckpointHeight() override { return 1180000; }

    public:
        LotteryConsensus_checkpoint_1180000(int height) : LotteryConsensus_checkpoint_1124000(height) {}
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


    /*******************************************************************************************************************
    *
    *  Lottery checkpoint at _ block
    *
    *******************************************************************************************************************/
    // TODO (brangr) (v0.21.0): change GetLotteryReferralDepth Time to Height
    class LotteryConsensus_checkpoint_ : public LotteryConsensus_checkpoint_1180000
    {
    protected:
        int CheckpointHeight() override { return -1; }
        int GetLotteryReferralDepth() override { return -1; }

        void ExtendReferrer(const string& contentAddress, int64_t txTime, map<string, string>& refs) override
        {
            // This logic replaced with ExtendReferrers()
        }
        void ExtendReferrers() override
        {
            auto& postWinners = _winners.PostWinners;
            auto& postRefs = _winners.PostReferrerWinners;
            auto& commentWinners = _winners.CommentWinners;
            auto& commentRefs = _winners.CommentReferrerWinners;

            vector<string> winners;
            for (string addr : postWinners)
                if (find(winners.begin(), winners.end(), addr) == winners.end())
                    winners.push_back(addr);

            for (string addr : commentWinners)
                if (find(winners.begin(), winners.end(), addr) == winners.end())
                    winners.push_back(addr);

            auto referrers = PocketDb::TransRepoInst.GetReferrers(winners, Height - GetLotteryReferralDepth());
            if (referrers->empty()) return;

            for (const auto& it : *referrers)
            {
                if (find(postWinners.begin(), postWinners.end(), it.first) != postWinners.end())
                    if (find(postRefs.begin(), postRefs.end(), it.second) == postRefs.end())
                        postRefs.push_back(it.second);

                if (find(commentWinners.begin(), commentWinners.end(), it.first) != commentWinners.end())
                    if (find(commentRefs.begin(), commentRefs.end(), it.second) == commentRefs.end())
                        commentRefs.push_back(it.second);
            }
        }

    public :
        LotteryConsensus_checkpoint_(int height) : LotteryConsensus_checkpoint_1180000(height) {}
    };


    /*******************************************************************************************************************
    *
    *  Lottery factory for select actual rules version
    *
    *******************************************************************************************************************/
    class LotteryConsensusFactory
    {
    private:
        static inline const std::map<int, std::function<LotteryConsensus*(int height)>> m_rules =
        {
            {1180000, [](int height) { return new LotteryConsensus_checkpoint_1180000(height); }},
            {1124000, [](int height) { return new LotteryConsensus_checkpoint_1124000(height); }},
            {1035000, [](int height) { return new LotteryConsensus_checkpoint_1035000(height); }},
            {514185,  [](int height) { return new LotteryConsensus_checkpoint_514185(height); }},
            {0,       [](int height) { return new LotteryConsensus(height); }},
        };
    public:
        LotteryConsensusFactory() = default;
        static shared_ptr <LotteryConsensus> Instance(int height)
        {
            return shared_ptr<LotteryConsensus>(
                (--m_rules.upper_bound(height))->second(height)
            );
        }
    };
}

#endif // POCKETCONSENSUS_LOTTERY_HPP
