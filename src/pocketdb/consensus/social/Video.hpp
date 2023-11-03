// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETCONSENSUS_VIDEO_HPP
#define POCKETCONSENSUS_VIDEO_HPP

#include "pocketdb/consensus/Reputation.h"
#include "pocketdb/consensus/Social.h"
#include "pocketdb/models/dto/content/Video.h"

namespace PocketConsensus
{
    typedef shared_ptr<Video> VideoRef;

    /*******************************************************************************************************************
    *  Video consensus base class
    *******************************************************************************************************************/
    class VideoConsensus : public SocialConsensus<Video>
    {
    public:
        VideoConsensus() : SocialConsensus<Video>()
        {
            // TODO (limits): set limits
            Limits.Set("payload_size", 60000, 60000, 60000);
        }

        ConsensusValidateResult Validate(const CTransactionRef& tx, const VideoRef& ptx, const PocketBlockRef& block) override
        {
            if (ptx->IsEdit())
                return ValidateEdit(ptx);

            return SocialConsensus::Validate(tx, ptx, block);
        }
        ConsensusValidateResult Check(const CTransactionRef& tx, const VideoRef& ptx) override
        {
            if (auto[baseCheck, baseCheckCode] = SocialConsensus::Check(tx, ptx); !baseCheck)
                return {false, baseCheckCode};

            // Check required fields
            if (IsEmpty(ptx->GetAddress())) return {false, ConsensusResult_Failed};

            // Repost not allowed
            if (!IsEmpty(ptx->GetRelayTxHash())) return {false, ConsensusResult_NotAllowed};

            return Success;
        }

    protected:        
        virtual int64_t GetLimit(AccountMode mode)
        {
            return mode == AccountMode_Pro
                   ? GetConsensusLimit(ConsensusLimit_pro_video)
                   : mode == AccountMode_Full
                     ? GetConsensusLimit(ConsensusLimit_full_video)
                     : GetConsensusLimit(ConsensusLimit_trial_video);
        }

        ConsensusValidateResult ValidateBlock(const VideoRef& ptx, const PocketBlockRef& block) override
        {
            // Edit
            if (ptx->IsEdit())
                return ValidateEditBlock(ptx, block);

            // ---------------------------------------------------------
            // New

            // Get count from chain
            int count = GetChainCount(ptx);

            // Get count from block
            for (auto& blockTx : *block)
            {
                if (!TransactionHelper::IsIn(*blockTx->GetType(), {CONTENT_VIDEO}))
                    continue;

                auto blockPtx = static_pointer_cast<Video>(blockTx);

                if (*ptx->GetAddress() != *blockPtx->GetAddress())
                    continue;

                if (*blockPtx->GetHash() == *ptx->GetHash())
                    continue;

                if (blockPtx->IsEdit())
                    continue;

                count += 1;
            }

            return ValidateLimit(ptx, count);
        }
        ConsensusValidateResult ValidateMempool(const VideoRef& ptx) override
        {

            // Edit
            if (ptx->IsEdit())
                return ValidateEditMempool(ptx);

            // ---------------------------------------------------------
            // New

            // Get count from chain
            int count = GetChainCount(ptx);

            // and from mempool
            count += ConsensusRepoInst.CountMempoolVideo(*ptx->GetAddress());

            return ValidateLimit(ptx, count);
        }
        vector<string> GetAddressesForCheckRegistration(const VideoRef& ptx) override
        {
            return {*ptx->GetAddress()};
        }

