// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include "pocketdb/consensus/social/Post.h"
#include "pocketdb/ReputationConsensus.h"

namespace PocketConsensus
{
    /*******************************************************************************************************************
    *  Post consensus base class
    *******************************************************************************************************************/
    ConsensusValidateResult PostConsensus::Validate(const PTransactionRef& ptx, const PocketBlockRef& block)
    {
        auto _ptx = static_pointer_cast<Post>(ptx);

        // Base validation with calling block or mempool check
        if (auto[baseValidate, baseValidateCode] = SocialConsensus::Validate(_ptx, block); !baseValidate)
            return {false, baseValidateCode};

        if (_ptx->IsEdit())
            return ValidateEdit(_ptx);

        return Success;
    }

    ConsensusValidateResult PostConsensus::Check(const CTransactionRef& tx, const PTransactionRef& ptx)
    {
        auto _ptx = static_pointer_cast<Post>(ptx);

        if (auto[baseCheck, baseCheckCode] = SocialConsensus::Check(tx, _ptx); !baseCheck)
            return {false, baseCheckCode};

        // Check required fields
        if (IsEmpty(_ptx->GetAddress())) return {false, SocialConsensusResult_Failed};

        return Success;
    }

    ConsensusValidateResult PostConsensus::ValidateEdit(const PTransactionRef& ptx)
    {
        auto _ptx = static_pointer_cast<Post>(ptx);

        // First get original post transaction
        auto originalTx = PocketDb::TransRepoInst.GetByHash(*_ptx->GetRootTxHash());
        if (!originalTx)
            return {false, SocialConsensusResult_NotFound};

        const auto originalPtx = static_pointer_cast<Post>(originalTx);

        // Change type not allowed
        if (*originalTx->GetType() != *_ptx->GetType())
            return {false, SocialConsensusResult_NotAllowed};

        // You are author? Really?
        if (*_ptx->GetAddress() != *originalPtx->GetAddress())
            return {false, SocialConsensusResult_ContentEditUnauthorized};

        // Original post edit only 24 hours
        if (!AllowEditWindow(_ptx, originalPtx))
            return {false, SocialConsensusResult_ContentEditLimit};

        // Check edit limit
        return ValidateEditOneLimit(_ptx);
    }

    ConsensusValidateResult PostConsensus::ValidateBlock(const PTransactionRef& ptx, const PocketBlockRef& block)
    {
        auto _ptx = static_pointer_cast<Post>(ptx);

        // Edit posts
        if (_ptx->IsEdit())
            return ValidateEditBlock(_ptx, block);

        // ---------------------------------------------------------
        // New posts

        // Get count from chain
        int count = GetChainCount(_ptx);

        // Get count from block
        for (const auto& blockTx : *block)
        {
            if (!IsIn(*blockTx->GetType(), {CONTENT_POST}))
                continue;

            const auto blockPtx = static_pointer_cast<Post>(blockTx);

            // TODO (brangr): DEBUG
            //if (*blockTx->GetHash() == *_ptx->GetHash())
            //    continue;

            if (*_ptx->GetAddress() == *blockPtx->GetAddress())
            {
                if (AllowBlockLimitTime(_ptx, blockPtx))
                    count += 1;
            }
        }

        return ValidateLimit(_ptx, count);
    }

    ConsensusValidateResult PostConsensus::ValidateMempool(const PTransactionRef& ptx)
    {
        auto _ptx = static_pointer_cast<Post>(ptx);

        // Edit posts
        if (_ptx->IsEdit())
            return ValidateEditMempool(_ptx);

        // ---------------------------------------------------------
        // New posts

        // Get count from chain
        int count = GetChainCount(_ptx);

        // Get count from mempool
        count += ConsensusRepoInst.CountMempoolPost(*_ptx->GetAddress());

        return ValidateLimit(_ptx, count);
    }

