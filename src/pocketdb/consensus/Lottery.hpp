// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 Bitcoin developers
// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETCONSENSUS_LOTTERY_HPP
#define POCKETCONSENSUS_LOTTERY_HPP

#include "pocketdb/consensus/Base.hpp"

namespace PocketConsensus
{
    typedef std::map<opcodetype, std::vector<std::string>> LotteryWinners;

    class LotteryConsensus : public BaseConsensus {
    public:
        LotteryConsensus() = default;
        virtual LotteryWinners &Winners(const CBlock &block) = 0;
        virtual CAmount RatingReward(opcodetype code) = 0;
    };

    class LotteryConsensus_checkpoint_0 : public LotteryConsensus
    {
    private:
        LotteryWinners _winners;

    protected:
        const int CheckpointHeight = 0;

        const int RATINGS_PAYOUT_MAX = 25;
        const int64_t _lottery_referral_depth; // todo (brangr): inherited in checkpoints

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
        LotteryWinners &Winners(const CBlock &block)
        {
//            std::vector <std::string> vLotteryPost;
//            std::map <std::string, std::string> mLotteryPostRef;
//            std::map<std::string, int> allPostRatings;
//
//            std::vector <std::string> vLotteryComment;
//            std::map <std::string, std::string> mLotteryCommentRef;
//            std::map<std::string, int> allCommentRatings;
//
//            for (const auto &tx : blockPrev.vtx)
//            {
//                std::vector <std::string> vasm;
//                if (!FindPocketNetAsmString(tx, vasm)) continue;
//                if ((vasm[1] == OR_SCORE || vasm[1] == OR_COMMENT_SCORE) && vasm.size() >= 4)
//                {
//                    std::vector<unsigned char> _data_hex = ParseHex(vasm[3]);
//                    std::string _data_str(_data_hex.begin(), _data_hex.end());
//                    std::vector <std::string> _data;
//                    boost::split(_data, _data_str, boost::is_any_of("\t "));
//                    if (_data.size() >= 2)
//                    {
//                        std::string _address = _data[0];
//                        int _value = std::stoi(_data[1]);
//
//                        // For lottery use scores as 4=1 and 5=2 - Scores to posts
//                        if (vasm[1] == OR_SCORE && (_value == 4 || _value == 5))
//                        {
//                            std::string _score_address;
//                            std::string _post_address;
//
//                            // Get address of score initiator
//                            reindexer::Item _score_itm;
//                            if (g_pocketdb->SelectOne(
//                                reindexer::Query("Scores").Where("txid", CondEq, tx->GetHash().GetHex()),
//                                _score_itm).ok())
//                                _score_address = _score_itm["address"].As<string>();
//
//                            reindexer::Item _post_itm;
//                            if (g_pocketdb->SelectOne(
//                                reindexer::Query("Posts").Where("txid", CondEq, _score_itm["posttxid"].As<string>()),
//                                _post_itm).ok())
//                                _post_address = _post_itm["address"].As<string>();
//
//                            if (_score_address.empty() || _post_address.empty())
//                            {
//                                LogPrintf("GetRatingRewards error: _score_address='%s' _post_address='%s'\n",
//                                    _score_address, _post_address);
//                                continue;
//                            }
//
//                            if (_address == _post_address &&
//                                g_antibot->AllowModifyReputationOverPost(_score_address, _post_address, height, tx,
//                                    true))
//                            {
//                                if (allPostRatings.find(_post_address) == allPostRatings.end())
//                                    allPostRatings.insert(std::make_pair(_post_address, 0));
//                                allPostRatings[_post_address] += (_value - 3);
//
//                                // Find winners with referral program
//                                if (height >= Params().GetConsensus().lottery_referral_beg)
//                                {
//                                    reindexer::Item _referrer_itm;
//
//                                    reindexer::Query _referrer_query = reindexer::Query("UsersView").Where("address",
//                                        CondEq, _post_address).Not().Where("referrer", CondEq, "").Not().Where(
//                                        "referrer", CondEq, _score_address);
//                                    if (height >= Params().GetConsensus().lottery_referral_limitation)
//                                    {
//                                        _referrer_query.Where("regdate", CondGe,
//                                            (int64_t) tx->nTime - _lottery_referral_depth);
//                                    }
//
//                                    if (g_pocketdb->SelectOne(_referrer_query, _referrer_itm).ok())
//                                    {
//                                        if (mLotteryPostRef.find(_post_address) == mLotteryPostRef.end())
//                                        {
//                                            auto _referrer_address = _referrer_itm["referrer"].As<string>();
//                                            mLotteryPostRef.emplace(_post_address, _referrer_address);
//                                        }
//                                    }
//                                }
//                            }
//                        }
//
//                        // For lottery use scores as 1 and -1 - Scores to comments
//                        if (vasm[1] == OR_COMMENT_SCORE && (_value == 1))
//                        {
//                            std::string _score_address;
//                            std::string _comment_address;
//
//                            // Get address of score initiator
//                            reindexer::Item _score_itm;
//                            if (g_pocketdb->SelectOne(
//                                reindexer::Query("CommentScores").Where("txid", CondEq, tx->GetHash().GetHex()),
//                                _score_itm).ok())
//                                _score_address = _score_itm["address"].As<string>();
//
//                            reindexer::Item _comment_itm;
//                            if (g_pocketdb->SelectOne(
//                                reindexer::Query("Comment").Where("txid", CondEq, _score_itm["commentid"].As<string>()),
//                                _comment_itm).ok())
//                                _comment_address = _comment_itm["address"].As<string>();
//
//                            if (_score_address.empty() || _comment_address.empty())
//                            {
//                                LogPrintf("GetRatingRewards error: _score_address='%s' _comment_address='%s'\n",
//                                    _score_address, _comment_address);
//                                continue;
//                            }
//
//                            if (_address == _comment_address &&
//                                g_antibot->AllowModifyReputationOverComment(_score_address, _comment_address, height,
//                                    tx, true))
//                            {
//                                if (allCommentRatings.find(_comment_address) ==
//                                    allCommentRatings.end())
//                                    allCommentRatings.insert(std::make_pair(_comment_address, 0));
//                                allCommentRatings[_comment_address] += _value;
//
//                                // Find winners with referral program
//                                if (height >= Params().GetConsensus().lottery_referral_beg)
//                                {
//                                    reindexer::Item _referrer_itm;
//
//                                    reindexer::Query _referrer_query = reindexer::Query("UsersView").Where("address",
//                                        CondEq, _comment_address).Not().Where("referrer", CondEq, "").Not().Where(
//                                        "referrer", CondEq, _score_address);
//                                    if (height >= Params().GetConsensus().lottery_referral_limitation)
//                                    {
//                                        _referrer_query.Where("regdate", CondGe,
//                                            (int64_t) tx->nTime - _lottery_referral_depth);
//                                    }
//
//                                    if (g_pocketdb->SelectOne(_referrer_query, _referrer_itm).ok())
//                                    {
//                                        if (mLotteryCommentRef.find(_comment_address) == mLotteryCommentRef.end())
//                                        {
//                                            auto _referrer_address = _referrer_itm["referrer"].As<string>();
//                                            mLotteryCommentRef.emplace(_comment_address, _referrer_address);
//                                        }
//                                    }
//                                }
//                            }
//                        }
//
//                    }
//                }
//            } // for
//
//            // Sort founded users
//            std::vector < std::pair < std::string, std::pair < int, arith_uint256>>> allPostSorted;
//            std::vector < std::pair < std::string, std::pair < int, arith_uint256>>> allCommentSorted;
//
//            // Users with scores by post
//            for (auto &it : allPostRatings)
//            {
//                CDataStream ss(hashProofOfStakeSource);
//                ss << it.first;
//                arith_uint256 hashSortRating = UintToArith256(Hash(ss.begin(), ss.end())) / it.second;
//                allPostSorted.push_back(std::make_pair(it.first, std::make_pair(it.second, hashSortRating)));
//            }
//
//            // Users with scores by comment
//            for (auto &it : allCommentRatings)
//            {
//                CDataStream ss(hashProofOfStakeSource);
//                ss << it.first;
//                arith_uint256 hashSortRating = UintToArith256(Hash(ss.begin(), ss.end())) / it.second;
//                allCommentSorted.push_back(std::make_pair(it.first, std::make_pair(it.second, hashSortRating)));
//            }
//
//            // Shrink founded users
//            // Users with scores by post
//            if (allPostSorted.size() > 0)
//            {
//                std::sort(allPostSorted.begin(), allPostSorted.end(), [](auto &a, auto &b)
//                {
//                    return a.second.second < b.second.second;
//                });
//
//                if (allPostSorted.size() > RATINGS_PAYOUT_MAX)
//                {
//                    allPostSorted.resize(RATINGS_PAYOUT_MAX);
//                }
//
//                for (auto &it : allPostSorted)
//                {
//                    vLotteryPost.push_back(it.first);
//                }
//            }
//
//            // Users with scores by comment
//            if (allCommentSorted.size() > 0)
//            {
//                std::sort(allCommentSorted.begin(), allCommentSorted.end(), [](auto &a, auto &b)
//                {
//                    return a.second.second < b.second.second;
//                });
//
//                if (allCommentSorted.size() > RATINGS_PAYOUT_MAX)
//                {
//                    allCommentSorted.resize(RATINGS_PAYOUT_MAX);
//                }
//
//                for (auto &it : allCommentSorted)
//                {
//                    vLotteryComment.push_back(it.first);
//                }
//            }

            return _winners;
        }


    };

    /******************************************************
     *
     * Lottery factory for select actual rules version
     *
    *******************************************************/
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
