// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 Bitcoin developers
// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETCONSENSUS_COMMENT_HPP
#define POCKETCONSENSUS_COMMENT_HPP

#include "utils/html.h"

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
    public:
        CommentConsensus(int height) : SocialBaseConsensus(height) {}
        CommentConsensus() : SocialBaseConsensus() {}

    protected:
        
        virtual int64_t GetCommentMessageMaxSize() { return 2000; }
    
        tuple<bool, SocialConsensusResult> ValidateModel(shared_ptr<Transaction> tx) override
        {
            auto ptx = static_pointer_cast<Comment>(tx);

            // TODO (brangr): implement
            // std::string _address = oitm["address"].get_str();
            // std::string _txid = oitm["txid"].get_str();
            // int64_t _time = oitm["time"].get_int64();

            // std::string _msg = oitm["msg"].get_str();
            // std::string _otxid = oitm["otxid"].get_str();
            // std::string _postid = oitm["postid"].get_str();
            // std::string _parentid = oitm["parentid"].get_str();
            // std::string _answerid = oitm["answerid"].get_str();

            vector<string> addresses = { *ptx->GetAddress() };
            if (!PocketDb::ConsensusRepoInst.ExistsUserRegistrations(addresses))
                return make_tuple(false, SocialConsensusResult_NotRegistered);

            

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

            return Success;
        }

        tuple<bool, SocialConsensusResult> ValidateLimit(shared_ptr<Transaction> tx, PocketBlock& block) override
        {
            //     int commentsCount = g_pocketdb->SelectCount(Query("Comment").Where("address", CondEq, _address).Where("last", CondEq, true).Where("time", CondGe, _time - 86400).Where("block", CondLt, height));

            //     if (blockVtx.Exists("Comment")) {
            //         for (auto& mtx : blockVtx.Data["Comment"]) {
            //             if (mtx["txid"].get_str() != _txid && mtx["address"].get_str() == _address && mtx["otxid"].get_str() == mtx["txid"].get_str()) {
            //                 if (!checkWithTime || mtx["time"].get_int64() <= _time)
            //                     commentsCount += 1;
            //             }
            //         }
            //     }

            //     ABMODE mode;
            //     getMode(_address, mode, height);
            //     int limit = getLimit(Comment, mode, height);
            //     if (commentsCount >= limit) {
            //         result = ANTIBOTRESULT::CommentLimit;
            //         return false;
            //     }
        }

        tuple<bool, SocialConsensusResult> ValidateLimit(shared_ptr<Transaction> tx) override
        {
            //     int commentsCount = g_pocketdb->SelectCount(Query("Comment").Where("address", CondEq, _address).Where("last", CondEq, true).Where("time", CondGe, _time - 86400).Where("block", CondLt, height));

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

            //     ABMODE mode;
            //     getMode(_address, mode, height);
            //     int limit = getLimit(Comment, mode, height);
            //     if (commentsCount >= limit) {
            //         result = ANTIBOTRESULT::CommentLimit;
            //         return false;
            //     }
        }


        tuple<bool, SocialConsensusResult> CheckModel(shared_ptr<Transaction> tx) override
        {
            auto ptx = static_pointer_cast<Comment>(tx);

            auto pMsg = ptx->GetPayloadMsg();
            if (!pMsg || (*pMsg).empty() || HtmlUtils::UrlDecode(*pMsg).length() > GetCommentMessageMaxSize())
                return {false, SocialConsensusResult_Size};

            return Success;
        }

    };

    /*******************************************************************************************************************
    *
    *  Factory for select actual rules version
    *
    *******************************************************************************************************************/
    class CommentConsensusFactory
    {
    private:
        static inline const std::map<int, std::function<CommentConsensus*(int height)>> m_rules =
        {
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