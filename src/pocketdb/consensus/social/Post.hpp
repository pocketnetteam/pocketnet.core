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
    /*******************************************************************************************************************
    *  Post consensus base class
    *******************************************************************************************************************/
    class PostConsensus : public SocialConsensusTest<shared_ptr<Post>>
    {
    public:
        PostConsensus(int height) : SocialConsensusTest<shared_ptr<Post>>(height) {}

        ConsensusValidateResult Validate(const shared_ptr<Post>& ptx, const PocketBlockRef& block) override
        {
            // Base validation with calling block or mempool check
            if (auto[baseValidate, baseValidateCode] = SocialConsensusTest::Validate(ptx, block); !baseValidate)
                return {false, baseValidateCode};

            if (ptx->IsEdit())
                return ValidateEdit(ptx);

            return Success;
        }

        ConsensusValidateResult Check(const CTransactionRef& tx, const shared_ptr<Post>& ptx) override
        {
            if (auto[baseCheck, baseCheckCode] = SocialConsensusTest::Check(tx, ptx); !baseCheck)
                return {false, baseCheckCode};

            // Check required fields
            if (IsEmpty(ptx->GetString1())) return {false, SocialConsensusResult_Failed};

            return Success;
        }

    protected:
        virtual int64_t GetLimitWindow() { return 86400; }
        virtual int64_t GetEditWindow() { return 86400; }
        virtual int64_t GetFullLimit() { return 30; }
        virtual int64_t GetTrialLimit() { return 15; }
        virtual int64_t GetEditLimit() { return 5; }
        virtual int64_t GetLimit(AccountMode mode) { return mode >= AccountMode_Full ? GetFullLimit() : GetTrialLimit(); }


        virtual ConsensusValidateResult ValidateEdit(const shared_ptr<Post>& ptx)
        {
            // First get original post transaction
            auto originalTx = PocketDb::TransRepoInst.GetByHash(*ptx->GetRootTxHash());
            if (!originalTx)
                return {false, SocialConsensusResult_NotFound};

            const auto originalPtx = static_pointer_cast<Post>(originalTx);

            // Change type not allowed
            if (*originalTx->GetType() != *ptx->GetType())
                return {false, SocialConsensusResult_NotAllowed};

            // You are author? Really?
            if (*ptx->GetAddress() != *originalTx->GetString1())
                return {false, SocialConsensusResult_ContentEditUnauthorized};

            // Original post edit only 24 hours
            if (!AllowEditWindow(ptx, originalPtx))
                return {false, SocialConsensusResult_ContentEditLimit};

            // Check edit limit
            return ValidateEditOneLimit(ptx);
        }

        ConsensusValidateResult ValidateBlock(const shared_ptr<Post>& ptx, const PocketBlockRef& block) override
        {
            // Edit posts
            if (ptx->IsEdit())
                return ValidateEditBlock(ptx, block);

            // ---------------------------------------------------------
            // New posts

            // Get count from chain
            int count = GetChainCount(ptx);

            // Get count from block
            for (const auto& blockTx : *block)
            {
                if (!IsIn(*blockTx->GetType(), {CONTENT_POST}))
                    continue;

                const auto blockPtx = static_pointer_cast<Post>(blockTx);

                //if (*blockTx->GetHash() == *ptx->GetHash())
                //    continue;

                LogPrintf("5: %s\n", *ptx->GetAddress());
                LogPrintf("51: %s\n", *ptx->GetString1());
                LogPrintf("52: %s\n", *blockTx->GetString1());
                if (*ptx->GetString1() == *blockTx->GetString1())
                {
                    if (AllowBlockLimitTime(ptx, blockPtx))
                        count += 1;
                }
            }

            return ValidateLimit(ptx, count);
        }

        ConsensusValidateResult ValidateMempool(const shared_ptr<Post>& ptx) override
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

        virtual ConsensusValidateResult ValidateLimit(const shared_ptr<Post>& ptx, int count)
        {
            auto reputationConsensus = PocketConsensus::ReputationConsensusFactoryInst.Instance(Height);
            auto[mode, reputation, balance] = reputationConsensus->GetAccountInfo(*ptx->GetString1());
            if (count >= GetLimit(mode))
                return {false, SocialConsensusResult_ContentLimit};

            return Success;
        }

        virtual bool AllowBlockLimitTime(const shared_ptr<Post>& ptx, const shared_ptr<Post>& blockPtx)
        {
            return *blockPtx->GetTime() <= *ptx->GetTime();
        }

        virtual bool AllowEditWindow(const shared_ptr<Post>& ptx, const shared_ptr<Post>& originalTx)
        {
            return (*ptx->GetTime() - *originalTx->GetTime()) <= GetEditWindow();
        }

        virtual int GetChainCount(const shared_ptr<Post>& ptx)
        {
            return ConsensusRepoInst.CountChainPostTime(
                *ptx->GetString1(),
                *ptx->GetTime() - GetLimitWindow()
            );
        }

        virtual ConsensusValidateResult ValidateEditBlock(const shared_ptr<Post>& ptx, const PocketBlockRef& block)
        {
            // Double edit in block not allowed
            for (auto& blockTx : *block)
            {
                if (!IsIn(*blockTx->GetType(), {CONTENT_POST}))
                    continue;

                if (*blockTx->GetHash() == *ptx->GetHash())
                    continue;

                if (*ptx->GetString2() == *blockTx->GetString2())
                    return {false, SocialConsensusResult_DoubleContentEdit};
            }

            // Check edit limit
            return ValidateEditOneLimit(ptx);
        }

        virtual ConsensusValidateResult ValidateEditMempool(const shared_ptr<Post>& ptx)
        {
            if (ConsensusRepoInst.CountMempoolPostEdit(*ptx->GetString1(), *ptx->GetString2()) > 0)
                return {false, SocialConsensusResult_DoubleContentEdit};

            // Check edit limit
            return ValidateEditOneLimit(ptx);
        }

        virtual ConsensusValidateResult ValidateEditOneLimit(const shared_ptr<Post>& ptx)
        {
            int count = ConsensusRepoInst.CountChainPostEdit(*ptx->GetString1(), *ptx->GetString2());
            if (count >= GetEditLimit())
                return {false, SocialConsensusResult_ContentEditLimit};

            return Success;
        }

        vector<string> GetAddressesForCheckRegistration(const shared_ptr<Post>& ptx) override
        {
            return {*ptx->GetString1()};
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
        bool AllowBlockLimitTime(const shared_ptr<Post>& ptx, const shared_ptr<Post>& blockPtx) override
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
            return ConsensusRepoInst.CountChainPostHeight(*ptx->GetString1(), Height - (int) GetLimitWindow());
        }
        bool AllowEditWindow(const shared_ptr<Post>& ptx, const shared_ptr<Post>& originalTx) override
        {
            auto[ok, originalTxHeight] = ConsensusRepoInst.GetTransactionHeight(*originalTx->GetHash());
            if (!ok)
                return false;

            return (Height - originalTxHeight) <= GetEditWindow();
        }
    };

    /*******************************************************************************************************************
    *  Start checkpoint at 1324655 block
    *******************************************************************************************************************/
    class PostConsensus_checkpoint_1324655 : public PostConsensus_checkpoint_1180000
    {
    public:
        PostConsensus_checkpoint_1324655(int height) : PostConsensus_checkpoint_1180000(height) {}
    protected:
        int64_t GetTrialLimit() override { return 5; }
    };

    /*******************************************************************************************************************
    *  Factory for select actual rules version
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

    class PostConsensusFactoryTest : public ConsensusFactoryTest<PostConsensus>
    {
    protected:
        vector<ConsensusCheckpointTest<PostConsensus>> m_rules() override
        {
            return
            {
                {0,       -1, [](int height) { return make_shared<PostConsensus>(height); }},
                {1124000, -1, [](int height) { return make_shared<PostConsensus_checkpoint_1124000>(height); }},
                {1180000, -1, [](int height) { return make_shared<PostConsensus_checkpoint_1180000>(height); }},
                {1324655, 0,  [](int height) { return make_shared<PostConsensus_checkpoint_1324655>(height); }},
            };
        }
    };
} // namespace PocketConsensus

#endif // POCKETCONSENSUS_POST_HPP