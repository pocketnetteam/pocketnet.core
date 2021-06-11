// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 Bitcoin developers
// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETCONSENSUS_COMMENTDELETE_HPP
#define POCKETCONSENSUS_COMMENTDELETE_HPP

#include "pocketdb/consensus/Base.hpp"

namespace PocketConsensus
{
    /*******************************************************************************************************************
    *
    *  Comment consensus base class
    *
    *******************************************************************************************************************/
    class CommentDeleteConsensus : public SocialBaseConsensus
    {
    protected:
    public:
        CommentDeleteConsensus(int height) : SocialBaseConsensus(height) {}

        tuple<bool, SocialConsensusResult> Validate(shared_ptr<Blocking> tx, PocketBlock& block) override
        {
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

            // // Double delete in block denied
            // if (blockVtx.Exists("Comment")) {
            //     for (auto& mtx : blockVtx.Data["Comment"]) {
            //         if (mtx["txid"].get_str() != _txid && mtx["otxid"].get_str() == _otxid) {
            //             result = ANTIBOTRESULT::DoubleCommentDelete;
            //             return false;
            //         }
            //     }
            // }

            // // Double delete in mempool denied
            // if (checkMempool) {
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
            // }

            // return true;
        }


    };


    /*******************************************************************************************************************
    *
    *  Start checkpoint
    *
    *******************************************************************************************************************/
    class CommentDeleteConsensus_checkpoint_0 : public CommentDeleteConsensus
    {
    protected:
    public:

        CommentDeleteConsensus_checkpoint_0(int height) : CommentDeleteConsensus(height) {}

    }; // class CommentDeleteConsensus_checkpoint_0


    /*******************************************************************************************************************
    *
    *  Consensus checkpoint at 1 block
    *
    *******************************************************************************************************************/
    class CommentDeleteConsensus_checkpoint_1 : public CommentDeleteConsensus_checkpoint_0
    {
    protected:
        int CheckpointHeight() override { return 1; }
    public:
        CommentDeleteConsensus_checkpoint_1(int height) : CommentDeleteConsensus_checkpoint_0(height) {}
    };


    /*******************************************************************************************************************
    *
    *  Factory for select actual rules version
    *  Каждая новая перегрузка добавляет новый функционал, поддерживающийся с некоторым условием - например высота
    *
    *******************************************************************************************************************/
    class CommentDeleteConsensusFactory
    {
    private:
        inline static std::vector<std::pair<int, std::function<CommentDeleteConsensus*(int height)>>> m_rules
            {
                {1, [](int height) { return new CommentDeleteConsensus_checkpoint_1(height); }},
                {0, [](int height) { return new CommentDeleteConsensus_checkpoint_0(height); }},
            };
    public:
        shared_ptr <CommentDeleteConsensus> Instance(int height)
        {
            for (const auto& rule : m_rules)
                if (height >= rule.first)
                    return shared_ptr<CommentDeleteConsensus>(rule.second(height));
        }
    };
}

#endif // POCKETCONSENSUS_COMMENTDELETE_HPP