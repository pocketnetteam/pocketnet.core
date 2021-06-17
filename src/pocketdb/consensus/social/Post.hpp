// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 Bitcoin developers
// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETCONSENSUS_POST_HPP
#define POCKETCONSENSUS_POST_HPP

#include "pocketdb/consensus/social/Base.hpp"
#include "pocketdb/models/dto/Post.hpp"

namespace PocketConsensus
{
    /*******************************************************************************************************************
    *
    *  Post consensus base class
    *
    *******************************************************************************************************************/
    class PostConsensus : public SocialBaseConsensus
    {
    public:
        PostConsensus(int height) : SocialBaseConsensus(height) {}
        PostConsensus() : SocialBaseConsensus() {}

    protected:

        virtual int64_t GetEditPostWindow() { return 86400; }


        tuple<bool, SocialConsensusResult> ValidateModel(shared_ptr <Transaction> tx) override
        {
            auto ptx = static_pointer_cast<Post>(tx);

            vector<string> addresses = {*ptx->GetAddress()};
            if (!PocketDb::ConsensusRepoInst.ExistsUserRegistrations(addresses))
                return {false, SocialConsensusResult_NotRegistered};

            if (ptx->IsEdit())
                return ValidateEditModel(ptx);

            return Success;
        }

        virtual tuple<bool, SocialConsensusResult> ValidateEditModel(shared_ptr <Post> tx)
        {
            // First get original post transaction
            auto originalTx = PocketDb::TransRepoInst.GetById(*tx->GetRootTxHash());
            if (!originalTx || !originalTx->GetHeight())
                return {false, SocialConsensusResult_NotFound};

            auto originalPostTx = static_pointer_cast<Post>(originalTx);

            // You are author? Really?
            if (*tx->GetAddress() != *originalPostTx->GetAddress())
                return {false, SocialConsensusResult_PostEditUnauthorized};

            // Original post edit only 24 hours
            if ((*tx->GetTime() - *originalPostTx->GetTime()) > GetEditPostWindow())
                return {false, SocialConsensusResult_PostEditLimit};

            // Check limit
            // {
            //     size_t edit_count = g_pocketdb->SelectCount(Query("Posts").Where("txid", CondEq, _txid).Not().Where("txidEdit", CondEq, "").Where("block", CondLt, height));
            //     edit_count += g_pocketdb->SelectCount(Query("PostsHistory").Where("txid", CondEq, _txid).Not().Where("txidEdit", CondEq, "").Where("block", CondLt, height));

            //     ABMODE mode;
            //     getMode(_address, mode, height);
            //     size_t limit = getLimit(PostEdit, mode, height);
            //     if (edit_count >= limit) {
            //         result = ANTIBOTRESULT::PostEditLimit;
            //         return false;
            //     }
            // }

            return make_tuple(true, SocialConsensusResult_Success);
        }


        tuple<bool, SocialConsensusResult> ValidateLimit(shared_ptr <Transaction> tx, PocketBlock& block) override
        {
            auto ptx = static_pointer_cast<Post>(tx);

            if (ptx->IsEdit())
                return ValidateEditLimit(ptx, block);

            // ----------------------------------

            // // Compute count of posts for last 24 hours
            // int postsCount = g_pocketdb->SelectCount(
            //     Query("Posts")
            //         .Where("address", CondEq, _address)
            //         .Where("txidEdit", CondEq, "")
            //         .Where("block", CondLt, height)
            //         .Where("time", CondGe, _time - 86400)); // TODO (brangr): replace with blocks - this and all time queries

            // // Also get posts from history
            // postsCount += g_pocketdb->SelectCount(
            //     Query("PostsHistory")
            //         .Where("address", CondEq, _address)
            //         .Where("txidEdit", CondEq, "")
            //         .Where("block", CondLt, height)
            //         .Where("time", CondGe, _time - 86400));


            // if (blockVtx.Exists("Posts")) {
            //     for (auto& mtx : blockVtx.Data["Posts"]) {
            //         if (mtx["txid"].get_str() != _txid && mtx["address"].get_str() == _address && mtx["txidEdit"].get_str().empty()) {
            //             if (!checkWithTime || mtx["time"].get_int64() <= _time)
            //                 postsCount += 1;
            //         }
            //     }
            // }



            // ABMODE mode;
            // getMode(_address, mode, height);
            // int limit = getLimit(Post, mode, height);
            // if (postsCount >= limit) {
            //     result = ANTIBOTRESULT::PostLimit;
            //     return false;
            // }
        }

