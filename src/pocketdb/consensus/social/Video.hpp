// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 Bitcoin developers
// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETCONSENSUS_VIDEO_HPP
#define POCKETCONSENSUS_VIDEO_HPP

#include "pocketdb/consensus/Reputation.hpp"
#include "pocketdb/consensus/social/Base.hpp"
#include "pocketdb/models/dto/Video.hpp"

namespace PocketConsensus
{
    using namespace std;

    /*******************************************************************************************************************
    *
    *  Video consensus base class
    *
    *******************************************************************************************************************/
    class VideoConsensus : public SocialBaseConsensus
    {
    public:
        VideoConsensus(int height) : SocialBaseConsensus(height) {}

    protected:

        // TODO (brangr): setup limits after close https://github.com/pocketnetteam/pocketnet.core/issues/22

        virtual int GetLimitWindow() { return 1440; }

        virtual int64_t GetEditWindow() { return 1440; }

        virtual int64_t GetFullLimit() { return 30; }

        virtual int64_t GetFullEditLimit() { return 5; }

        virtual int64_t GetTrialLimit() { return 15; }

        virtual int64_t GetTrialEditLimit() { return 5; }

        virtual int64_t GetLimit(AccountMode mode)
        {
            return mode == AccountMode_Full ? GetFullLimit() : GetTrialLimit();
        }

        virtual int64_t GetEditLimit(AccountMode mode)
        {
            return mode == AccountMode_Full ? GetFullEditLimit() : GetTrialEditLimit();
        }

        // ------------------------------------------------------------------------------------------------------------

        tuple<bool, SocialConsensusResult> ValidateModel(const PTransactionRef & tx) override
        {
            auto ptx = static_pointer_cast<Video>(tx);

            if (ptx->IsEdit())
                return ValidateEditModel(ptx);

            return Success;
        }

        virtual tuple<bool, SocialConsensusResult> ValidateEditModel(const shared_ptr<Video>& tx)
        {
            // First get original post transaction
            auto originalTx = PocketDb::TransRepoInst.GetByHash(*tx->GetRootTxHash());
            if (!originalTx)
                return {false, SocialConsensusResult_NotFound};

            // Change type not allowed
            if (*originalTx->GetType() != *tx->GetType())
                return {false, SocialConsensusResult_NotAllowed};

            // Cast tx to Video for next checks
            auto originalVideoTx = static_pointer_cast<Video>(originalTx);

            // You are author? Really?
            if (*tx->GetAddress() != *originalVideoTx->GetAddress())
                return {false, SocialConsensusResult_ContentEditUnauthorized};

            // Original post edit only 24 hours
            if (!AllowEditWindow(tx, originalVideoTx))
                return {false, SocialConsensusResult_ContentEditLimit};

            return make_tuple(true, SocialConsensusResult_Success);
        }

        // ------------------------------------------------------------------------------------------------------------

        tuple<bool, SocialConsensusResult> ValidateLimit(const PTransactionRef & tx,
                                                         const PocketBlock& block) override
        {
            auto ptx = static_pointer_cast<Video>(tx);

            // ---------------------------------------------------------
            // Edit
            if (ptx->IsEdit())
                return ValidateEditLimit(ptx, block);

            // ---------------------------------------------------------
            // New

            // Get count from chain
            int count = GetChainCount(ptx);

            // Get count from block
            for (auto& blockTx : block)
            {
                if (!IsIn(*blockTx->GetType(), {CONTENT_VIDEO}))
                    continue;

                if (*blockTx->GetHash() == *ptx->GetHash())
                    continue;

                auto blockPtx = static_pointer_cast<Video>(blockTx);
                if (*ptx->GetAddress() == *blockPtx->GetAddress())
                    count += 1;
            }

            return ValidateLimit(ptx, count);
        }

        tuple<bool, SocialConsensusResult> ValidateLimit(const PTransactionRef & tx) override
        {
            auto ptx = static_pointer_cast<Video>(tx);

            // ---------------------------------------------------------
            // Edit
            if (ptx->IsEdit())
                return ValidateEditLimit(ptx);

            // ---------------------------------------------------------
            // New

            // Get count from chain
            int count = GetChainCount(ptx);

            // and from mempool
            count += ConsensusRepoInst.CountMempoolVideo(*ptx->GetAddress());

            return ValidateLimit(ptx, count);
        }

