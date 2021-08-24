// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETCONSENSUS_COMMENT_DELETE_HPP
#define POCKETCONSENSUS_COMMENT_DELETE_HPP

#include "pocketdb/consensus/Social.hpp"
#include "pocketdb/models/dto/CommentDelete.hpp"

namespace PocketConsensus
{
    /*******************************************************************************************************************
    *  CommentDelete consensus base class
    *******************************************************************************************************************/
    class CommentDeleteConsensus : public SocialConsensus
    {
    public:
        CommentDeleteConsensus(int height) : SocialConsensus(height) {}

        ConsensusValidateResult Validate(const PTransactionRef& ptx, const PocketBlockRef& block) override
        {
            auto _ptx = static_pointer_cast<CommentDelete>(ptx);

            // Base validation with calling block or mempool check
            if (auto[baseValidate, baseValidateCode] = SocialConsensus::Validate(ptx, block); !baseValidate)
                return {false, baseValidateCode};
                
            // Actual comment not deleted
            if (auto[ok, actuallTx] = ConsensusRepoInst.GetLastContent(*_ptx->GetRootTxHash());
                !ok || *actuallTx->GetType() == PocketTxType::CONTENT_COMMENT_DELETE)
                return {false, SocialConsensusResult_NotFound};

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

            return Success;
        }
        
        ConsensusValidateResult Check(const CTransactionRef& tx, const PTransactionRef& ptx) override
        {
            auto _ptx = static_pointer_cast<CommentDelete>(ptx);

            if (auto[baseCheck, baseCheckCode] = SocialConsensus::Check(tx, ptx); !baseCheck)
                return {false, baseCheckCode};

            // Check required fields
            if (IsEmpty(_ptx->GetAddress())) return {false, SocialConsensusResult_Failed};
            if (IsEmpty(_ptx->GetPostTxHash())) return {false, SocialConsensusResult_Failed};
            if (ptx->GetPayload()) return {false, SocialConsensusResult_Failed};

            return Success;
        }

    protected:

        ConsensusValidateResult ValidateBlock(const PTransactionRef& ptx, const PocketBlockRef& block) override
        {
            for (auto& blockTx : *block)
            {
                if (!IsIn(*blockTx->GetType(), {CONTENT_COMMENT, CONTENT_COMMENT_EDIT, CONTENT_COMMENT_DELETE}))
                    continue;

                if (*blockTx->GetHash() == *ptx->GetHash())
                    continue;

                // GetString2() = RootTxHash
                if (*ptx->GetString2() == *blockTx->GetString2())
                    return {false, SocialConsensusResult_DoubleCommentDelete};
            }

            return Success;
        }

        ConsensusValidateResult ValidateMempool(const PTransactionRef& ptx) override
        {
            // GetString2() = RootTxHash
            if (ConsensusRepoInst.CountMempoolCommentEdit(*ptx->GetString1(), *ptx->GetString2()) > 0)
                return {false, SocialConsensusResult_DoubleCommentDelete};

            return Success;
        }

        vector<string> GetAddressesForCheckRegistration(const PTransactionRef& ptx) override
        {
            return {*ptx->GetString1()};
        }
    };

    /*******************************************************************************************************************
    *  Factory for select actual rules version
    *******************************************************************************************************************/
    class CommentDeleteConsensusFactory : public SocialConsensusFactory
    {
    private:
        const vector<ConsensusCheckpoint> _rules = {
            {0, 0, [](int height) { return make_shared<CommentDeleteConsensus>(height); }},
        };
    protected:
        const vector<ConsensusCheckpoint>& m_rules() override { return _rules; }
    };
}

#endif // POCKETCONSENSUS_COMMENT_DELETE_HPP