    ConsensusValidateResult PostConsensus::ValidateLimit(const PTransactionRef& ptx, int count)
    {
        auto _ptx = static_pointer_cast<Post>(ptx);

        auto reputationConsensus = PocketConsensus::ReputationConsensusFactoryInst.Instance(Height);
        auto[mode, reputation, balance] = reputationConsensus->GetAccountInfo(*_ptx->GetAddress());
        if (count >= GetLimit(mode))
            return {false, SocialConsensusResult_ContentLimit};

        return Success;
    }

    bool PostConsensus::AllowBlockLimitTime(const PTransactionRef& ptx, const PTransactionRef& blockPtx)
    {
        return *blockPtx->GetTime() <= *ptx->GetTime();
    }

    bool PostConsensus::AllowEditWindow(const PTransactionRef& ptx, const PTransactionRef& originalTx)
    {
        return (*ptx->GetTime() - *originalTx->GetTime()) <= GetEditWindow();
    }

    int PostConsensus::GetChainCount(const PTransactionRef& ptx)
    {
        auto _ptx = static_pointer_cast<Post>(ptx);
        return ConsensusRepoInst.CountChainPostTime(
            *_ptx->GetAddress(),
            *_ptx->GetTime() - GetLimitWindow()
        );
    }

    ConsensusValidateResult PostConsensus::ValidateEditBlock(const PTransactionRef& ptx, const PocketBlockRef& block)
    {
        auto _ptx = static_pointer_cast<Post>(ptx);

        // Double edit in block not allowed
        for (auto& blockTx : *block)
        {
            if (!IsIn(*blockTx->GetType(), {CONTENT_POST}))
                continue;

            auto blockPtx = static_pointer_cast<Post>(blockTx);

            if (*blockPtx->GetHash() == *_ptx->GetHash())
                continue;

            if (*_ptx->GetRootTxHash() == *blockPtx->GetRootTxHash())
                return {false, SocialConsensusResult_DoubleContentEdit};
        }

        // Check edit limit
        return ValidateEditOneLimit(_ptx);
    }

    ConsensusValidateResult PostConsensus::ValidateEditMempool(const PTransactionRef& ptx)
    {
        auto _ptx = static_pointer_cast<Post>(ptx);

        if (ConsensusRepoInst.CountMempoolPostEdit(*_ptx->GetAddress(), *_ptx->GetRootTxHash()) > 0)
            return {false, SocialConsensusResult_DoubleContentEdit};

        // Check edit limit
        return ValidateEditOneLimit(_ptx);
    }

    ConsensusValidateResult PostConsensus::ValidateEditOneLimit(const PTransactionRef& ptx)
    {
        auto _ptx = static_pointer_cast<Post>(ptx);
        int count = ConsensusRepoInst.CountChainPostEdit(*_ptx->GetAddress(), *_ptx->GetRootTxHash());
        if (count >= GetEditLimit())
            return {false, SocialConsensusResult_ContentEditLimit};

        return Success;
    }

    vector <string> PostConsensus::GetAddressesForCheckRegistration(const PTransactionRef& ptx)
    {
        auto _ptx = static_pointer_cast<Post>(ptx);
        return {*_ptx->GetAddress()};
    }

    /*******************************************************************************************************************
    *  Start checkpoint at 1124000 block
    *******************************************************************************************************************/
    bool PostConsensus_checkpoint_1124000::AllowBlockLimitTime(const PTransactionRef& ptx, const PTransactionRef& blockPtx)
    {
        return true;
    }

    /*******************************************************************************************************************
    *  Start checkpoint at 1180000 block
    *******************************************************************************************************************/
    int PostConsensus_checkpoint_1180000::GetChainCount(const PTransactionRef& ptx)
    {
        auto _ptx = static_pointer_cast<Post>(ptx);
        return ConsensusRepoInst.CountChainPostHeight(*_ptx->GetAddress(), Height - (int) GetLimitWindow());
    }
    bool PostConsensus_checkpoint_1180000::AllowEditWindow(const PTransactionRef& ptx, const PTransactionRef& originalTx)
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
