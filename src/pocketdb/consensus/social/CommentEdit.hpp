// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETCONSENSUS_COMMENT_EDIT_HPP
#define POCKETCONSENSUS_COMMENT_EDIT_HPP

#include "utils/html.h"
#include "pocketdb/consensus/Reputation.h"
#include "pocketdb/consensus/Social.h"
#include "pocketdb/models/dto/CommentEdit.h"

namespace PocketConsensus
{
    using namespace std;
    typedef shared_ptr<CommentEdit> CommentEditRef;

    /*******************************************************************************************************************
    *  CommentEdit consensus base class
    *******************************************************************************************************************/
    class CommentEditConsensus : public SocialConsensus<CommentEdit>
    {
    public:
        CommentEditConsensus(int height) : SocialConsensus<CommentEdit>(height) {}
        ConsensusValidateResult Validate(const CommentEditRef& ptx, const PocketBlockRef& block) override
        {
            // Base validation with calling block or mempool check
            if (auto[baseValidate, baseValidateCode] = SocialConsensus::Validate(ptx, block); !baseValidate)
                return {false, baseValidateCode};

            // Actual comment not deleted
            if (auto[ok, actuallTx] = ConsensusRepoInst.GetLastContent(*ptx->GetRootTxHash());
                !ok || *actuallTx->GetType() == PocketTxType::CONTENT_COMMENT_DELETE)
                return {false, SocialConsensusResult_CommentDeletedEdit};

            // Original comment exists
            auto originalTx = PocketDb::TransRepoInst.GetByHash(*ptx->GetRootTxHash());
            if (!originalTx)
                return {false, SocialConsensusResult_NotFound};

            auto originalPtx = static_pointer_cast<CommentEdit>(originalTx);

            // Parent comment
            {
                auto currParentTxHash = IsEmpty(ptx->GetParentTxHash()) ? "" : *ptx->GetParentTxHash();
                auto origParentTxHash = IsEmpty(originalPtx->GetParentTxHash()) ? "" : *originalPtx->GetParentTxHash();

                if (currParentTxHash != origParentTxHash)
                    return {false, SocialConsensusResult_InvalidParentComment};

                if (!IsEmpty(originalPtx->GetParentTxHash()))
                    if (!PocketDb::TransRepoInst.ExistsByHash(origParentTxHash))
                        return {false, SocialConsensusResult_InvalidParentComment};
            }

            // Answer comment
            {
                auto currAnswerTxHash = IsEmpty(ptx->GetAnswerTxHash()) ? "" : *ptx->GetAnswerTxHash();
                auto origAnswerTxHash = IsEmpty(originalPtx->GetAnswerTxHash()) ? "" : *originalPtx->GetAnswerTxHash();

                if (currAnswerTxHash != origAnswerTxHash)
                    return {false, SocialConsensusResult_InvalidAnswerComment};

                if (!IsEmpty(originalPtx->GetAnswerTxHash()))
                    if (!PocketDb::TransRepoInst.GetByHash(origAnswerTxHash))
                        return {false, SocialConsensusResult_InvalidAnswerComment};
            }

            // Original comment edit only 24 hours
            if (!AllowEditWindow(ptx, originalPtx))
                return {false, SocialConsensusResult_CommentEditLimit};

            // Check exists content transaction
            auto[contentOk, contentTx] = PocketDb::ConsensusRepoInst.GetLastContent(*ptx->GetPostTxHash());
            if (!contentOk)
                return {false, SocialConsensusResult_NotFound};

            if (*contentTx->GetType() == CONTENT_DELETE)
                return {false, SocialConsensusResult_CommentDeletedContent};

            // TODO (brangr): convert to Content base class
            //auto contentPtx = static_pointer_cast<Content>(contentTx);

            // Check Blocking
            if (auto[existsBlocking, blockingType] = PocketDb::ConsensusRepoInst.GetLastBlockingType(
                    *contentTx->GetString1(), *ptx->GetAddress()
                ); existsBlocking && blockingType == ACTION_BLOCKING)
                return {false, SocialConsensusResult_Blocking};

            // Check payload size
            if (auto[ok, code] = ValidatePayloadSize(ptx); !ok)
                return {false, code};

            // Check edit limit
            return ValidateEditOneLimit(ptx);
        }
        ConsensusValidateResult Check(const CTransactionRef& tx, const CommentEditRef& ptx) override
        {
            if (auto[baseCheck, baseCheckCode] = SocialConsensus::Check(tx, ptx); !baseCheck)
                return {false, baseCheckCode};

            // Check required fields
            if (IsEmpty(ptx->GetAddress())) return {false, SocialConsensusResult_Failed};
            if (IsEmpty(ptx->GetPostTxHash())) return {false, SocialConsensusResult_Failed};
            if (IsEmpty(ptx->GetRootTxHash())) return {false, SocialConsensusResult_Failed};

            // Maximum for message data
            if (!ptx->GetPayload()) return {false, SocialConsensusResult_Failed};
            if (IsEmpty(ptx->GetPayloadMsg())) return {false, SocialConsensusResult_Failed};
            if (HtmlUtils::UrlDecode(*ptx->GetPayloadMsg()).length() > (size_t)GetConsensusLimit(ConsensusLimit_max_comment_size))
                return {false, SocialConsensusResult_Size};

            return Success;
        }

