// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETCONSENSUS_VIDEO_HPP
#define POCKETCONSENSUS_VIDEO_HPP

#include "pocketdb/consensus/Reputation.hpp"
#include "pocketdb/consensus/Social.hpp"
#include "pocketdb/models/dto/Video.hpp"

namespace PocketConsensus
{
    /*******************************************************************************************************************
    *  Video consensus base class
    *******************************************************************************************************************/
    class VideoConsensus : public SocialConsensus
    {
    public:
        VideoConsensus(int height) : SocialConsensus(height) {}

        ConsensusValidateResult Validate(const PTransactionRef& ptx, const PocketBlockRef& block) override
        {
            auto _ptx = static_pointer_cast<Video>(ptx);

            // Base validation with calling block or mempool check
            if (auto[baseValidate, baseValidateCode] = SocialConsensus::Validate(_ptx, block); !baseValidate)
                return {false, baseValidateCode};

            if (_ptx->IsEdit())
                return ValidateEdit(ptx);

            return Success;
        }

        ConsensusValidateResult Check(const CTransactionRef& tx, const PTransactionRef& ptx) override
        {
            auto _ptx = static_pointer_cast<Video>(ptx);

            if (auto[baseCheck, baseCheckCode] = SocialConsensus::Check(tx, _ptx); !baseCheck)
                return {false, baseCheckCode};

            // Check required fields
            if (IsEmpty(_ptx->GetAddress())) return {false, SocialConsensusResult_Failed};

            // Repost not allowed
            if (!IsEmpty(_ptx->GetRelayTxHash())) return {false, SocialConsensusResult_NotAllowed};

            return Success;
        }

    protected:
        virtual int GetLimitWindow() { return 1440; }
        virtual int64_t GetEditWindow() { return 1440; }
        virtual int64_t GetProLimit() { return 100; }
        virtual int64_t GetFullLimit() { return 30; }
        virtual int64_t GetTrialLimit() { return 15; }
        virtual int64_t GetEditLimit() { return 5; }
        virtual int64_t GetLimit(AccountMode mode)
        {
            return mode == AccountMode_Pro
                   ? GetProLimit()
                   : mode == AccountMode_Full
                     ? GetFullLimit()
                     : GetTrialLimit();
        }

        virtual ConsensusValidateResult ValidateEdit(const PTransactionRef& ptx)
        {
            auto _ptx = static_pointer_cast<Video>(ptx);

            // First get original post transaction
            auto originalTx = PocketDb::TransRepoInst.GetByHash(*_ptx->GetRootTxHash());
            if (!originalTx)
                return {false, SocialConsensusResult_NotFound};

            // Change type not allowed
            if (*originalTx->GetType() != *_ptx->GetType())
                return {false, SocialConsensusResult_NotAllowed};

            // You are author? Really?
            if (*_ptx->GetAddress() != *originalTx->GetString1())
                return {false, SocialConsensusResult_ContentEditUnauthorized};

            // Original post edit only 24 hours
            if (!AllowEditWindow(_ptx, originalTx))
                return {false, SocialConsensusResult_ContentEditLimit};

            return make_tuple(true, SocialConsensusResult_Success);
        }

        // ------------------------------------------------------------------------------------------------------------

        ConsensusValidateResult ValidateBlock(const PTransactionRef& ptx, const PocketBlockRef& block) override
        {
            auto _ptx = static_pointer_cast<Video>(ptx);

            // Edit
            if (_ptx->IsEdit())
                return ValidateEditBlock(_ptx, block);

            // ---------------------------------------------------------
            // New

            // Get count from chain
            int count = GetChainCount(_ptx);

            // Get count from block
            for (auto& blockTx : *block)
            {
                if (!IsIn(*blockTx->GetType(), {CONTENT_VIDEO}))
                    continue;

                if (*blockTx->GetHash() == *_ptx->GetHash())
                    continue;

                if (*_ptx->GetAddress() == *blockTx->GetString1())
                    count += 1;
            }

            return ValidateLimit(_ptx, count);
        }

