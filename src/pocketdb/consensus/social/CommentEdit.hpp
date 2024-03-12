// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETCONSENSUS_COMMENT_EDIT_HPP
#define POCKETCONSENSUS_COMMENT_EDIT_HPP

#include "util/html.h"
#include "pocketdb/consensus/Reputation.h"
#include "pocketdb/consensus/Social.h"
#include "pocketdb/models/dto/content/CommentEdit.h"

namespace PocketConsensus
{
    typedef shared_ptr<CommentEdit> CommentEditRef;

    /*******************************************************************************************************************
    *  CommentEdit consensus base class
    *******************************************************************************************************************/
    class CommentEditConsensus : public SocialConsensus<CommentEdit>
    {
    public:
        CommentEditConsensus() : SocialConsensus<CommentEdit>()
        {
            // TODO (limits): set limits
            Limits.Set("payload_size", 2000, 2000, 2000);
        }

        ConsensusValidateResult Validate(const CTransactionRef& tx, const CommentEditRef& ptx, const PocketBlockRef& block) override
        {
            // Actual comment not deleted
            auto[actuallTxOk, actuallTx] = ConsensusRepoInst.GetLastContent(
                *ptx->GetRootTxHash(),
                { CONTENT_COMMENT, CONTENT_COMMENT_EDIT, CONTENT_COMMENT_DELETE }
            );
            if (!actuallTxOk || *actuallTx->GetType() == TxType::CONTENT_COMMENT_DELETE)
                return {false, ConsensusResult_CommentDeletedEdit};

            // Original comment exists
            auto[originalTxOk, originalTx] = PocketDb::ConsensusRepoInst.GetFirstContent(*ptx->GetRootTxHash());
            if (!actuallTxOk || !originalTxOk)
                return {false, ConsensusResult_NotFound};

            auto originalPtx = static_pointer_cast<CommentEdit>(originalTx);

            // Check author of comment
            if (auto[ok, result] = CheckAuthor(ptx, originalPtx); !ok)
                return {false, result};

            // Parent comment
            {
                auto currParentTxHash = IsEmpty(ptx->GetParentTxHash()) ? "" : *ptx->GetParentTxHash();
                auto origParentTxHash = IsEmpty(originalPtx->GetParentTxHash()) ? "" : *originalPtx->GetParentTxHash();

                if (currParentTxHash != origParentTxHash)
                    return {false, ConsensusResult_InvalidParentComment};

                if (!origParentTxHash.empty())
                {
                    if (auto[ok, origParentTx] = ConsensusRepoInst.GetLastContent(
                        origParentTxHash, { CONTENT_COMMENT, CONTENT_COMMENT_EDIT }); !ok)
                        return {false, ConsensusResult_InvalidParentComment};
                }
            }

            // Answer comment
            {
                auto currAnswerTxHash = IsEmpty(ptx->GetAnswerTxHash()) ? "" : *ptx->GetAnswerTxHash();
                auto origAnswerTxHash = IsEmpty(originalPtx->GetAnswerTxHash()) ? "" : *originalPtx->GetAnswerTxHash();

                if (currAnswerTxHash != origAnswerTxHash)
                    return {false, ConsensusResult_InvalidAnswerComment};

                if (!origAnswerTxHash.empty())
                {
                    if (auto[ok, origAnswerTx] = ConsensusRepoInst.GetLastContent(
                        origAnswerTxHash, { CONTENT_COMMENT, CONTENT_COMMENT_EDIT }); !ok)
                        return {false, ConsensusResult_InvalidAnswerComment};
                }
            }

            // Original comment edit only 24 hours
            if (!AllowEditWindow(ptx, originalPtx))
                return {false, ConsensusResult_CommentEditLimit};

            // Check exists content transaction
            auto[contentOk, contentTx] = PocketDb::ConsensusRepoInst.GetLastContent(
                *ptx->GetPostTxHash(), { CONTENT_POST, CONTENT_VIDEO, CONTENT_ARTICLE, CONTENT_STREAM, CONTENT_AUDIO, CONTENT_DELETE, BARTERON_OFFER });

            if (!contentOk)
                return {false, ConsensusResult_NotFound};

            if (*contentTx->GetType() == CONTENT_DELETE)
                return {false, ConsensusResult_CommentDeletedContent};
            
            // Check Blocking
            if (ValidateBlocking(*contentTx->GetString1(), *ptx->GetAddress()))
                return {false, ConsensusResult_Blocking};

            // Check edit limit
            if (auto[checkResult, checkCode] = ValidateEditOneLimit(ptx); !checkResult)
                return {false, checkCode};

            return SocialConsensus::Validate(tx, ptx, block);
        }
        ConsensusValidateResult Check(const CTransactionRef& tx, const CommentEditRef& ptx) override
        {
            if (auto[baseCheck, baseCheckCode] = SocialConsensus::Check(tx, ptx); !baseCheck)
                return {false, baseCheckCode};

            // Check required fields
            if (IsEmpty(ptx->GetAddress())) return {false, ConsensusResult_Failed};
            if (IsEmpty(ptx->GetPostTxHash())) return {false, ConsensusResult_Failed};
            if (IsEmpty(ptx->GetRootTxHash())) return {false, ConsensusResult_Failed};

            // Maximum for message data
            if (!ptx->GetPayload()) return {false, ConsensusResult_Size};
            if (IsEmpty(ptx->GetPayloadMsg())) return {false, ConsensusResult_Size};

            return Success;
        }

