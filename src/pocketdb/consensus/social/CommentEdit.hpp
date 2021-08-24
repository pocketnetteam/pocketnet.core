// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETCONSENSUS_COMMENT_EDIT_HPP
#define POCKETCONSENSUS_COMMENT_EDIT_HPP

#include "utils/html.h"
#include "pocketdb/consensus/Reputation.hpp"
#include "pocketdb/consensus/Social.hpp"
#include "pocketdb/models/dto/CommentEdit.hpp"

namespace PocketConsensus
{
    /*******************************************************************************************************************
    *  CommentEdit consensus base class
    *******************************************************************************************************************/
    class CommentEditConsensus : public SocialConsensus
    {
    public:
        CommentEditConsensus(int height) : SocialConsensus(height) {}

        ConsensusValidateResult Validate(const PTransactionRef& ptx, const PocketBlockRef& block) override
        {
            auto _ptx = static_pointer_cast<CommentEdit>(ptx);

            // Base validation with calling block or mempool check
            if (auto[baseValidate, baseValidateCode] = SocialConsensus::Validate(ptx, block); !baseValidate)
                return {false, baseValidateCode};
                
            // Actual comment not deleted
            if (auto[ok, actuallTx] = ConsensusRepoInst.GetLastContent(*_ptx->GetRootTxHash());
                !ok || *actuallTx->GetType() == PocketTxType::CONTENT_COMMENT_DELETE)
                return {false, SocialConsensusResult_CommentDeletedEdit};

            // Original comment exists
            auto originalTx = PocketDb::TransRepoInst.GetByHash(*_ptx->GetRootTxHash());
            if (!originalTx)
                return {false, SocialConsensusResult_NotFound};

            // Parent comment
            {
                // GetString4() = ParentTxHash
                auto currParentTxHash = IsEmpty(_ptx->GetParentTxHash()) ? "" : *_ptx->GetParentTxHash();
                auto origParentTxHash = IsEmpty(originalTx->GetString4()) ? "" : *originalTx->GetString4();

                if (currParentTxHash != origParentTxHash)
                    return {false, SocialConsensusResult_InvalidParentComment};

                if (!IsEmpty(originalTx->GetString4()))
                    if (!PocketDb::TransRepoInst.GetByHash(origParentTxHash))
                        return {false, SocialConsensusResult_InvalidParentComment};
            }

            // Answer comment
            {
                // GetString5() = AnswerTxHash
                auto currAnswerTxHash = IsEmpty(_ptx->GetAnswerTxHash()) ? "" : *_ptx->GetAnswerTxHash();
                auto origAnswerTxHash = IsEmpty(originalTx->GetString5()) ? "" : *originalTx->GetString5();

                if (currAnswerTxHash != origAnswerTxHash)
                    return {false, SocialConsensusResult_InvalidAnswerComment};

                if (!IsEmpty(originalTx->GetString5()))
                    if (!PocketDb::TransRepoInst.GetByHash(origAnswerTxHash))
                        return {false, SocialConsensusResult_InvalidAnswerComment};
            }

            // Original comment edit only 24 hours
            if (!AllowEditWindow(_ptx, originalTx))
                return {false, SocialConsensusResult_CommentEditLimit};

            // Check exists content transaction
            auto contentTx = PocketDb::TransRepoInst.GetByHash(*_ptx->GetPostTxHash());
            if (!contentTx)
                return {false, SocialConsensusResult_NotFound};

            // Check Blocking
            if (auto[existsBlocking, blockingType] = PocketDb::ConsensusRepoInst.GetLastBlockingType(
                    *contentTx->GetString1(), *_ptx->GetAddress() // GetString1() returned author content
                ); existsBlocking && blockingType == ACTION_BLOCKING)
                return {false, SocialConsensusResult_Blocking};

            // Check edit limit
            return ValidateEditOneLimit(ptx);
        }

