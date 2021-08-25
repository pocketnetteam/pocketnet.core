// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include "pocketdb/consensus/social/PostT.h"
#include "pocketdb/ReputationConsensus.h"

namespace PocketConsensus
{
    /*******************************************************************************************************************
    *  Post consensus base class
    *******************************************************************************************************************/
    PostConsensusT::PostConsensusT(int height) : SocialConsensusT<Post>(height) {}

    ConsensusValidateResult PostConsensusT::Validate(const shared_ptr<Post>& ptx, const PocketBlockRef& block)
    {
        // Base validation with calling block or mempool check
        if (auto[baseValidate, baseValidateCode] = SocialConsensusT::Validate(ptx, block); !baseValidate)
            return {false, baseValidateCode};

        if (ptx->IsEdit())
            return ValidateEdit(ptx);

        return Success;
    }

    ConsensusValidateResult PostConsensusT::Check(const CTransactionRef& tx, const shared_ptr<Post>& ptx)
    {
        if (auto[baseCheck, baseCheckCode] = SocialConsensusT::Check(tx, ptx); !baseCheck)
            return {false, baseCheckCode};

        // Check required fields
        if (IsEmpty(ptx->GetAddress())) return {false, SocialConsensusResult_Failed};

        return Success;
    }

    ConsensusValidateResult PostConsensusT::ValidateEdit(const shared_ptr<Post>& ptx)
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
        if (*ptx->GetAddress() != *originalPtx->GetAddress())
            return {false, SocialConsensusResult_ContentEditUnauthorized};

        // Original post edit only 24 hours
        if (!AllowEditWindow(ptx, originalPtx))
            return {false, SocialConsensusResult_ContentEditLimit};

        // Check edit limit
        return ValidateEditOneLimit(ptx);
    }

    ConsensusValidateResult PostConsensusT::ValidateBlock(const shared_ptr<Post>& ptx, const PocketBlockRef& block)
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

            // TODO (brangr): DEBUG
            //if (*blockTx->GetHash() == *ptx->GetHash())
            //    continue;

            if (*ptx->GetAddress() == *blockPtx->GetAddress())
            {
                if (AllowBlockLimitTime(ptx, blockPtx))
                    count += 1;
            }
        }

        return ValidateLimit(ptx, count);
    }

    ConsensusValidateResult PostConsensusT::ValidateMempool(const shared_ptr<Post>& ptx)
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

    ConsensusValidateResult PostConsensusT::ValidateLimit(const shared_ptr<Post>& ptx, int count)
    {
        auto reputationConsensus = PocketConsensus::ReputationConsensusFactoryInst.Instance(Height);
        auto[mode, reputation, balance] = reputationConsensus->GetAccountInfo(*ptx->GetAddress());
        if (count >= GetLimit(mode))
            return {false, SocialConsensusResult_ContentLimit};

        return Success;
    }

    bool PostConsensusT::AllowBlockLimitTime(const shared_ptr<Post>& ptx, const shared_ptr<Post>& blockPtx)
    {
        return *blockPtx->GetTime() <= *ptx->GetTime();
    }

    bool PostConsensusT::AllowEditWindow(const shared_ptr<Post>& ptx, const shared_ptr<Post>& originalTx)
    {
        return (*ptx->GetTime() - *originalTx->GetTime()) <= GetEditWindow();
    }

    int PostConsensusT::GetChainCount(const shared_ptr<Post>& ptx)
    {
        return ConsensusRepoInst.CountChainPostTime(
            *ptx->GetAddress(),
            *ptx->GetTime() - GetLimitWindow()
        );
    }

    ConsensusValidateResult PostConsensusT::ValidateEditBlock(const shared_ptr<Post>& ptx, const PocketBlockRef& block)
    {
        // Double edit in block not allowed
        for (auto& blockTx : *block)
        {
            if (!IsIn(*blockTx->GetType(), {CONTENT_POST}))
                continue;

            auto blockPtx = static_pointer_cast<Post>(blockTx);

            if (*blockPtx->GetHash() == *ptx->GetHash())
                continue;

            if (*ptx->GetRootTxHash() == *blockPtx->GetRootTxHash())
                return {false, SocialConsensusResult_DoubleContentEdit};
        }

        // Check edit limit
        return ValidateEditOneLimit(ptx);
    }

    ConsensusValidateResult PostConsensusT::ValidateEditMempool(const shared_ptr<Post>& ptx)
    {
        if (ConsensusRepoInst.CountMempoolPostEdit(*ptx->GetAddress(), *ptx->GetRootTxHash()) > 0)
            return {false, SocialConsensusResult_DoubleContentEdit};

        // Check edit limit
        return ValidateEditOneLimit(ptx);
    }

    ConsensusValidateResult PostConsensusT::ValidateEditOneLimit(const shared_ptr<Post>& ptx)
    {
        int count = ConsensusRepoInst.CountChainPostEdit(*ptx->GetAddress(), *ptx->GetRootTxHash());
        if (count >= GetEditLimit())
            return {false, SocialConsensusResult_ContentEditLimit};

        return Success;
    }

    vector <string> PostConsensusT::GetAddressesForCheckRegistration(const shared_ptr<Post>& ptx)
    {
        return {*ptx->GetAddress()};
    }

    /*******************************************************************************************************************
    *  Start checkpoint at 1124000 block
    *******************************************************************************************************************/
    bool PostConsensusT_checkpoint_1124000::AllowBlockLimitTime(const shared_ptr<Post>& ptx, const shared_ptr<Post>& blockPtx)
    {
        return true;
    }

    /*******************************************************************************************************************
    *  Start checkpoint at 1180000 block
    *******************************************************************************************************************/
    int PostConsensusT_checkpoint_1180000::GetChainCount(const shared_ptr<Post>& ptx)
    {
        return ConsensusRepoInst.CountChainPostHeight(*ptx->GetAddress(), Height - (int) GetLimitWindow());
    }
    bool PostConsensusT_checkpoint_1180000::AllowEditWindow(const shared_ptr<Post>& ptx, const shared_ptr<Post>& originalTx)
    {
        auto[ok, originalTxHeight] = ConsensusRepoInst.GetTransactionHeight(*originalTx->GetHash());
        if (!ok)
            return false;

        return (Height - originalTxHeight) <= GetEditWindow();
    }

    /*******************************************************************************************************************
    *  Start checkpoint at 1324655 block
    *******************************************************************************************************************/


} // namespace PocketConsensus
