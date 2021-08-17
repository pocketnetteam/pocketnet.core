// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 Bitcoin developers
// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETCONSENSUS_POST_HPP
#define POCKETCONSENSUS_POST_HPP

#include "pocketdb/consensus/Reputation.hpp"
#include "pocketdb/consensus/social/Base.hpp"
#include "pocketdb/models/dto/Post.hpp"

namespace PocketConsensus
{
    using namespace std;

    /*******************************************************************************************************************
    *
    *  Post consensus base class
    *
    *******************************************************************************************************************/
    class PostConsensus : public SocialBaseConsensus
    {
    public:
        PostConsensus(int height) : SocialBaseConsensus(height) {}

    protected:

        virtual int64_t GetLimitWindow() { return 86400; }

        virtual int64_t GetEditWindow() { return 86400; }

        virtual int64_t GetFullLimit() { return 30; }

        virtual int64_t GetFullEditLimit() { return 5; }

        virtual int64_t GetTrialLimit() { return 15; }

        virtual int64_t GetTrialEditLimit() { return 5; }

        virtual int64_t GetLimit(AccountMode mode)
        {
            return mode >= AccountMode_Full ? GetFullLimit() : GetTrialLimit();
        }

        virtual int64_t GetEditLimit(AccountMode mode)
        {
            return mode >= AccountMode_Full ? GetFullEditLimit() : GetTrialEditLimit();
        }

        // ------------------------------------------------------------------------------------------------------------

        tuple<bool, SocialConsensusResult> ValidateModel(const shared_ptr<Transaction>& tx) override
        {
            auto ptx = static_pointer_cast<Post>(tx);

            if (ptx->IsEdit())
                return ValidateEditModel(ptx);

            return Success;
        }

        virtual tuple<bool, SocialConsensusResult> ValidateEditModel(const shared_ptr<Post>& tx)
        {
            // First get original post transaction
            auto originalTx = PocketDb::TransRepoInst.GetByHash(*tx->GetRootTxHash());
            if (!originalTx)
                return {false, SocialConsensusResult_NotFound};

            // Change type not allowed
            if (*originalTx->GetType() != *tx->GetType())
                return {false, SocialConsensusResult_NotAllowed};

            // Cast tx to Post for next checks
            auto originalPostTx = static_pointer_cast<Post>(originalTx);

            // You are author? Really?
            if (*tx->GetAddress() != *originalPostTx->GetAddress())
                return {false, SocialConsensusResult_ContentEditUnauthorized};

            // Original post edit only 24 hours
            if (!AllowEditWindow(tx, originalPostTx))
                return {false, SocialConsensusResult_ContentEditLimit};

            return make_tuple(true, SocialConsensusResult_Success);
        }

        // ------------------------------------------------------------------------------------------------------------

        virtual bool AllowBlockLimitTime(const PTransactionRef& ptx, const PTransactionRef& blockPtx)
        {
            return *blockPtx->GetTime() <= *ptx->GetTime();
        }

        virtual bool AllowEditWindow(const PTransactionRef& ptx, const PTransactionRef& originalTx)
        {
            return (*ptx->GetTime() - *originalTx->GetTime()) <= GetEditWindow();
        }

        // ------------------------------------------------------------------------------------------------------------

        tuple<bool, SocialConsensusResult> ValidateLimit(const PTransactionRef& tx,
                                                         const PocketBlock& block) override
        {
            auto ptx = static_pointer_cast<Post>(tx);

            // ---------------------------------------------------------
            // Edit posts
            if (ptx->IsEdit())
                return ValidateEditLimit(ptx, block);

            // ---------------------------------------------------------
            // New posts

            // Get count from chain
            int count = GetChainCount(ptx);

            // Get count from block
            for (auto& blockTx : block)
            {
                if (!IsIn(*blockTx->GetType(), {CONTENT_POST}))
                    continue;

                if (*blockTx->GetHash() == *ptx->GetHash())
                    continue;

                auto blockPtx = static_pointer_cast<Post>(blockTx);
                if (*ptx->GetAddress() == *blockPtx->GetAddress())
                {
                    if (AllowBlockLimitTime(tx, blockTx))
                        count += 1;
                }
            }

            return ValidateLimit(ptx, count);
        }

        tuple<bool, SocialConsensusResult> ValidateLimit(const shared_ptr<Transaction>& tx) override
        {
            auto ptx = static_pointer_cast<Post>(tx);

            // ---------------------------------------------------------
            // Edit posts
            if (ptx->IsEdit())
                return ValidateEditLimit(ptx);

            // ---------------------------------------------------------
            // New posts

            // Get count from chain
            int count = GetChainCount(ptx);

            // Get count from mempool
            count += ConsensusRepoInst.CountMempoolPost(*ptx->GetAddress());

            return ValidateLimit(ptx, count);
        }

        virtual tuple<bool, SocialConsensusResult> ValidateLimit(const shared_ptr<Post>& tx, int count)
        {
            ReputationConsensusFactory reputationConsensusFactoryInst;
            auto reputationConsensus = reputationConsensusFactoryInst.Instance(Height);
            auto[mode, reputation, balance] = reputationConsensus->GetAccountInfo(*tx->GetAddress());
            auto limit = GetLimit(mode);

            if (count >= limit)
                return {false, SocialConsensusResult_ContentLimit};

            return Success;
        }

