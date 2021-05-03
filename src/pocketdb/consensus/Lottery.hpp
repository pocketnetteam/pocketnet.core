// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 Bitcoin developers
// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETCONSENSUS_LOTTERY_HPP
#define POCKETCONSENSUS_LOTTERY_HPP

#include "pocketdb/consensus/Base.hpp"
#include "pocketdb/services/TransactionSerializer.hpp"

namespace PocketConsensus
{
    typedef std::map<opcodetype, std::vector<std::string>> LotteryWinners;

    /*******************************************************************************************************************
    *
    *  Lottery base class
    *
    *******************************************************************************************************************/
    class LotteryConsensus : public BaseConsensus
    {
    public:

        LotteryConsensus() = default;

        virtual LotteryWinners &
        Winners(const CBlock &block, CDataStream &hashProofOfStakeSource) = 0;

        virtual CAmount
        RatingReward(opcodetype code) = 0;

        virtual void
        ExtendReferrers(const CTransactionRef &tx, PocketTxType txType, string scoreAddress, string destAddress) = 0;

    };


    /*******************************************************************************************************************
    *
    *  Lottery start checkpoint
    *
    *******************************************************************************************************************/
    class LotteryConsensus_checkpoint_0 : public LotteryConsensus
    {
    private:
        LotteryWinners _winners;

    protected:
        const int CheckpointHeight = 0;

        const int RATINGS_PAYOUT_MAX = 25;
        //const int64_t _lottery_referral_depth; // todo (brangr): inherited in checkpoints

        void GetReferrers(std::vector<std::string> &winners, std::map<std::string, std::string> all_referrers,
            std::vector<std::string> &referrers)
        {
            for (auto &addr : winners)
            {
                if (all_referrers.find(addr) != all_referrers.end())
                {
                    referrers.push_back(all_referrers[addr]);
                }
            }
        }

        virtual tuple<bool, string, int> ParseScoreAsm(PocketTxType txType, std::vector<std::string> vasm)
        {
            if (vasm.size() < 4)
                return make_tuple(false, "", 0);

            std::vector<unsigned char> _data_hex = ParseHex(vasm[3]);
            std::string _data_str(_data_hex.begin(), _data_hex.end());
            std::vector<std::string> _data;
            boost::split(_data, _data_str, boost::is_any_of("\t "));

            if (_data.size() >= 2)
                return make_tuple(true, _data[0], std::stoi(_data[1]));

            return make_tuple(false, "", 0);
        }

    public:

        LotteryConsensus_checkpoint_0() = default;







        // TODO (brangr): !!!!!!!!!!!!!!!!!!!
        // Create transactions for all winners
//     bool ret = false;
//     ret = GenerateOuts(nCredit, results, totalAmount, vLotteryPost, OP_WINNER_POST, height, winner_types) || ret;
//     ret = GenerateOuts(nCredit, results, totalAmount, vLotteryComment, OP_WINNER_COMMENT, height, winner_types) || ret;
//
//     if (height >= Params().GetConsensus().lottery_referral_beg) {
//         std::vector<std::string> vLotteryPostRef;
//         GetReferrers(vLotteryPost, mLotteryPostRef, vLotteryPostRef);
//
//         std::vector<std::string> vLotteryCommentRef;
//         GetReferrers(vLotteryComment, mLotteryCommentRef, vLotteryCommentRef);
//
//         ret = GenerateOuts(nCredit, results, totalAmount, vLotteryPostRef, OP_WINNER_POST_REFERRAL, height, winner_types) || ret;
//         ret = GenerateOuts(nCredit, results, totalAmount, vLotteryCommentRef, OP_WINNER_COMMENT_REFERRAL, height, winner_types) || ret;
//     }




