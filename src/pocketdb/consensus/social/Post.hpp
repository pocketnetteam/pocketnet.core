// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 Bitcoin developers
// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETCONSENSUS_POST_HPP
#define POCKETCONSENSUS_POST_HPP

#include "pocketdb/consensus/Base.hpp"

namespace PocketConsensus
{
    /*******************************************************************************************************************
    *
    *  Post consensus base class
    *
    *******************************************************************************************************************/
    class PostConsensus : public SocialBaseConsensus
    {
    protected:
    public:
        PostConsensus(int height) : SocialBaseConsensus(height) {}

        tuple<bool, SocialConsensusResult> Validate(PocketBlock& txs) override
        {
            std::string _address = oitm["address"].get_str();
            std::string _txid = oitm["txid"].get_str();
            int64_t _time = oitm["time"].get_int64();

            if (!CheckRegistration(oitm, _address, checkMempool, checkWithTime, height, blockVtx, result)) {
                return false;
            }

            // Compute count of posts for last 24 hours
            int postsCount = g_pocketdb->SelectCount(
                Query("Posts")
                    .Where("address", CondEq, _address)
                    .Where("txidEdit", CondEq, "")
                    .Where("block", CondLt, height)
                    .Where("time", CondGe, _time - 86400)); // TODO (brangr): replace with blocks - this and all time queries

            // Also get posts from history
            postsCount += g_pocketdb->SelectCount(
                Query("PostsHistory")
                    .Where("address", CondEq, _address)
                    .Where("txidEdit", CondEq, "")
                    .Where("block", CondLt, height)
                    .Where("time", CondGe, _time - 86400));

            // Also check mempool
            if (checkMempool) {
                reindexer::QueryResults res;
                if (g_pocketdb->Select(
                                reindexer::Query("Mempool")
                                    .Where("table", CondEq, "Posts")
                                    .Where("txid_source", CondEq, "")
                                    .Not()
                                    .Where("txid", CondEq, _txid),
                                res)
                        .ok()) {
                    for (auto& m : res) {
                        reindexer::Item mItm = m.GetItem();
                        std::string t_src = DecodeBase64(mItm["data"].As<string>());

                        reindexer::Item t_itm = g_pocketdb->DB()->NewItem("Posts");
                        if (t_itm.FromJSON(t_src).ok()) {
                            if (t_itm["address"].As<string>() == _address && t_itm["txidEdit"].As<string>().empty()) {
                                if (!checkWithTime || t_itm["time"].As<int64_t>() <= _time)
                                    postsCount += 1;
                            }
                        }
                    }
                }
            }

            // Check block
            if (blockVtx.Exists("Posts")) {
                for (auto& mtx : blockVtx.Data["Posts"]) {
                    if (mtx["txid"].get_str() != _txid && mtx["address"].get_str() == _address && mtx["txidEdit"].get_str().empty()) {
                        if (!checkWithTime || mtx["time"].get_int64() <= _time)
                            postsCount += 1;
                    }
                }
            }

            // Check limit
            ABMODE mode;
            getMode(_address, mode, height);
            int limit = getLimit(Post, mode, height);
            if (postsCount >= limit) {
                result = ANTIBOTRESULT::PostLimit;
                return false;
            }

            return true;
        }