        virtual int GetChainCount(const shared_ptr<Post>& ptx)
        {
            return ConsensusRepoInst.CountChainPostTime(
                *ptx->GetAddress(),
                *ptx->GetTime() - GetLimitWindow()
            );
        }

        // ------------------------------------------------------------------------------------------------------------

        virtual tuple<bool, SocialConsensusResult> ValidateEditLimit(const shared_ptr<Post>& tx,
                                                                     const PocketBlock& block)
        {
            // Double edit in block not allowed
            for (auto& blockTx : block)
            {
                if (!IsIn(*blockTx->GetType(), {CONTENT_POST}))
                    continue;

                if (*blockTx->GetHash() == *tx->GetHash())
                    continue;

                auto blockPtx = static_pointer_cast<Post>(blockTx);
                if (*tx->GetRootTxHash() == *blockPtx->GetRootTxHash())
                    return {false, SocialConsensusResult_DoubleContentEdit};
            }

            // Check edit limit
            return ValidateEditOneLimit(tx);
        }

        virtual tuple<bool, SocialConsensusResult> ValidateEditLimit(const shared_ptr<Post>& tx)
        {
            if (ConsensusRepoInst.CountMempoolPostEdit(*tx->GetAddress(), *tx->GetRootTxHash()) > 0)
                return {false, SocialConsensusResult_DoubleContentEdit};

            // Check edit limit
            return ValidateEditOneLimit(tx);
        }

        virtual tuple<bool, SocialConsensusResult> ValidateEditOneLimit(const shared_ptr<Post>& tx)
        {
            int count = ConsensusRepoInst.CountChainPostEdit(*tx->GetAddress(), *tx->GetRootTxHash());

            ReputationConsensusFactory reputationConsensusFactoryInst;
            auto reputationConsensus = reputationConsensusFactoryInst.Instance(Height);
            auto[mode, reputation, balance] = reputationConsensus->GetAccountInfo(*tx->GetAddress());
            auto limit = GetEditLimit(mode);

            if (count >= limit)
                return {false, SocialConsensusResult_ContentEditLimit};

            return Success;
        }

        tuple<bool, SocialConsensusResult> CheckModel(const shared_ptr<Transaction>& tx) override
        {
            auto ptx = static_pointer_cast<Post>(tx);

            // Check required fields
            if (IsEmpty(ptx->GetAddress())) return {false, SocialConsensusResult_Failed};

            return Success;
        }

        vector<string> GetAddressesForCheckRegistration(const PTransactionRef& tx) override
        {
            auto ptx = static_pointer_cast<Post>(tx);
            return {*ptx->GetAddress()};
        }
    };

    /*******************************************************************************************************************
    *
    *  Start checkpoint at 1124000 block
    *
    *******************************************************************************************************************/
    class PostConsensus_checkpoint_1124000 : public PostConsensus
    {
    public:
        PostConsensus_checkpoint_1124000(int height) : PostConsensus(height) {}

    protected:

        bool AllowBlockLimitTime(const PTransactionRef& ptx, const PTransactionRef& blockPtx) override
        {
            return true;
        }
    };

    /*******************************************************************************************************************
    *
    *  Start checkpoint at 1180000 block
    *
    *******************************************************************************************************************/
    class PostConsensus_checkpoint_1180000 : public PostConsensus_checkpoint_1124000
    {
    public:
        PostConsensus_checkpoint_1180000(int height) : PostConsensus_checkpoint_1124000(height) {}

    protected:

        int64_t GetEditWindow() override { return 1440; }

        int64_t GetLimitWindow() override { return 1440; }


        int GetChainCount(const shared_ptr<Post>& ptx) override
        {
            return ConsensusRepoInst.CountChainPostHeight(
                *ptx->GetAddress(),
                Height - (int) GetLimitWindow()
            );
        }

        bool AllowEditWindow(const PTransactionRef& ptx, const PTransactionRef& originalTx) override
        {
            auto[ok, originalTxHeight] = ConsensusRepoInst.GetTransactionHeight(*originalTx->GetHash());
            if (!ok)
                return false;

            return (Height - originalTxHeight) <= GetEditWindow();
        }
    };

    /*******************************************************************************************************************
    *
    *  Start checkpoint at 1324655 block
    *
    *******************************************************************************************************************/
    class PostConsensus_checkpoint_1324655 : public PostConsensus_checkpoint_1180000
    {
    public:
        PostConsensus_checkpoint_1324655(int height) : PostConsensus_checkpoint_1180000(height) {}
    protected:
        int64_t GetTrialLimit() override { return 5; }
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
                {1324655, [](int height) { return new PostConsensus_checkpoint_1324655(height); }},
                {1180000, [](int height) { return new PostConsensus_checkpoint_1180000(height); }},
                {1124000, [](int height) { return new PostConsensus_checkpoint_1124000(height); }},
                {0,       [](int height) { return new PostConsensus(height); }},
            };

    public:
        shared_ptr<PostConsensus> Instance(int height)
        {
            return shared_ptr<PostConsensus>(
                (--m_rules.upper_bound(height))->second(height));
        }
    };
} // namespace PocketConsensus

#endif // POCKETCONSENSUS_POST_HPP