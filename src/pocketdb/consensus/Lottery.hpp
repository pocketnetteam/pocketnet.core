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
#include "pocketdb/helpers/TransactionHelper.hpp"

namespace PocketConsensus
{
    using std::map;
    using std::vector;
    using std::string;
    using std::pair;

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
        virtual void ExtendReferrers() {}
    public:
        LotteryConsensus() = default;
        virtual LotteryWinners &Winners(const CBlock &block, CDataStream &hashProofOfStakeSource) { return _winners; }
        virtual CAmount RatingReward(CAmount nCredit, opcodetype code) {}
        virtual void ExtendWinnerTypes(opcodetype type, std::vector<opcodetype>& winner_types) {}
    };


    /*******************************************************************************************************************
    *
    *  Lottery start checkpoint
    *
    *******************************************************************************************************************/
    class LotteryConsensus_checkpoint_0 : public LotteryConsensus
    {
    protected:

        // in vasm placed score destination address and score value
        // We need parse and check this data for general lottery rules
        // Also wee allowed scores to comments and posts only 
        virtual tuple<bool, string, int> ParseScoreAsm(PocketTxType txType, vector<string> vasm)
        {
            if (txType != PocketTx::PocketTxType::ACTION_SCORE_COMMENT && txType != PocketTx::PocketTxType::ACTION_SCORE_POST)
                return make_tuple(false, "", 0);

            if (vasm.size() < 4)
                return make_tuple(false, "", 0);

            vector<unsigned char> _data_hex = ParseHex(vasm[3]);
            string _data_str(_data_hex.begin(), _data_hex.end());
            vector<string> _data;
            boost::split(_data, _data_str, boost::is_any_of("\t "));

            if (_data.size() >= 2)
            {
                string addr = _data[0];
                int val = std::stoi(_data[1]);

                if (txType == PocketTx::PocketTxType::ACTION_SCORE_COMMENT && val != 1)
                    return make_tuple(false, "", 0);
                
                if (txType == PocketTx::PocketTxType::ACTION_SCORE_POST && val != 4 && val != 5)
                    return make_tuple(false, "", 0);
                
                // All good
                return make_tuple(true, addr, val);
            }

            return make_tuple(false, "", 0);
        }

        void SortWinners(map<string, int>& candidates, CDataStream &hashProofOfStakeSource, vector<string>& winners)
        {
            vector<pair<string, pair<int, arith_uint256>>> candidatesSorted;
            for (auto &it: candidates)
            {
                CDataStream ss(hashProofOfStakeSource);
                ss << it.first;
                arith_uint256 hashSortRating = UintToArith256(Hash(ss.begin(), ss.end())) / it.second;
                candidatesSorted.push_back(std::make_pair(it.first, std::make_pair(it.second, hashSortRating)));
            }

            if (!candidatesSorted.empty())
            {
                std::sort(candidatesSorted.begin(), candidatesSorted.end(), [](auto &a, auto &b)
                {
                    return a.second.second < b.second.second;
                });

                if ((int)candidatesSorted.size() > MaxWinnersCount())
                    candidatesSorted.resize(MaxWinnersCount());

                for (auto &it : candidatesSorted)
                    winners.push_back(it.first);
            }
        }

        // void GetReferrers(vector<string> &winners, map<string, string> all_referrers,
        //     vector<string> &referrers)
        // {
        //     for (auto &addr : winners)
        //     {
        //         if (all_referrers.find(addr) != all_referrers.end())
        //         {
        //             referrers.push_back(all_referrers[addr]);
        //         }
        //     }
        // }

    public:

        LotteryConsensus_checkpoint_0() = default;

        // Get all lottery winner
        LotteryWinners &Winners(const CBlock &block, CDataStream &hashProofOfStakeSource) override
        {
            map<string, int> postCandidates;
            map<string, int> commentCandidates;

            for (const auto &tx : block.vtx)
            {
                vector<string> vasm;
                auto txType = PocketHelpers::ParseType(tx, vasm);

                // Parse asm and get destination address and score value
                // In lottery allowed only likes to posts and comments
                // Also in lottery allowed only positive scores
                auto[ok1, destAddress, scoreValue] = ParseScoreAsm(txType, vasm);
                if (!ok1) continue;
                
                // 1st output in transaction always send money to self
                auto[ok2, scoreAddress] = PocketHelpers::GetPocketAuthorAddress(tx);
                if (!ok2) continue;

                // TODO (brangr): implement reputation check
                //g_antibot->AllowModifyReputationOverPost(_score_address, _post_address, height, tx, true)
                //g_antibot->AllowModifyReputationOverComment(_score_address, _comment_address, height, tx, true))

                if (txType != PocketTx::PocketTxType::ACTION_SCORE_POST)
                    postCandidates[destAddress] += (scoreValue - 3);;

                if (txType == PocketTx::PocketTxType::ACTION_SCORE_COMMENT)
                    commentCandidates[destAddress] += scoreValue;
            }

            // Sort founded users
            SortWinners(postCandidates, hashProofOfStakeSource, _winners.PostWinners);
            SortWinners(commentCandidates, hashProofOfStakeSource, _winners.CommentWinners);

            // Find referrers for final winners list
            ExtendReferrers();

            return _winners;
        }

        CAmount RatingReward(CAmount nCredit, opcodetype code) override
        {
            // TODO (brangr): implement
            // ratingReward = nCredit * 0.5;
            // if (op_code_type == OP_WINNER_COMMENT) ratingReward = ratingReward / 10;
            // totalAmount += ratingReward;
        }

    }; // class LotteryConsensus_checkpoint_0


    /*******************************************************************************************************************
    *
    *  Lottery checkpoint at 514185 block
    *
    *******************************************************************************************************************/
    class LotteryConsensus_checkpoint_514185 : public LotteryConsensus_checkpoint_0
    {
    protected:
        int CheckpointHeight() override { return 514185; }
        void ExtendReferrers() override
        {
            // TODO (brangr): implement
//        // Find winners with referral program
//        if (height >= Params().GetConsensus().lottery_referral_beg)
//        {
//            reindexer::Item _referrer_itm;
//
//            reindexer::Query _referrer_query = reindexer::Query("UsersView").Where(
//                "address",
//                CondEq, _post_address).Not().Where("referrer", CondEq, "").Not().Where(
//                "referrer", CondEq, _score_address);
//            if (height >= Params().GetConsensus().lottery_referral_limitation)
//            {
//                _referrer_query.Where("regdate", CondGe,
//                    (int64_t) tx->nTime - _lottery_referral_depth);
//            }
//
//            if (g_pocketdb->SelectOne(_referrer_query, _referrer_itm).ok())
//            {
//                if (mLotteryPostRef.find(_post_address) == mLotteryPostRef.end())
//                {
//                    auto _referrer_address = _referrer_itm["referrer"].As<string>();
//                    mLotteryPostRef.emplace(_post_address, _referrer_address);
//                }
//            }
//        }
//
//        // Find winners with referral program
//        if (height >= Params().GetConsensus().lottery_referral_beg)
//        {
//            reindexer::Item _referrer_itm;
//
//            reindexer::Query _referrer_query = reindexer::Query("UsersView").Where(
//                "address",
//                CondEq, _comment_address).Not().Where("referrer", CondEq, "").Not().Where(
//                "referrer", CondEq, _score_address);
//            if (height >= Params().GetConsensus().lottery_referral_limitation)
//            {
//                _referrer_query.Where("regdate", CondGe,
//                    (int64_t) tx->nTime - _lottery_referral_depth);
//            }
//
//            if (g_pocketdb->SelectOne(_referrer_query, _referrer_itm).ok())
//            {
//                if (mLotteryCommentRef.find(_comment_address) == mLotteryCommentRef.end())
//                {
//                    auto _referrer_address = _referrer_itm["referrer"].As<string>();
//                    mLotteryCommentRef.emplace(_comment_address, _referrer_address);
//                }
//            }
//        }
        }
    public:
        void ExtendWinnerTypes(opcodetype type, std::vector<opcodetype>& winner_types) override
        {
            winner_types.push_back(type);
        }
        CAmount RatingReward(CAmount nCredit, opcodetype code) override
        {
            // TODO (brangr): implement
            // Referrer program 5 - 100%; 2.0 - nodes; 3.0 - all for lottery;
            // 2.0 - posts; 0.4 - referrer over posts (20%); 0.5 - comment; 0.1 - referrer over comment (20%);
            // if (op_code_type == OP_WINNER_POST) ratingReward = nCredit * 0.40;
            // if (op_code_type == OP_WINNER_POST_REFERRAL) ratingReward = nCredit * 0.08;
            // if (op_code_type == OP_WINNER_COMMENT) ratingReward = nCredit * 0.10;
            // if (op_code_type == OP_WINNER_COMMENT_REFERRAL) ratingReward = nCredit * 0.02;
            // totalAmount += ratingReward;
        }
    };


    /*******************************************************************************************************************
    *
    *  Lottery checkpoint at 1124000 block
    *
    *******************************************************************************************************************/
    class LotteryConsensus_checkpoint_1124000 : public LotteryConsensus_checkpoint_514185
    {
    protected:
        int CheckpointHeight() override { return 1124000; }
    public:
        CAmount RatingReward(CAmount nCredit, opcodetype code) override
        {
            // TODO (brangr): implement
            // Referrer program 5 - 100%; 4.75 - nodes; 0.25 - all for lottery;
            // .1 - posts (2%); .1 - referrer over posts (2%); 0.025 - comment (.5%); 0.025 - referrer over comment (.5%);
            // if (op_code_type == OP_WINNER_POST) ratingReward = nCredit * 0.02;
            // if (op_code_type == OP_WINNER_POST_REFERRAL) ratingReward = nCredit * 0.02;
            // if (op_code_type == OP_WINNER_COMMENT) ratingReward = nCredit * 0.005;
            // if (op_code_type == OP_WINNER_COMMENT_REFERRAL) ratingReward = nCredit * 0.005;
            // totalAmount += ratingReward;
        }
    };


    /*******************************************************************************************************************
    *
    *  Lottery checkpoint at 1035000 block
    *
    *******************************************************************************************************************/
    class LotteryConsensus_checkpoint_1035000 : public LotteryConsensus_checkpoint_1124000
    {
    protected:
        int CheckpointHeight() override { return 1035000; }
        void ExtendReferrers() override
        {
            // TODO (brangr): implement
//        // Find winners with referral program
//        if (height >= Params().GetConsensus().lottery_referral_beg)
//        {
//            reindexer::Item _referrer_itm;
//
//            reindexer::Query _referrer_query = reindexer::Query("UsersView").Where(
//                "address",
//                CondEq, _post_address).Not().Where("referrer", CondEq, "").Not().Where(
//                "referrer", CondEq, _score_address);
//            if (height >= Params().GetConsensus().lottery_referral_limitation)
//            {
//                _referrer_query.Where("regdate", CondGe,
//                    (int64_t) tx->nTime - _lottery_referral_depth);
//            }
//
//            if (g_pocketdb->SelectOne(_referrer_query, _referrer_itm).ok())
//            {
//                if (mLotteryPostRef.find(_post_address) == mLotteryPostRef.end())
//                {
//                    auto _referrer_address = _referrer_itm["referrer"].As<string>();
//                    mLotteryPostRef.emplace(_post_address, _referrer_address);
//                }
//            }
//        }
//
//        // Find winners with referral program
//        if (height >= Params().GetConsensus().lottery_referral_beg)
//        {
//            reindexer::Item _referrer_itm;
//
//            reindexer::Query _referrer_query = reindexer::Query("UsersView").Where(
//                "address",
//                CondEq, _comment_address).Not().Where("referrer", CondEq, "").Not().Where(
//                "referrer", CondEq, _score_address);
//            if (height >= Params().GetConsensus().lottery_referral_limitation)
//            {
//                _referrer_query.Where("regdate", CondGe,
//                    (int64_t) tx->nTime - _lottery_referral_depth);
//            }
//
//            if (g_pocketdb->SelectOne(_referrer_query, _referrer_itm).ok())
//            {
//                if (mLotteryCommentRef.find(_comment_address) == mLotteryCommentRef.end())
//                {
//                    auto _referrer_address = _referrer_itm["referrer"].As<string>();
//                    mLotteryCommentRef.emplace(_comment_address, _referrer_address);
//                }
//            }
//        }
        }
    };


    /*******************************************************************************************************************
    *
    *  Lottery checkpoint at 1180000 block
    *
    *******************************************************************************************************************/
    class LotteryConsensus_checkpoint_1180000 : public LotteryConsensus_checkpoint_1035000
    {
    protected:
        int CheckpointHeight() override { return 1180000; }
    public:
        CAmount RatingReward(CAmount nCredit, opcodetype code) override
        {
            // TODO (brangr): implement
            // Reduce all winnings by 10 times
            // Referrer program 5 - 100%; 4.975 - nodes; 0.025 - all for lottery;
            // if (op_code_type == OP_WINNER_POST) ratingReward = nCredit * 0.002;
            // if (op_code_type == OP_WINNER_POST_REFERRAL) ratingReward = nCredit * 0.002;
            // if (op_code_type == OP_WINNER_COMMENT) ratingReward = nCredit * 0.0005;
            // if (op_code_type == OP_WINNER_COMMENT_REFERRAL) ratingReward = nCredit * 0.0005;
            // totalAmount += ratingReward;
        }
    };


    /*******************************************************************************************************************
    *
    *  Lottery factory for select actual rules version
    *  Каждая новая перегрузка добавляет новый функционал, поддерживающийся с некоторым условием - например высота
    *
    *******************************************************************************************************************/
    class LotteryConsensusFactory
    {
    private:
    public:
        LotteryConsensusFactory() = default;
        shared_ptr<LotteryConsensus> Instance(int height)
        {
            // TODO (brangr): implement достать подходящий чекпойнт реализацию
            auto instPtr = shared_ptr<LotteryConsensus_checkpoint_1180000>(new LotteryConsensus_checkpoint_1180000());
        }
    };

}

#endif // POCKETCONSENSUS_LOTTERY_HPP