        bool AntiBot::check_post_edit(const UniValue& oitm, BlockVTX& blockVtx, bool checkMempool, bool checkWithTime, int height, ANTIBOTRESULT& result)
        {
            std::string _address = oitm["address"].get_str();
            std::string _txid = oitm["txid"].get_str();         // Original post id
            std::string _txidEdit = oitm["txidEdit"].get_str(); // new transaction txid
            int64_t _time = oitm["time"].get_int64();

            // User registered?
            if (!CheckRegistration(oitm, _address, checkMempool, checkWithTime, height, blockVtx, result)) {
                return false;
            }

            // Posts exists?
            reindexer::Item _original_post_itm;
            if (!g_pocketdb->SelectOne(Query("Posts").Where("txid", CondEq, _txid).Where("txidEdit", CondEq, "").Where("block", CondLt, height), _original_post_itm).ok()) {
                if (!g_pocketdb->SelectOne(Query("PostsHistory").Where("txid", CondEq, _txid).Where("txidEdit", CondEq, "").Where("block", CondLt, height), _original_post_itm).ok()) {
                    result = ANTIBOTRESULT::NotFound;
                    return false;
                }
            }

            // You are author? Really?
            if (_original_post_itm["address"].As<string>() != _address) {
                result = ANTIBOTRESULT::PostEditUnauthorized;
                return false;
            }

            // Original post edit only 24 hours
            if (_time - _original_post_itm["time"].As<int64_t>() > GetActualLimit(Limit::edit_post_timeout, height)) {
                result = ANTIBOTRESULT::PostEditLimit;
                return false;
            }

            // Double edit in block denied
            if (blockVtx.Exists("Posts")) {
                for (auto& mtx : blockVtx.Data["Posts"]) {
                    if (mtx["txid"].get_str() == _txid && mtx["txidEdit"].get_str() != _txidEdit) {
                        result = ANTIBOTRESULT::DoublePostEdit;
                        return false;
                    }
                }
            }

            // Double edit in mempool denied
            if (checkMempool) {
                if (g_pocketdb->Exists(reindexer::Query("Mempool").Where("table", CondEq, "Posts").Where("txid_source", CondEq, _txid))) {
                    result = ANTIBOTRESULT::DoublePostEdit;
                    return false;
                }
            }

            // Check limit
            {
                size_t edit_count = g_pocketdb->SelectCount(Query("Posts").Where("txid", CondEq, _txid).Not().Where("txidEdit", CondEq, "").Where("block", CondLt, height));
                edit_count += g_pocketdb->SelectCount(Query("PostsHistory").Where("txid", CondEq, _txid).Not().Where("txidEdit", CondEq, "").Where("block", CondLt, height));

                ABMODE mode;
                getMode(_address, mode, height);
                size_t limit = getLimit(PostEdit, mode, height);
                if (edit_count >= limit) {
                    result = ANTIBOTRESULT::PostEditLimit;
                    return false;
                }
            }

            return true;
        }

    };


    /*******************************************************************************************************************
    *
    *  Start checkpoint
    *
    *******************************************************************************************************************/
    class PostConsensus_checkpoint_0 : public PostConsensus
    {
    protected:
    public:

        PostConsensus_checkpoint_0(int height) : PostConsensus(height) {}

    }; // class PostConsensus_checkpoint_0


    /*******************************************************************************************************************
    *
    *  Consensus checkpoint at 1 block
    *
    *******************************************************************************************************************/
    class PostConsensus_checkpoint_1 : public PostConsensus_checkpoint_0
    {
    protected:
        int CheckpointHeight() override { return 1; }
    public:
        PostConsensus_checkpoint_1(int height) : PostConsensus_checkpoint_0(height) {}
    };


    /*******************************************************************************************************************
    *
    *  Factory for select actual rules version
    *  Каждая новая перегрузка добавляет новый функционал, поддерживающийся с некоторым условием - например высота
    *
    *******************************************************************************************************************/
    class PostConsensusFactory
    {
    private:
        inline static std::vector<std::pair<int, std::function<PostConsensus*(int height)>>> m_rules
            {
                {1, [](int height) { return new PostConsensus_checkpoint_1(height); }},
                {0, [](int height) { return new PostConsensus_checkpoint_0(height); }},
            };
    public:
        shared_ptr <PostConsensus> Instance(int height)
        {
            for (const auto& rule : m_rules)
                if (height >= rule.first)
                    return shared_ptr<PostConsensus>(rule.second(height));
        }
    };
}

#endif // POCKETCONSENSUS_POST_HPP