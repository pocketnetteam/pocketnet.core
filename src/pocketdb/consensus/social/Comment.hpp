// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 Bitcoin developers
// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETCONSENSUS_COMMENT_HPP
#define POCKETCONSENSUS_COMMENT_HPP

#include "pocketdb/consensus/social/Base.hpp"
#include "pocketdb/models/dto/Comment.hpp"

namespace PocketConsensus
{
    /*******************************************************************************************************************
    *
    *  Comment consensus base class
    *
    *******************************************************************************************************************/
    class CommentConsensus : public SocialBaseConsensus
    {
    protected:
    public:
        CommentConsensus(int height) : SocialBaseConsensus(height) {}

        tuple<bool, SocialConsensusResult> Validate(shared_ptr<Transaction> tx, PocketBlock& block) override
        {
            // TODO (brangr): implement
            // std::string _address = oitm["address"].get_str();
            // std::string _txid = oitm["txid"].get_str();
            // int64_t _time = oitm["time"].get_int64();

            // std::string _msg = oitm["msg"].get_str();
            // std::string _otxid = oitm["otxid"].get_str();
            // std::string _postid = oitm["postid"].get_str();
            // std::string _parentid = oitm["parentid"].get_str();
            // std::string _answerid = oitm["answerid"].get_str();

            // if (!CheckRegistration(oitm, _address, checkMempool, checkWithTime, height, blockVtx, result)) {
            //     return false;
            // }

            // // Size message limit
            // if (_msg == "" || UrlDecode(_msg).length() > GetActualLimit(Limit::comment_size_limit, height)) {
            //     result = ANTIBOTRESULT::Size;
            //     return false;
            // }

            // // Parent comment
            // if (_parentid != "" && !g_pocketdb->Exists(Query("Comment").Where("otxid", CondEq, _parentid).Where("last", CondEq, true).Not().Where("msg", CondEq, "").Where("block", CondLt, height))) {
            //     result = ANTIBOTRESULT::InvalidParentComment;
            //     return false;
            // }

            // // Answer comment
            // if (_answerid != "" && !g_pocketdb->Exists(Query("Comment").Where("otxid", CondEq, _answerid).Where("last", CondEq, true).Not().Where("msg", CondEq, "").Where("block", CondLt, height))) {
            //     result = ANTIBOTRESULT::InvalidAnswerComment;
            //     return false;
            // }

            // Item post_itm;
            // if (_postid == "" || !g_pocketdb->SelectOne(Query("Posts").Where("txid", CondEq, _postid).Where("block", CondLt, height), post_itm).ok()) {
            //     result = ANTIBOTRESULT::NotFound;
            //     return false;
            // }

            // // Blocking
            // if (g_pocketdb->Exists(Query("BlockingView").Where("address", CondEq, post_itm["address"].As<string>()).Where("address_to", CondEq, _address).Where("block", CondLt, height))) {
            //     result = ANTIBOTRESULT::Blocking;
            //     return false;
            // }

            // // Compute count of comments for last 24 hours
            // {
            //     int commentsCount = g_pocketdb->SelectCount(Query("Comment").Where("address", CondEq, _address).Where("last", CondEq, true).Where("time", CondGe, _time - 86400).Where("block", CondLt, height));

            //     // Also check mempool
            //     if (checkMempool) {
            //         reindexer::QueryResults res;
            //         if (g_pocketdb->Select(reindexer::Query("Mempool").Where("table", CondEq, "Comment").Not().Where("txid", CondEq, _txid), res).ok()) {
            //             for (auto& m : res) {
            //                 reindexer::Item mItm = m.GetItem();
            //                 std::string t_src = DecodeBase64(mItm["data"].As<string>());

            //                 reindexer::Item t_itm = g_pocketdb->DB()->NewItem("Comment");
            //                 if (t_itm.FromJSON(t_src).ok()) {
            //                     if (t_itm["address"].As<string>() == _address && t_itm["otxid"].As<string>() == t_itm["txid"].As<string>()) {
            //                         if (!checkWithTime || t_itm["time"].As<int64_t>() <= _time)
            //                             commentsCount += 1;
            //                     }
            //                 }
            //             }
            //         }
            //     }

            //     // Check block
            //     if (blockVtx.Exists("Comment")) {
            //         for (auto& mtx : blockVtx.Data["Comment"]) {
            //             if (mtx["txid"].get_str() != _txid && mtx["address"].get_str() == _address && mtx["otxid"].get_str() == mtx["txid"].get_str()) {
            //                 if (!checkWithTime || mtx["time"].get_int64() <= _time)
            //                     commentsCount += 1;
            //             }
            //         }
            //     }

            //     // Check limit
            //     ABMODE mode;
            //     getMode(_address, mode, height);
            //     int limit = getLimit(Comment, mode, height);
            //     if (commentsCount >= limit) {
            //         result = ANTIBOTRESULT::CommentLimit;
            //         return false;
            //     }
            // }

            // return true;
        }
  /*      
        bool AntiBot::check_comment_edit(const UniValue oitm, BlockVTX& blockVtx, bool checkMempool, bool checkWithTime, int height, ANTIBOTRESULT& result)
        {
            std::string _address = oitm["address"].get_str();
            int64_t _time = oitm["time"].get_int64();

            std::string _msg = oitm["msg"].get_str();
            std::string _txid = oitm["txid"].get_str();
            std::string _otxid = oitm["otxid"].get_str();
            std::string _postid = oitm["postid"].get_str();
            std::string _parentid = oitm["parentid"].get_str();
            std::string _answerid = oitm["answerid"].get_str();

            // User registered?
            if (!CheckRegistration(oitm, _address, checkMempool, checkWithTime, height, blockVtx, result)) {
                return false;
            }

            // Size message limit
            if (_msg == "" || UrlDecode(_msg).length() > GetActualLimit(Limit::comment_size_limit, height)) {
                result = ANTIBOTRESULT::Size;
                return false;
            }

            // Original comment exists
            reindexer::Item _original_comment_itm;
            if (!g_pocketdb->SelectOne(Query("Comment").Where("otxid", CondEq, _otxid).Where("txid", CondEq, _otxid).Where("address", CondEq, _address).Where("block", CondLt, height), _original_comment_itm).ok()) {
                result = ANTIBOTRESULT::NotFound;
                return false;
            }

            // Last comment not deleted
            if (!g_pocketdb->Exists(Query("Comment").Where("otxid", CondEq, _otxid).Where("last", CondEq, true).Where("address", CondEq, _address).Not().Where("msg", CondEq, "").Where("block", CondLt, height))) {
                result = ANTIBOTRESULT::CommentDeletedEdit;
                return false;
            }

            // Parent comment
            if (_parentid != _original_comment_itm["parentid"].As<string>() || (_parentid != "" && !g_pocketdb->Exists(Query("Comment").Where("otxid", CondEq, _parentid).Where("last", CondEq, true).Not().Where("msg", CondEq, "").Where("block", CondLt, height)))) {
                result = ANTIBOTRESULT::InvalidParentComment;
                return false;
            }

            // Answer comment
            if (_answerid != _original_comment_itm["answerid"].As<string>() || (_answerid != "" && !g_pocketdb->Exists(Query("Comment").Where("otxid", CondEq, _answerid).Where("last", CondEq, true).Not().Where("msg", CondEq, "").Where("block", CondLt, height)))) {
                result = ANTIBOTRESULT::InvalidAnswerComment;
                return false;
            }

            // Original comment edit only 24 hours
            if (_time - _original_comment_itm["time"].As<int64_t>() > GetActualLimit(Limit::edit_comment_timeout, height)) {
                result = ANTIBOTRESULT::CommentEditLimit;
                return false;
            }

            Item post_itm;
            if (_postid == "" || !g_pocketdb->SelectOne(Query("Posts").Where("txid", CondEq, _postid).Where("block", CondLt, height), post_itm).ok()) {
                result = ANTIBOTRESULT::NotFound;
                return false;
            }

            // Blocking
            if (g_pocketdb->Exists(Query("BlockingView").Where("address", CondEq, post_itm["address"].As<string>()).Where("address_to", CondEq, _address).Where("block", CondLt, height))) {
                result = ANTIBOTRESULT::Blocking;
                return false;
            }

            // Double edit in block denied
            if (blockVtx.Exists("Comment")) {
                for (auto& mtx : blockVtx.Data["Comment"]) {
                    if (mtx["txid"].get_str() != _txid && mtx["otxid"].get_str() == _otxid) {
                        result = ANTIBOTRESULT::DoubleCommentEdit;
                        return false;
                    }
                }
            }

            // Double edit in mempool denied
            if (checkMempool) {
                reindexer::QueryResults res;
                if (g_pocketdb->Select(reindexer::Query("Mempool").Where("table", CondEq, "Comment").Not().Where("txid", CondEq, _txid), res).ok()) {
                    for (auto& m : res) {
                        reindexer::Item mItm = m.GetItem();
                        std::string t_src = DecodeBase64(mItm["data"].As<string>());

                        reindexer::Item t_itm = g_pocketdb->DB()->NewItem("Comment");
                        if (t_itm.FromJSON(t_src).ok()) {
                            if (t_itm["otxid"].As<string>() == _otxid) {
                                result = ANTIBOTRESULT::DoubleCommentEdit;
                                return false;
                            }
                        }
                    }
                }
            }

            // Check limit
            {
                size_t edit_count = g_pocketdb->SelectCount(Query("Comment").Where("otxid", CondEq, _otxid).Where("block", CondLt, height));

                ABMODE mode;
                getMode(_address, mode, height);
                int limit = getLimit(CommentEdit, mode, height);
                if (edit_count >= limit) {
                    result = ANTIBOTRESULT::CommentEditLimit;
                    return false;
                }
            }

            return true;
        }
*/
    };

    /*******************************************************************************************************************
    *
    *  Consensus checkpoint at 1 block
    *
    *******************************************************************************************************************/
    class CommentConsensus_checkpoint_1 : public CommentConsensus
    {
    protected:
        int CheckpointHeight() override { return 1; }
    public:
        CommentConsensus_checkpoint_1(int height) : CommentConsensus(height) {}
    };


    /*******************************************************************************************************************
    *
    *  Factory for select actual rules version
    *  Каждая новая перегрузка добавляет новый функционал, поддерживающийся с некоторым условием - например высота
    *
    *******************************************************************************************************************/
    class CommentConsensusFactory
    {
    private:
        static inline const std::map<int, std::function<CommentConsensus*(int height)>> m_rules =
        {
            {1, [](int height) { return new CommentConsensus_checkpoint_1(height); }},
            {0, [](int height) { return new CommentConsensus(height); }},
        };
    public:
        shared_ptr <CommentConsensus> Instance(int height)
        {
            return shared_ptr<CommentConsensus>(
                (--m_rules.upper_bound(height))->second(height)
            );
        }
    };
}

#endif // POCKETCONSENSUS_COMMENT_HPP