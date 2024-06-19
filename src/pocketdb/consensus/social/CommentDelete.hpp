// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETCONSENSUS_COMMENT_DELETE_HPP
#define POCKETCONSENSUS_COMMENT_DELETE_HPP

#include "pocketdb/consensus/Social.h"
#include "pocketdb/models/dto/content/CommentDelete.h"

namespace PocketConsensus
{
    typedef shared_ptr<CommentDelete> CommentDeleteRef;

    /*******************************************************************************************************************
    *  CommentDelete consensus base class
    *******************************************************************************************************************/
    class CommentDeleteConsensus : public SocialConsensus<CommentDelete>
    {
    public:
        CommentDeleteConsensus() : SocialConsensus<CommentDelete>()
        {
            // TODO (limits): set limits
        }

        ConsensusValidateResult Validate(const CTransactionRef& tx, const CommentDeleteRef& ptx, const PocketBlockRef& block) override
        {
            // Actual comment not deleted
            auto[actuallTxOk, actuallTx] = ConsensusRepoInst.GetLastContent(
                *ptx->GetRootTxHash(),
                { CONTENT_COMMENT, CONTENT_COMMENT_EDIT, CONTENT_COMMENT_DELETE }
            );
            if (!actuallTxOk || *actuallTx->GetType() == TxType::CONTENT_COMMENT_DELETE)
                return {false, ConsensusResult_NotFound};

            // Original comment exists
            auto[originalTxOk, originalTx] = PocketDb::ConsensusRepoInst.GetFirstContent(*ptx->GetRootTxHash());
            if (!actuallTxOk || !originalTxOk)
                return {false, ConsensusResult_NotFound};

            auto originalPtx = static_pointer_cast<Comment>(originalTx);
        
            // Parent comment
            {
                auto currParentTxHash = IsEmpty(ptx->GetParentTxHash()) ? "" : *ptx->GetParentTxHash();
                auto origParentTxHash = IsEmpty(originalPtx->GetParentTxHash()) ? "" : *originalPtx->GetParentTxHash();

                if (currParentTxHash != origParentTxHash)
                    return {false, ConsensusResult_InvalidParentComment};

                if (!IsEmpty(originalPtx->GetParentTxHash()))
                    if (!PocketDb::TransRepoInst.ExistsLast(origParentTxHash))
                        return {false, ConsensusResult_InvalidParentComment};
            }

            // Answer comment
            {
                // GetString5() = AnswerTxHash
                auto currAnswerTxHash = IsEmpty(ptx->GetAnswerTxHash()) ? "" : *ptx->GetAnswerTxHash();
                auto origAnswerTxHash = IsEmpty(originalPtx->GetString5()) ? "" : *originalPtx->GetAnswerTxHash();

                if (currAnswerTxHash != origAnswerTxHash)
                    return {false, ConsensusResult_InvalidAnswerComment};

                if (!IsEmpty(originalPtx->GetAnswerTxHash()))
                    if (!PocketDb::TransRepoInst.Exists(origAnswerTxHash))
                        return {false, ConsensusResult_InvalidAnswerComment};
            }

            // Check exists content transaction
            auto[contentOk, contentTx] = PocketDb::ConsensusRepoInst.GetLastContent(
                *ptx->GetPostTxHash(), { CONTENT_POST, CONTENT_VIDEO, CONTENT_ARTICLE, CONTENT_STREAM, CONTENT_AUDIO, CONTENT_DELETE, BARTERON_OFFER, CONTENT_APP });

            if (!contentOk)
                return {false, ConsensusResult_NotFound};
            
            // Check author of comment
            if (auto[ok, result] = CheckAuthor(ptx, originalPtx, contentTx); !ok)
                return {false, result}; 

            return SocialConsensus::Validate(tx, ptx, block);
        }
        ConsensusValidateResult Check(const CTransactionRef& tx, const CommentDeleteRef& ptx) override
        {
            if (auto[baseCheck, baseCheckCode] = SocialConsensus::Check(tx, ptx); !baseCheck)
                return {false, baseCheckCode};

            // Check required fields
            if (IsEmpty(ptx->GetAddress())) return {false, ConsensusResult_Failed};
            if (IsEmpty(ptx->GetPostTxHash())) return {false, ConsensusResult_Failed};
            if (ptx->GetPayload()) return {false, ConsensusResult_Failed};

            return Success;
        }

    protected:
        ConsensusValidateResult ValidateBlock(const CommentDeleteRef& ptx, const PocketBlockRef& block) override
        {
            for (auto& blockTx : *block)
            {
                if (!TransactionHelper::IsIn(*blockTx->GetType(), {CONTENT_COMMENT, CONTENT_COMMENT_EDIT, CONTENT_COMMENT_DELETE}))
                    continue;

                if (*blockTx->GetHash() == *ptx->GetHash())
                    continue;

                auto blockPtx = static_pointer_cast<CommentDelete>(blockTx);
                if (*ptx->GetRootTxHash() == *blockPtx->GetRootTxHash())
                    return {false, ConsensusResult_DoubleCommentDelete};
            }

            return Success;
        }
        ConsensusValidateResult ValidateMempool(const CommentDeleteRef& ptx) override
        {
            if (ConsensusRepoInst.CountMempoolCommentEdit(*ptx->GetAddress(), *ptx->GetRootTxHash()) > 0)
                return {false, ConsensusResult_DoubleCommentDelete};

            return Success;
        }
        vector<string> GetAddressesForCheckRegistration(const CommentDeleteRef& ptx) override
        {
            return {*ptx->GetString1()};
        }
        virtual ConsensusValidateResult CheckAuthor(const CommentDeleteRef& ptx, const CommentRef& originalPtx, const PTransactionRef& contentTx)
        {
            return Success;
        }
    };

    // Protect deleting comment only for authors and authors of content
    class CommentDeleteConsensus_checkpoint_check_author : public CommentDeleteConsensus
    {
    public:
        CommentDeleteConsensus_checkpoint_check_author() : CommentDeleteConsensus() {}
    protected:
        ConsensusValidateResult CheckAuthor(const CommentDeleteRef& ptx, const CommentRef& originalPtx, const PTransactionRef& contentTx) override
        {
            if (*ptx->GetAddress() != *originalPtx->GetAddress() && *ptx->GetAddress() != *contentTx->GetString1())
                return {false, ConsensusResult_ContentEditUnauthorized};
            
            return Success;
        }
    };


    // ----------------------------------------------------------------------------------------------
    // Factory for select actual rules version
    class CommentDeleteConsensusFactory : public BaseConsensusFactory<CommentDeleteConsensus>
    {
    public:
        CommentDeleteConsensusFactory()
        {
            Checkpoint({       0,       0, -1, make_shared<CommentDeleteConsensus>() });
            Checkpoint({ 1873500, 1155000,  0, make_shared<CommentDeleteConsensus_checkpoint_check_author>() });
        }
    };

    static CommentDeleteConsensusFactory ConsensusFactoryInst_CommentDelete;
}

#endif // POCKETCONSENSUS_COMMENT_DELETE_HPP