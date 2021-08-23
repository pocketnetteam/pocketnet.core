// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETCONSENSUS_POST_HPP
#define POCKETCONSENSUS_POST_HPP

#include "pocketdb/consensus/Reputation.hpp"
#include "pocketdb/consensus/Social.hpp"
#include "pocketdb/models/dto/Post.hpp"

namespace PocketConsensus
{
    typedef shared_ptr<Post> PostRef;

    /*******************************************************************************************************************
    *
    *  Post consensus base class
    *
    *******************************************************************************************************************/
    class PostConsensus : public SocialConsensus<PostRef>
    {
    public:
        PostConsensus(int height) : SocialConsensus(height) {}

        ConsensusValidateResult Validate(const PostRef& ptx, const PocketBlockRef& block) override
        {
            // Base validation with calling block or mempool check
            if (auto[baseValidate, baseValidateCode] = SocialConsensus::Validate(ptx, block); !baseValidate)
                return {false, baseValidateCode};

            if (ptx->IsEdit())
                return ValidateEdit(ptx);

            return Success;
        }

        ConsensusValidateResult Check(const PostRef& ptx) override
        {
            // Check required fields
            if (IsEmpty(ptx->GetAddress())) return {false, SocialConsensusResult_Failed};

            return Success;
        }

    protected:
        virtual int64_t GetLimitWindow() { return 86400; }
        virtual int64_t GetEditWindow() { return 86400; }
        virtual int64_t GetFullLimit() { return 30; }
        virtual int64_t GetTrialLimit() { return 15; }
        virtual int64_t GetEditLimit() { return 5; }
        virtual int64_t GetLimit(AccountMode mode) { return mode >= AccountMode_Full ? GetFullLimit() : GetTrialLimit(); }


        virtual ConsensusValidateResult ValidateEdit(const PostRef& ptx)
        {
            // First get original post transaction
            auto originalTx = PocketDb::TransRepoInst.GetByHash(*ptx->GetRootTxHash());
            if (!originalTx)
                return {false, SocialConsensusResult_NotFound};

            // Change type not allowed
            if (*originalTx->GetType() != *ptx->GetType())
                return {false, SocialConsensusResult_NotAllowed};

            // Cast tx to Post for next checks
            auto originalPostTx = static_pointer_cast<Post>(originalTx);

            // You are author? Really?
            if (*ptx->GetAddress() != *originalPostTx->GetAddress())
                return {false, SocialConsensusResult_ContentEditUnauthorized};

            // Original post edit only 24 hours
            if (!AllowEditWindow(ptx, originalPostTx))
                return {false, SocialConsensusResult_ContentEditLimit};
                
            // Check edit limit
            return ValidateEditOneLimit(ptx);
        }

        ConsensusValidateResult ValidateBlock(const PostRef& ptx, const PocketBlockRef& block) override
        {
            // Edit posts
            if (ptx->IsEdit())
                return ValidateEditBlock(ptx, block);

            // ---------------------------------------------------------
            // New posts

            // Get count from chain
            int count = GetChainCount(ptx);

            // Get count from block
            for (auto& blockTx : *block)
            {
                if (!IsIn(*blockTx->GetType(), {CONTENT_POST}))
                    continue;

                if (*blockTx->GetHash() == *ptx->GetHash())
                    continue;

                auto blockPtx = static_pointer_cast<Post>(blockTx);
                if (*ptx->GetAddress() == *blockPtx->GetAddress())
                {
                    if (AllowBlockLimitTime(ptx, blockPtx))
                        count += 1;
                }
            }

            return ValidateLimit(ptx, count);
        }

        ConsensusValidateResult ValidateMempool(const PostRef& ptx) override
        {
            // Edit posts
            if (ptx->IsEdit())
                return ValidateEditMempool(ptx);

            // ---------------------------------------------------------
            // New posts

            // Get count from chain
            int count = GetChainCount(ptx);

            // Get count from mempool
            count += ConsensusRepoInst.CountMempoolPost(*ptx->GetAddress());

            return ValidateLimit(ptx, count);
        }

        virtual ConsensusValidateResult ValidateLimit(const PostRef& tx, int count)
        {
            auto reputationConsensus = PocketConsensus::ReputationConsensusFactoryInst.Instance(Height);

            auto[mode, reputation, balance] = reputationConsensus->GetAccountInfo(*tx->GetAddress());
            auto limit = GetLimit(mode);

            if (count >= limit)
                return {false, SocialConsensusResult_ContentLimit};

            return Success;
        }
        
        virtual bool AllowBlockLimitTime(const PostRef& ptx, const PostRef& blockPtx)
        {
            return *blockPtx->GetTime() <= *ptx->GetTime();
        }

        virtual bool AllowEditWindow(const PostRef& ptx, const PostRef& originalTx)
        {
            return (*ptx->GetTime() - *originalTx->GetTime()) <= GetEditWindow();
        }

        virtual int GetChainCount(const PostRef& ptx)
        {
            return ConsensusRepoInst.CountChainPostTime(
                *ptx->GetAddress(),
                *ptx->GetTime() - GetLimitWindow()
            );
        }

        virtual ConsensusValidateResult ValidateEditBlock(const PostRef& ptx, const PocketBlockRef& block)
        {
            // Double edit in block not allowed
            for (auto& blockTx : *block)
            {
                if (!IsIn(*blockTx->GetType(), {CONTENT_POST}))
                    continue;

                if (*blockTx->GetHash() == *ptx->GetHash())
                    continue;

                auto blockPtx = static_pointer_cast<Post>(blockTx);
                if (*ptx->GetRootTxHash() == *blockPtx->GetRootTxHash())
                    return {false, SocialConsensusResult_DoubleContentEdit};
            }
            
            // Check edit limit
            return ValidateEditOneLimit(ptx);
        }

        virtual ConsensusValidateResult ValidateEditMempool(const PostRef& tx)
        {
            if (ConsensusRepoInst.CountMempoolPostEdit(*tx->GetAddress(), *tx->GetRootTxHash()) > 0)
                return {false, SocialConsensusResult_DoubleContentEdit};

            // Check edit limit
            return ValidateEditOneLimit(tx);
        }

        virtual ConsensusValidateResult ValidateEditOneLimit(const PostRef& tx)
        {
            int count = ConsensusRepoInst.CountChainPostEdit(*tx->GetAddress(), *tx->GetRootTxHash());
            if (count >= GetEditLimit())
                return {false, SocialConsensusResult_ContentEditLimit};

            return Success;
        }

        vector<string> GetAddressesForCheckRegistration(const PostRef& tx) override
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

        bool AllowBlockLimitTime(const PostRef& ptx, const PostRef& blockPtx) override
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

        bool AllowEditWindow(const PostRef& ptx, const PostRef& originalTx) override
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
    class PostConsensusFactory : public SocialConsensusFactory
    {
    private:
        const vector<ConsensusCheckpoint> _rules = {
            {0,       -1, [](int height) { return make_shared<PostConsensus>(height); }},
            {1124000, -1, [](int height) { return make_shared<PostConsensus_checkpoint_1124000>(height); }},
            {1180000, -1, [](int height) { return make_shared<PostConsensus_checkpoint_1180000>(height); }},
            {1324655, 0,  [](int height) { return make_shared<PostConsensus_checkpoint_1324655>(height); }},
        };
    protected:
        const vector<ConsensusCheckpoint>& m_rules() override { return _rules; }
    };
} // namespace PocketConsensus

#endif // POCKETCONSENSUS_POST_HPP