    protected:

        ConsensusValidateResult ValidateBlock(const CommentEditRef& ptx, const PocketBlockRef& block) override
        {
            for (auto& blockTx : *block)
            {
                if (!IsIn(*blockTx->GetType(), {CONTENT_COMMENT, CONTENT_COMMENT_EDIT, CONTENT_COMMENT_DELETE}))
                    continue;

                auto blockPtx = static_pointer_cast<CommentEdit>(blockTx);

                if (*blockPtx->GetHash() == *ptx->GetHash())
                    continue;

                if (*ptx->GetRootTxHash() == *blockPtx->GetRootTxHash())
                    return {false, SocialConsensusResult_DoubleCommentEdit};
            }

            return Success;
        }
        ConsensusValidateResult ValidateMempool(const CommentEditRef& ptx) override
        {
            if (ConsensusRepoInst.CountMempoolCommentEdit(*ptx->GetAddress(), *ptx->GetRootTxHash()) > 0)
                return {false, SocialConsensusResult_DoubleCommentEdit};

            return Success;
        }
        vector<string> GetAddressesForCheckRegistration(const CommentEditRef& ptx) override
        {
            return {*ptx->GetAddress()};
        }

        virtual bool AllowEditWindow(const CommentEditRef& ptx, const CommentEditRef& blockPtx)
        {
            return (*ptx->GetTime() - *blockPtx->GetTime()) <= GetConsensusLimit(ConsensusLimit_edit_comment_depth);
        }
        virtual ConsensusValidateResult ValidateEditOneLimit(const CommentEditRef& ptx)
        {
            int count = ConsensusRepoInst.CountChainCommentEdit(*ptx->GetAddress(), *ptx->GetRootTxHash());
            if (count >= GetConsensusLimit(ConsensusLimit_comment_edit_count))
                return {false, SocialConsensusResult_CommentEditLimit};

            return Success;
        }
        virtual ConsensusValidateResult ValidatePayloadSize(const CommentEditRef& ptx)
        {
            size_t dataSize = (ptx->GetPayloadMsg() ? HtmlUtils::UrlDecode(*ptx->GetPayloadMsg()).size() : 0);

            if (dataSize > GetConsensusLimit(ConsensusLimit_max_comment_size))
                return {false, SocialConsensusResult_ContentSizeLimit};

            return Success;
        }
    };

    /*******************************************************************************************************************
    *  Start checkpoint at 1180000 block
    *******************************************************************************************************************/
    class CommentEditConsensus_checkpoint_1180000 : public CommentEditConsensus
    {
    public:
        CommentEditConsensus_checkpoint_1180000(int height) : CommentEditConsensus(height) {}
    protected:
        bool AllowEditWindow(const CommentEditRef& ptx, const CommentEditRef& originalTx) override
        {
            auto[ok, originalTxHeight] = ConsensusRepoInst.GetTransactionHeight(*originalTx->GetHash());
            if (!ok) return false;
            return (Height - originalTxHeight) <= GetConsensusLimit(ConsensusLimit_edit_comment_depth);
        }
    };

    /*******************************************************************************************************************
    *  Factory for select actual rules version
    *******************************************************************************************************************/
    class CommentEditConsensusFactory
    {
    private:
        const vector<ConsensusCheckpoint < CommentEditConsensus>> m_rules = {
            { 0, -1, [](int height) { return make_shared<CommentEditConsensus>(height); }},
            { 1180000, 0, [](int height) { return make_shared<CommentEditConsensus_checkpoint_1180000>(height); }},
        };
    public:
        shared_ptr<CommentEditConsensus> Instance(int height)
        {
            int m_height = (height > 0 ? height : 0);
            return (--upper_bound(m_rules.begin(), m_rules.end(), m_height,
                [&](int target, const ConsensusCheckpoint<CommentEditConsensus>& itm)
                {
                    return target < itm.Height(Params().NetworkIDString());
                }
            ))->m_func(height);
        }
    };
}

#endif // POCKETCONSENSUS_COMMENT_EDIT_HPP