        virtual tuple<bool, SocialConsensusResult> ValidateLimit(const shared_ptr<Video>& tx, int count)
        {
            ReputationConsensusFactory reputationConsensusFactoryInst;
            auto reputationConsensus = reputationConsensusFactoryInst.Instance(Height);

            auto[mode, reputation, balance] = reputationConsensus->GetAccountInfo(*tx->GetAddress());
            auto limit = GetLimit(mode);

            if (count >= limit)
                return {false, SocialConsensusResult_ContentLimit};

            return Success;
        }

        virtual int GetChainCount(const shared_ptr<Video>& ptx)
        {
            return ConsensusRepoInst.CountChainVideoHeight(
                *ptx->GetAddress(),
                Height - GetLimitWindow()
            );
        }

        // ------------------------------------------------------------------------------------------------------------

        virtual tuple<bool, SocialConsensusResult> ValidateEditLimit(const shared_ptr<Video>& tx,
                                                                     const PocketBlock& block)
        {
            // Double edit in block not allowed
            for (auto& blockTx : block)
            {
                if (!IsIn(*blockTx->GetType(), {CONTENT_VIDEO}))
                    continue;

                if (*blockTx->GetHash() == *tx->GetHash())
                    continue;

                auto blockPtx = static_pointer_cast<Video>(blockTx);
                if (*tx->GetRootTxHash() == *blockPtx->GetRootTxHash())
                    return {false, SocialConsensusResult_DoubleContentEdit};
            }

            // Check edit one itm limit
            int count = ConsensusRepoInst.CountChainVideoEdit(*tx->GetAddress(), *tx->GetRootTxHash());

            ReputationConsensusFactory reputationConsensusFactoryInst;
            auto reputationConsensus = reputationConsensusFactoryInst.Instance(Height);

            auto[mode, reputation, balance] = reputationConsensus->GetAccountInfo(*tx->GetAddress());
            auto limit = GetEditLimit(mode);

            if (count >= limit)
                return {false, SocialConsensusResult_ContentEditLimit};

            return Success;
        }

        virtual tuple<bool, SocialConsensusResult> ValidateEditLimit(const shared_ptr<Video>& tx)
        {
            if (ConsensusRepoInst.CountMempoolVideoEdit(*tx->GetAddress(), *tx->GetRootTxHash()) > 0)
                return {false, SocialConsensusResult_DoubleContentEdit};

            return Success;
        }

        virtual bool AllowEditWindow(const PTransactionRef& ptx, const PTransactionRef& originalTx)
        {
            auto[ok, originalTxHeight] = ConsensusRepoInst.GetTransactionHeight(*originalTx->GetHash());
            if (!ok)
                return false;

            return (Height - originalTxHeight) <= GetEditWindow();
        }

        // ------------------------------------------------------------------------------------------------------------

        tuple<bool, SocialConsensusResult> CheckModel(const PTransactionRef & tx) override
        {
            auto ptx = static_pointer_cast<Video>(tx);

            // Check required fields
            if (IsEmpty(ptx->GetAddress())) return {false, SocialConsensusResult_Failed};

            // Repost not allowed
            if (!IsEmpty(ptx->GetRelayTxHash())) return {false, SocialConsensusResult_NotAllowed};

            return Success;
        }

        vector<string> GetAddressesForCheckRegistration(const PTransactionRef& tx) override
        {
            auto ptx = static_pointer_cast<Video>(tx);
            return {*ptx->GetAddress()};
        }

    };

    /*******************************************************************************************************************
    *
    *  Factory for select actual rules version
    *
    *******************************************************************************************************************/
    class VideoConsensusFactory
    {
    private:
        const std::map<int, std::function<VideoConsensus*(int height)>> m_rules =
            {
                {0, [](int height) { return new VideoConsensus(height); }},
            };
    public:
        shared_ptr<VideoConsensus> Instance(int height)
        {
            return shared_ptr<VideoConsensus>(
                (--m_rules.upper_bound(height))->second(height)
            );
        }
    };
}

#endif // POCKETCONSENSUS_VIDEO_HPP