        // Get all potencial lottery participants
        // Filter and sort
        virtual LotteryWinners &Winners(const CBlock &block, CDataStream &hashProofOfStakeSource) override
        {
            std::vector<std::string> vLotteryPost;
            std::map<std::string, std::string> mLotteryPostRef;
            std::map<std::string, int> allPostRatings;

            std::vector<std::string> vLotteryComment;
            std::map<std::string, std::string> mLotteryCommentRef;
            std::map<std::string, int> allCommentRatings;

            for (const auto &tx : block.vtx)
            {
                // In lottery allowed only likes to posts and comments
                std::vector<std::string> vasm;
                auto txType = PocketServices::TransactionSerializer::ParseType(tx, vasm);
                if (txType != PocketTx::PocketTxType::SCORE_COMMENT_ACTION &&
                    txType != PocketTx::PocketTxType::SCORE_POST_ACTION)
                    continue;

                // Parse asm and get destination address and score value
                if (auto[ok, destAddress, scoreValue] = ParseScoreAsm(txType, vasm); ok)
                {
                    // vasm[1] == OR_SCORE && (_value == 4 || _value == 5)
                    // or
                    // vasm[1] == OR_COMMENT_SCORE && (_value == 1)

                    // нужен адрес лайкера
                    auto scoreAddress = "";
                    // адрес получателя есть

                    //g_antibot->AllowModifyReputationOverPost(_score_address, _post_address, height, tx, true)
                    //g_antibot->AllowModifyReputationOverComment(_score_address, _comment_address, height, tx, true))

                    // posts
                    //if (allPostRatings.find(_post_address) == allPostRatings.end())
                    //    allPostRatings.insert(std::make_pair(_post_address, 0));
                    //allPostRatings[_post_address] += (_value - 3);

                    // comments
                    //if (allCommentRatings.find(_comment_address) ==
                    //    allCommentRatings.end())
                    //    allCommentRatings.insert(std::make_pair(_comment_address, 0));
                    //allCommentRatings[_comment_address] += _value;

                    ExtendReferrers(tx, txType, scoreAddress, destAddress);
                }
            } // for (const auto &tx : block.vtx)

            // Sort founded users
            std::vector<std::pair<std::string, std::pair<int, arith_uint256>>> allPostSorted;
            std::vector<std::pair<std::string, std::pair<int, arith_uint256>>> allCommentSorted;

            // Users with scores by post
            for (
                auto &it: allPostRatings
                )
            {
                CDataStream ss(hashProofOfStakeSource);
                ss << it.first;
                arith_uint256 hashSortRating = UintToArith256(Hash(ss.begin(), ss.end())) / it.second;
                allPostSorted.push_back(std::make_pair(it.first, std::make_pair(it.second, hashSortRating)));
            }

            // Users with scores by comment
            for (
                auto &it: allCommentRatings
                )
            {
                CDataStream ss(hashProofOfStakeSource);
                ss << it.first;
                arith_uint256 hashSortRating = UintToArith256(Hash(ss.begin(), ss.end())) / it.second;
                allCommentSorted.push_back(std::make_pair(it.first, std::make_pair(it.second, hashSortRating)));
            }

            // Shrink founded users
            // Users with scores by post
            if (allPostSorted.size() > 0)
            {
                std::sort(allPostSorted.begin(), allPostSorted.end(), [](auto &a, auto &b)
                {
                    return a.second.second < b.second.second;
                });

                if (allPostSorted.size() > RATINGS_PAYOUT_MAX)
                {
                    allPostSorted.resize(RATINGS_PAYOUT_MAX);
                }

                for (auto &it : allPostSorted)
                {
                    vLotteryPost.push_back(it.first);
                }
            }

            // Users with scores by comment
            if (allCommentSorted.size() > 0)
            {
                std::sort(allCommentSorted.begin(), allCommentSorted.end(), [](auto &a, auto &b)
                {
                    return a.second.second < b.second.second;
                });

                if (allCommentSorted.size() > RATINGS_PAYOUT_MAX)
                {
                    allCommentSorted.resize(RATINGS_PAYOUT_MAX);
                }

                for (auto &it : allCommentSorted)
                {
                    vLotteryComment.push_back(it.first);
                }
            }

            return
                _winners;
        }

    }; // class LotteryConsensus_checkpoint_0


    /*******************************************************************************************************************
    *
    *  Lottery checkpoint at ... block
    *
    *******************************************************************************************************************/
    class LotteryConsensus_checkpoint_1 : public LotteryConsensus_checkpoint_0
    {
    protected:


    public:
//    winners()
//    {
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
//    }
    };


    /*******************************************************************************************************************
    *
    *  Lottery factory for select actual rules version
    *  Каждая новая перегрузка добавляет новый функционал, поддерживающийся с некоторым условием - например высота
    *
    *******************************************************************************************************************/
    class LotteryFactory
    {
    private:
    public:
        shared_ptr<LotteryConsensus> Instance(int height)
        {
            // TODO (brangr): достать подходящий чекпойнт реализацию
        }
    };

}

#endif // POCKETCONSENSUS_LOTTERY_HPP