        virtual tuple<bool, SocialConsensusResult> ValidateEditLimit(shared_ptr <Post> tx, PocketBlock& block)
        {
            // if (blockVtx.Exists("Posts")) {
            //     for (auto& mtx : blockVtx.Data["Posts"]) {
            //         if (mtx["txid"].get_str() == _txid && mtx["txidEdit"].get_str() != _txidEdit) {
            //             result = ANTIBOTRESULT::DoublePostEdit;
            //             return false;
            //         }
            //     }
            // }
        }


        tuple<bool, SocialConsensusResult> ValidateLimit(shared_ptr <Transaction> tx) override
        {
            auto ptx = static_pointer_cast<Post>(tx);

            if (ptx->IsEdit())
                return ValidateEditLimit(ptx);

            // ---------------------------------

            //     reindexer::QueryResults res;
            //     if (g_pocketdb->Select(
            //                     reindexer::Query("Mempool")
            //                         .Where("table", CondEq, "Posts")
            //                         .Where("txid_source", CondEq, "")
            //                         .Not()
            //                         .Where("txid", CondEq, _txid),
            //                     res)
            //             .ok()) {
            //         for (auto& m : res) {
            //             reindexer::Item mItm = m.GetItem();
            //             std::string t_src = DecodeBase64(mItm["data"].As<string>());

            //             reindexer::Item t_itm = g_pocketdb->DB()->NewItem("Posts");
            //             if (t_itm.FromJSON(t_src).ok()) {
            //                 if (t_itm["address"].As<string>() == _address && t_itm["txidEdit"].As<string>().empty()) {
            //                     if (!checkWithTime || t_itm["time"].As<int64_t>() <= _time)
            //                         postsCount += 1;
            //                 }
            //             }
            //         }
            //     }



            // ABMODE mode;
            // getMode(_address, mode, height);
            // int limit = getLimit(Post, mode, height);
            // if (postsCount >= limit) {
            //     result = ANTIBOTRESULT::PostLimit;
            //     return false;
            // }
        }

        virtual tuple<bool, SocialConsensusResult> ValidateEditLimit(shared_ptr <Post> tx)
        {
            //     if (g_pocketdb->Exists(reindexer::Query("Mempool").Where("table", CondEq, "Posts").Where("txid_source", CondEq, _txid))) {
            //         result = ANTIBOTRESULT::DoublePostEdit;
            //         return false;
            //     }
        }


        tuple<bool, SocialConsensusResult> CheckModel(shared_ptr <Transaction> tx) override
        {
            return Success;
        }

    };

    /*******************************************************************************************************************
    *
    *  Consensus checkpoint at ? block
    *
    *******************************************************************************************************************/
    // TODO (brangr): change time to block height
    class PostConsensus_checkpoint_ : public PostConsensus
    {
    protected:
        int CheckpointHeight() override { return 0; }
    public:
        PostConsensus_checkpoint_(int height) : PostConsensus(height) {}
    };


    /*******************************************************************************************************************
    *
    *  Factory for select actual rules version
    *
    *******************************************************************************************************************/
    class PostConsensusFactory
    {
    private:
        const std::map<int, std::function<PostConsensus*(int height)>> m_rules =
            {
                {0, [](int height) { return new PostConsensus(height); }},
            };
    public:
        shared_ptr <PostConsensus> Instance(int height)
        {
            return shared_ptr<PostConsensus>(
                (--m_rules.upper_bound(height))->second(height)
            );
        }
    };
}

#endif // POCKETCONSENSUS_POST_HPP