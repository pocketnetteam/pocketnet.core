// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 Bitcoin developers
// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETCONSENSUS_COMMENT_DELETE_HPP
#define POCKETCONSENSUS_COMMENT_DELETE_HPP

#include "pocketdb/consensus/social/Base.hpp"
#include "pocketdb/models/dto/CommentDelete.hpp"

namespace PocketConsensus
{
    /*******************************************************************************************************************
    *
    *  CommentDelete consensus base class
    *
    *******************************************************************************************************************/
    class CommentDeleteConsensus : public SocialBaseConsensus
    {
    public:
        CommentDeleteConsensus(int height) : SocialBaseConsensus(height) {}
        CommentDeleteConsensus() : SocialBaseConsensus() {}

    protected:
    
        tuple<bool, SocialConsensusResult> ValidateModel(shared_ptr<Transaction> tx) override
        {
            auto ptx = static_pointer_cast<CommentDelete>(tx);



            return Success;


            // TODO (brangr): implement
            // std::string _address = oitm["address"].get_str();
            // int64_t _time = oitm["time"].get_int64();

            // std::string _txid = oitm["txid"].get_str();
            // std::string _otxid = oitm["otxid"].get_str();
            // std::string _parentid = oitm["parentid"].get_str();
            // std::string _answerid = oitm["answerid"].get_str();

            // // User registered?
            // if (!CheckRegistration(oitm, _address, checkMempool, checkWithTime, height, blockVtx, result)) {
            //     return false;
            // }

            // // Original comment exists
            // reindexer::Item _original_comment_itm;
            // if (!g_pocketdb->SelectOne(Query("Comment").Where("otxid", CondEq, _otxid).Where("txid", CondEq, _otxid).Where("address", CondEq, _address).Where("block", CondLt, height), _original_comment_itm).ok()) {
            //     result = ANTIBOTRESULT::NotFound;
            //     return false;
            // }

            // // Last comment not deleted
            // if (!g_pocketdb->Exists(Query("Comment").Where("otxid", CondEq, _otxid).Where("last", CondEq, true).Where("address", CondEq, _address).Not().Where("msg", CondEq, "").Where("block", CondLt, height))) {
            //     result = ANTIBOTRESULT::DoubleCommentDelete;
            //     return false;
            // }

            // // Parent comment
            // if (_parentid != _original_comment_itm["parentid"].As<string>()) {
            //     result = ANTIBOTRESULT::InvalidParentComment;
            //     return false;
            // }

            // // Answer comment
            // if (_answerid != _original_comment_itm["answerid"].As<string>()) {
            //     result = ANTIBOTRESULT::InvalidAnswerComment;
            //     return false;
            // }

            return Success;
        }

        tuple<bool, SocialConsensusResult> ValidateLimit(shared_ptr<Transaction> tx, PocketBlock& block) override
        {
            // if (blockVtx.Exists("Comment")) {
            //     for (auto& mtx : blockVtx.Data["Comment"]) {
            //         if (mtx["txid"].get_str() != _txid && mtx["otxid"].get_str() == _otxid) {
            //             result = ANTIBOTRESULT::DoubleCommentDelete;
            //             return false;
            //         }
            //     }
            // }
        }

        tuple<bool, SocialConsensusResult> ValidateLimit(shared_ptr<Transaction> tx) override
        {
            //     reindexer::QueryResults res;
            //     if (g_pocketdb->Select(reindexer::Query("Mempool").Where("table", CondEq, "Comment").Not().Where("txid", CondEq, _txid), res).ok()) {
            //         for (auto& m : res) {
            //             reindexer::Item mItm = m.GetItem();
            //             std::string t_src = DecodeBase64(mItm["data"].As<string>());

            //             reindexer::Item t_itm = g_pocketdb->DB()->NewItem("Comment");
            //             if (t_itm.FromJSON(t_src).ok()) {
            //                 if (t_itm["otxid"].As<string>() == _otxid) {
            //                     result = ANTIBOTRESULT::DoubleCommentDelete;
            //                     return false;
            //                 }
            //             }
            //         }
            //     }
        }

        tuple<bool, SocialConsensusResult> CheckModel(shared_ptr<Transaction> tx) override
        {
            auto ptx = static_pointer_cast<CommentDelete>(tx);
            // TODO (brangr): implement msg must be empty
            return Success;
        }

    };

    /*******************************************************************************************************************
    *
    *  Factory for select actual rules version
    *
    *******************************************************************************************************************/
    class CommentDeleteConsensusFactory
    {
    private:
        static inline const std::map<int, std::function<CommentDeleteConsensus*(int height)>> m_rules =
        {
            {0, [](int height) { return new CommentDeleteConsensus(height); }},
        };
    public:
        shared_ptr <CommentDeleteConsensus> Instance(int height)
        {
            return shared_ptr<CommentDeleteConsensus>(
                (--m_rules.upper_bound(height))->second(height)
            );
        }
    };
}

#endif // POCKETCONSENSUS_COMMENT_DELETE_HPP