        ConsensusValidateResult Check(const CTransactionRef& tx, const PTransactionRef& ptx) override
        {
            auto _ptx = static_pointer_cast<CommentEdit>(ptx);

            if (auto[baseCheck, baseCheckCode] = SocialConsensus::Check(tx, ptx); !baseCheck)
                return {false, baseCheckCode};

            // Check required fields
            if (IsEmpty(_ptx->GetAddress())) return {false, SocialConsensusResult_Failed};
            if (IsEmpty(_ptx->GetPostTxHash())) return {false, SocialConsensusResult_Failed};
            if (IsEmpty(_ptx->GetRootTxHash())) return {false, SocialConsensusResult_Failed};

            // Maximum for message data
            if (!_ptx->GetPayload()) return {false, SocialConsensusResult_Failed};
            if (IsEmpty(_ptx->GetPayloadMsg())) return {false, SocialConsensusResult_Failed};
            if (HtmlUtils::UrlDecode(*_ptx->GetPayloadMsg()).length() > GetCommentMessageMaxSize())
                return {false, SocialConsensusResult_Size};

            return Success;
        }

    protected:
        virtual int64_t GetEditWindow() { return 86400; }
        virtual size_t GetCommentMessageMaxSize() { return 2000; }
        virtual int64_t GetEditLimit() { return 4; }


        ConsensusValidateResult ValidateBlock(const PTransactionRef& ptx, const PocketBlockRef& block) override
        {
            for (auto& blockTx : *block)
            {
                if (!IsIn(*blockTx->GetType(), {CONTENT_COMMENT, CONTENT_COMMENT_EDIT, CONTENT_COMMENT_DELETE}))
                    continue;

                if (*blockTx->GetHash() == *ptx->GetHash())
                    continue;

                if (*ptx->GetString2() == *blockTx->GetString2())
                    return {false, SocialConsensusResult_DoubleCommentEdit};
            }

            return Success;
        }

        ConsensusValidateResult ValidateMempool(const PTransactionRef& ptx) override
        {
            auto _ptx = static_pointer_cast<CommentEdit>(ptx);
            if (ConsensusRepoInst.CountMempoolCommentEdit(*_ptx->GetAddress(), *_ptx->GetRootTxHash()) > 0)
                return {false, SocialConsensusResult_DoubleCommentEdit};

            return Success;
        }
        
        virtual bool AllowEditWindow(const PTransactionRef& ptx, const PTransactionRef& blockPtx)
        {
            return (*ptx->GetTime() - *blockPtx->GetTime()) <= GetEditWindow();
        }

        virtual ConsensusValidateResult ValidateEditOneLimit(const PTransactionRef& ptx)
        {
            auto _ptx = static_pointer_cast<CommentEdit>(ptx);
            int count = ConsensusRepoInst.CountChainCommentEdit(*_ptx->GetString1(), *_ptx->GetRootTxHash());
            if (count >= GetEditLimit())
                return {false, SocialConsensusResult_CommentEditLimit};

            return Success;
        }

        vector<string> GetAddressesForCheckRegistration(const PTransactionRef& ptx) override
        {
            return {*ptx->GetString1()};
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

        int64_t GetEditWindow() override { return 1440; }

        bool AllowEditWindow(const PTransactionRef& ptx, const PTransactionRef& originalTx) override
        {
            auto[ok, originalTxHeight] = ConsensusRepoInst.GetTransactionHeight(*originalTx->GetHash());
            if (!ok) return false;
            return (Height - originalTxHeight) <= GetEditWindow();
        }
    };

    /*******************************************************************************************************************
    *  Factory for select actual rules version
    *******************************************************************************************************************/
    class CommentEditConsensusFactory : public SocialConsensusFactory
    {
    private:
        const vector<ConsensusCheckpoint> _rules = {
            {0,       -1, [](int height) { return make_shared<CommentEditConsensus>(height); }},
            {1180000, 0,  [](int height) { return make_shared<CommentEditConsensus_checkpoint_1180000>(height); }},
        };
    protected:
        const vector<ConsensusCheckpoint>& m_rules() override { return _rules; }
    };
}

#endif // POCKETCONSENSUS_COMMENT_EDIT_HPP