        virtual ConsensusValidateResult ValidateEdit(const VideoRef& ptx)
        {
            auto[lastContentOk, lastContent] = PocketDb::ConsensusRepoInst.GetLastContent(
                *ptx->GetRootTxHash(),
                { CONTENT_POST, CONTENT_VIDEO, CONTENT_DELETE }
            );
            if (lastContentOk && *lastContent->GetType() != CONTENT_VIDEO)
                return {false, ConsensusResult_NotAllowed};

            // First get original post transaction
            auto[originalTxOk, originalTx] = PocketDb::ConsensusRepoInst.GetFirstContent(*ptx->GetRootTxHash());
            if (!lastContentOk || !originalTxOk)
                return {false, ConsensusResult_NotFound};

            auto originalPtx = static_pointer_cast<Video>(originalTx);

            // Change type not allowed
            if (*originalPtx->GetType() != *ptx->GetType())
                return {false, ConsensusResult_NotAllowed};

            // You are author? Really?
            if (*ptx->GetAddress() != *originalPtx->GetAddress())
                return {false, ConsensusResult_ContentEditUnauthorized};

            // Original post edit only 24 hours
            if (!AllowEditWindow(ptx, originalPtx))
                return {false, ConsensusResult_ContentEditLimit};

            return Success;
        }
        // TODO (aok): move to base Social class
        virtual ConsensusValidateResult ValidateLimit(const VideoRef& ptx, int count)
        {
            auto reputationConsensus = PocketConsensus::ConsensusFactoryInst_Reputation.Instance(Height);
            auto address = ptx->GetAddress();
            auto[mode, reputation, balance] = reputationConsensus->GetAccountMode(*address);
            auto limit = GetLimit(mode);

            if (count >= limit)
                return {false, ConsensusResult_ContentLimit};

            return Success;
        }
        virtual int GetChainCount(const VideoRef& ptx)
        {

            return ConsensusRepoInst.CountChainHeight(
                *ptx->GetType(),
                *ptx->GetAddress()
            );
        }
        virtual ConsensusValidateResult ValidateEditBlock(const VideoRef& ptx, const PocketBlockRef& block)
        {

            // Double edit in block not allowed
            for (auto& blockTx : *block)
            {
                if (!TransactionHelper::IsIn(*blockTx->GetType(), {CONTENT_VIDEO, CONTENT_DELETE}))
                    continue;

                auto blockPtx = static_pointer_cast<Video>(blockTx);

                if (*blockPtx->GetHash() == *ptx->GetHash())
                    continue;

                if (*ptx->GetRootTxHash() == *blockPtx->GetRootTxHash())
                    return {false, ConsensusResult_DoubleContentEdit};
            }

            // Check edit limit
            return ValidateEditOneLimit(ptx);
        }
        virtual ConsensusValidateResult ValidateEditMempool(const VideoRef& ptx)
        {

            if (ConsensusRepoInst.CountMempoolVideoEdit(*ptx->GetAddress(), *ptx->GetRootTxHash()) > 0)
                return {false, ConsensusResult_DoubleContentEdit};

            // Check edit limit
            return ValidateEditOneLimit(ptx);
        }
        virtual ConsensusValidateResult ValidateEditOneLimit(const VideoRef& ptx)
        {

            int count = ConsensusRepoInst.CountChainVideoEdit(*ptx->GetAddress(), *ptx->GetRootTxHash());
            if (count >= GetConsensusLimit(ConsensusLimit_video_edit_count))
                return {false, ConsensusResult_ContentEditLimit};

            return Success;
        }
        virtual bool AllowEditWindow(const VideoRef& ptx, const VideoRef& originalTx)
        {
            auto[ok, originalTxHeight] = ConsensusRepoInst.GetTransactionHeight(*originalTx->GetHash());
            if (!ok)
                return false;

            return (Height - originalTxHeight) <= GetConsensusLimit(ConsensusLimit_edit_video_depth);
        }
    };

    // Fix general validating
    class VideoConsensus_checkpoint_tmp_fix : public VideoConsensus
    {
    public:
        VideoConsensus_checkpoint_tmp_fix() : VideoConsensus() {}
        
        ConsensusValidateResult Validate(const CTransactionRef& tx, const VideoRef& ptx, const PocketBlockRef& block) override
        {
            // Base validation with calling block or mempool check
            if (auto[baseValidate, baseValidateCode] = SocialConsensus::Validate(tx, ptx, block); !baseValidate)
                return {false, baseValidateCode};

            if (ptx->IsEdit())
                return ValidateEdit(ptx);

            return Success;
        }
    };

    class VideoConsensusFactory : public BaseConsensusFactory<VideoConsensus>
    {
    public:
        VideoConsensusFactory()
        {
            Checkpoint({       0,       0, -1, make_shared<VideoConsensus>() });
            Checkpoint({ 2552000, 2280000,  0, make_shared<VideoConsensus_checkpoint_tmp_fix>() });
        }
    };

    static VideoConsensusFactory ConsensusFactoryInst_Video;
}

#endif // POCKETCONSENSUS_VIDEO_HPP