        ConsensusValidateResult ValidateMempool(const PTransactionRef& ptx) override
        {
            auto _ptx = static_pointer_cast<Video>(ptx);

            // Edit
            if (_ptx->IsEdit())
                return ValidateEditMempool(_ptx);

            // ---------------------------------------------------------
            // New

            // Get count from chain
            int count = GetChainCount(_ptx);

            // and from mempool
            count += ConsensusRepoInst.CountMempoolVideo(*_ptx->GetAddress());

            return ValidateLimit(_ptx, count);
        }

        virtual ConsensusValidateResult ValidateLimit(const PTransactionRef& ptx, int count)
        {
            auto _ptx = static_pointer_cast<Video>(ptx);

            auto reputationConsensus = PocketConsensus::ReputationConsensusFactoryInst.Instance(Height);

            auto[mode, reputation, balance] = reputationConsensus->GetAccountInfo(*_ptx->GetAddress());
            auto limit = GetLimit(mode);

            if (count >= limit)
                return {false, SocialConsensusResult_ContentLimit};

            return Success;
        }

        virtual int GetChainCount(const PTransactionRef& ptx)
        {
            auto _ptx = static_pointer_cast<Video>(ptx);

            return ConsensusRepoInst.CountChainVideoHeight(
                *_ptx->GetAddress(),
                Height - GetLimitWindow()
            );
        }


        virtual ConsensusValidateResult ValidateEditBlock(const PTransactionRef& ptx, const PocketBlockRef& block)
        {
            auto _ptx = static_pointer_cast<Video>(ptx);

            // Double edit in block not allowed
            for (auto& blockTx : *block)
            {
                if (!IsIn(*blockTx->GetType(), {CONTENT_VIDEO}))
                    continue;

                if (*blockTx->GetHash() == *_ptx->GetHash())
                    continue;

                if (*_ptx->GetRootTxHash() == *blockTx->GetString2())
                    return {false, SocialConsensusResult_DoubleContentEdit};
            }

            // Check edit limit
            return ValidateEditOneLimit(_ptx);
        }

        virtual ConsensusValidateResult ValidateEditMempool(const PTransactionRef& ptx)
        {
            auto _ptx = static_pointer_cast<Video>(ptx);

            if (ConsensusRepoInst.CountMempoolVideoEdit(*_ptx->GetAddress(), *_ptx->GetRootTxHash()) > 0)
                return {false, SocialConsensusResult_DoubleContentEdit};

            // Check edit limit
            return ValidateEditOneLimit(ptx);
        }

        virtual ConsensusValidateResult ValidateEditOneLimit(const PTransactionRef& ptx)
        {
            auto _ptx = static_pointer_cast<Video>(ptx);

            int count = ConsensusRepoInst.CountChainVideoEdit(*_ptx->GetAddress(), *_ptx->GetRootTxHash());
            if (count >= GetEditLimit())
                return {false, SocialConsensusResult_ContentEditLimit};

            return Success;
        }

        virtual bool AllowEditWindow(const PTransactionRef& ptx, const PTransactionRef& originalTx)
        {
            auto[ok, originalTxHeight] = ConsensusRepoInst.GetTransactionHeight(*originalTx->GetHash());
            if (!ok)
                return false;

            return (Height - originalTxHeight) <= GetEditWindow();
        }

        vector<string> GetAddressesForCheckRegistration(const PTransactionRef& ptx) override
        {
            auto _ptx = static_pointer_cast<Video>(ptx);
            return {*_ptx->GetAddress()};
        }

    };

    /*******************************************************************************************************************
    *  Start checkpoint at 1324655 block
    *******************************************************************************************************************/
    class VideoConsensus_checkpoint_1324655 : public VideoConsensus
    {
    public:
        VideoConsensus_checkpoint_1324655(int height) : VideoConsensus(height) {}
    protected:
        int64_t GetTrialLimit() override { return 5; }
    };

    /*******************************************************************************************************************
    *  Factory for select actual rules version
    *******************************************************************************************************************/
    class VideoConsensusFactory : public SocialConsensusFactory
    {
    private:
        const vector<ConsensusCheckpoint> _rules = {
            {0,       -1, [](int height) { return make_shared<VideoConsensus>(height); }},
            {1324655, 0,  [](int height) { return make_shared<VideoConsensus_checkpoint_1324655>(height); }},
        };
    protected:
        const vector<ConsensusCheckpoint>& m_rules() override { return _rules; }
    };
}

#endif // POCKETCONSENSUS_VIDEO_HPP