    protected:

        ConsensusValidateResult ValidateBlock(const CommentEditRef& ptx, const PocketBlockRef& block) override
        {
            for (auto& blockTx : *block)
            {
                if (!TransactionHelper::IsIn(*blockTx->GetType(), {CONTENT_COMMENT, CONTENT_COMMENT_EDIT, CONTENT_COMMENT_DELETE}))
                    continue;

                if (*blockTx->GetHash() == *ptx->GetHash())
                    continue;

                auto blockPtx = static_pointer_cast<CommentEdit>(blockTx);
                if (*ptx->GetRootTxHash() == *blockPtx->GetRootTxHash())
                    return {false, ConsensusResult_DoubleCommentEdit};
            }

            return Success;
        }
        ConsensusValidateResult ValidateMempool(const CommentEditRef& ptx) override
        {
            if (ConsensusRepoInst.CountMempoolCommentEdit(*ptx->GetAddress(), *ptx->GetRootTxHash()) > 0)
                return {false, ConsensusResult_DoubleCommentEdit};

            return Success;
        }
        vector<string> GetAddressesForCheckRegistration(const CommentEditRef& ptx) override
        {
            return {*ptx->GetAddress()};
        }

        virtual bool ValidateBlocking(const string& address1, const string& address2)
        {
            auto[existsBlocking, blockingType] = PocketDb::ConsensusRepoInst.GetLastBlockingType(address1, address2);
            return existsBlocking && blockingType == ACTION_BLOCKING;
        }
        virtual bool AllowEditWindow(const CommentEditRef& ptx, const CommentEditRef& blockPtx)
        {
            return (*ptx->GetTime() - *blockPtx->GetTime()) <= GetConsensusLimit(ConsensusLimit_edit_comment_depth);
        }
        virtual ConsensusValidateResult ValidateEditOneLimit(const CommentEditRef& ptx)
        {
            int count = ConsensusRepoInst.CountChainCommentEdit(*ptx->GetAddress(), *ptx->GetRootTxHash());
            if (count >= GetConsensusLimit(ConsensusLimit_comment_edit_count))
                return {false, ConsensusResult_CommentEditLimit};

            return Success;
        }
        virtual ConsensusValidateResult CheckAuthor(const CommentEditRef& ptx, const CommentEditRef& originalPtx)
        {
            return Success;
        }
    };

    // ----------------------------------------------------------------------------------------------
    class CommentEditConsensus_checkpoint_1180000 : public CommentEditConsensus
    {
    public:
        CommentEditConsensus_checkpoint_1180000() : CommentEditConsensus() {}
    protected:
        bool AllowEditWindow(const CommentEditRef& ptx, const CommentEditRef& originalTx) override
        {
            auto[ok, originalTxHeight] = ConsensusRepoInst.GetTransactionHeight(*originalTx->GetHash());
            if (!ok) return false;
            return (Height - originalTxHeight) <= GetConsensusLimit(ConsensusLimit_edit_comment_depth);
        }
    };

    // ----------------------------------------------------------------------------------------------
    class CommentEditConsensus_checkpoint_check_author : public CommentEditConsensus_checkpoint_1180000
    {
    public:
        CommentEditConsensus_checkpoint_check_author() : CommentEditConsensus_checkpoint_1180000() {}
    protected:
        ConsensusValidateResult CheckAuthor(const CommentEditRef& ptx, const CommentEditRef& originalPtx) override
        {
            if (*ptx->GetAddress() != *originalPtx->GetAddress())
                return {false, ConsensusResult_ContentEditUnauthorized};
            
            return Success;
        }
    };

    // ----------------------------------------------------------------------------------------------
    class CommentEditConsensus_checkpoint_pip_105 : public CommentEditConsensus_checkpoint_check_author
    {
    public:
        CommentEditConsensus_checkpoint_pip_105() : CommentEditConsensus_checkpoint_check_author() {}
    protected:
        bool ValidateBlocking(const string& address1, const string& address2) override
        {
            return SocialConsensus::CheckBlocking(address1, address2);
        }
    };


    // ----------------------------------------------------------------------------------------------
    // Factory for select actual rules version
    class CommentEditConsensusFactory : public BaseConsensusFactory<CommentEditConsensus>
    {
    public:
        CommentEditConsensusFactory()
        {
            Checkpoint({       0,      -1, -1, make_shared<CommentEditConsensus>() });
            Checkpoint({ 1180000,       0, -1, make_shared<CommentEditConsensus_checkpoint_1180000>() });
            Checkpoint({ 1873500, 1155000, -1, make_shared<CommentEditConsensus_checkpoint_check_author>() });
            Checkpoint({ 2794500, 2574300,  0, make_shared<CommentEditConsensus_checkpoint_pip_105>() });
        }
    };

    static CommentEditConsensusFactory ConsensusFactoryInst_CommentEdit;
}

#endif // POCKETCONSENSUS_COMMENT_EDIT_HPP