// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETCONSENSUS_COMMENT_DELETE_HPP
#define POCKETCONSENSUS_COMMENT_DELETE_HPP

#include "pocketdb/consensus/Social.h"
#include "pocketdb/models/dto/content/CommentDelete.h"

namespace PocketConsensus
{
    using namespace std;
    typedef shared_ptr<CommentDelete> CommentDeleteRef;

    /*******************************************************************************************************************
    *  CommentDelete consensus base class
    *******************************************************************************************************************/
    class CommentDeleteConsensus : public SocialConsensus<CommentDelete>
    {
    public:
        CommentDeleteConsensus(int height) : SocialConsensus<CommentDelete>(height) {}

        ConsensusValidateResult Validate(const CTransactionRef& tx, const CommentDeleteRef& ptx, const PocketBlockRef& block) override
        {
            // Actual comment not deleted
            auto[actuallTxOk, actuallTx] = ConsensusRepoInst.GetLastContent(
                *ptx->GetRootTxHash(),
                { CONTENT_COMMENT, CONTENT_COMMENT_EDIT, CONTENT_COMMENT_DELETE }
            );
            if (!actuallTxOk || *actuallTx->GetType() == TxType::CONTENT_COMMENT_DELETE)
                return {false, SocialConsensusResult_NotFound};

            // Original comment exists
            auto[originalTxOk, originalTx] = PocketDb::ConsensusRepoInst.GetFirstContent(*ptx->GetRootTxHash());
            if (!actuallTxOk || !originalTxOk)
                return {false, SocialConsensusResult_NotFound};

            auto originalPtx = static_pointer_cast<Comment>(originalTx);
        
            // Parent comment
            {
                auto currParentTxHash = IsEmpty(ptx->GetParentTxHash()) ? "" : *ptx->GetParentTxHash();
                auto origParentTxHash = IsEmpty(originalPtx->GetParentTxHash()) ? "" : *originalPtx->GetParentTxHash();

                if (currParentTxHash != origParentTxHash)
                    return {false, SocialConsensusResult_InvalidParentComment};

                if (!IsEmpty(originalPtx->GetParentTxHash()))
                    if (!PocketDb::TransRepoInst.ExistsInChain(origParentTxHash))
                        return {false, SocialConsensusResult_InvalidParentComment};
            }

            // Answer comment
            {
                // GetString5() = AnswerTxHash
                auto currAnswerTxHash = IsEmpty(ptx->GetAnswerTxHash()) ? "" : *ptx->GetAnswerTxHash();
                auto origAnswerTxHash = IsEmpty(originalPtx->GetString5()) ? "" : *originalPtx->GetAnswerTxHash();

                if (currAnswerTxHash != origAnswerTxHash)
                    return {false, SocialConsensusResult_InvalidAnswerComment};

                if (!IsEmpty(originalPtx->GetAnswerTxHash()))
                    if (!PocketDb::TransRepoInst.Exists(origAnswerTxHash))
                        return {false, SocialConsensusResult_InvalidAnswerComment};
            }

            // Check exists content transaction
            auto[contentOk, contentTx] = PocketDb::ConsensusRepoInst.GetLastContent(
                *ptx->GetPostTxHash(), { CONTENT_POST, CONTENT_VIDEO, CONTENT_ARTICLE, CONTENT_DELETE });

            if (!contentOk)
                return {false, SocialConsensusResult_NotFound};
            
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
            if (IsEmpty(ptx->GetAddress())) return {false, SocialConsensusResult_Failed};
            if (IsEmpty(ptx->GetPostTxHash())) return {false, SocialConsensusResult_Failed};
            if (ptx->GetPayload()) return {false, SocialConsensusResult_Failed};

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
                    return {false, SocialConsensusResult_DoubleCommentDelete};
            }

            return Success;
        }
        ConsensusValidateResult ValidateMempool(const CommentDeleteRef& ptx) override
        {
            if (ConsensusRepoInst.CountMempoolCommentEdit(*ptx->GetAddress(), *ptx->GetRootTxHash()) > 0)
                return {false, SocialConsensusResult_DoubleCommentDelete};

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
        CommentDeleteConsensus_checkpoint_check_author(int height) : CommentDeleteConsensus(height) {}
    protected:
        ConsensusValidateResult CheckAuthor(const CommentDeleteRef& ptx, const CommentRef& originalPtx, const PTransactionRef& contentTx) override
        {
            if (*ptx->GetAddress() != *originalPtx->GetAddress() && *ptx->GetAddress() != *contentTx->GetString1())
                return {false, SocialConsensusResult_ContentEditUnauthorized};
            
            return Success;
        }
    };

    /*******************************************************************************************************************
    *  Factory for select actual rules version
    *******************************************************************************************************************/
    class CommentDeleteConsensusFactory
    {
    private:
        const vector<ConsensusCheckpoint < CommentDeleteConsensus>> m_rules = {
            {       0,       0, [](int height) { return make_shared<CommentDeleteConsensus>(height); }},
            { 1873500, 1155000, [](int height) { return make_shared<CommentDeleteConsensus_checkpoint_check_author>(height); }},
        };
    public:
        shared_ptr<CommentDeleteConsensus> Instance(int height)
        {
            int m_height = (height > 0 ? height : 0);
            return (--upper_bound(m_rules.begin(), m_rules.end(), m_height,
                [&](int target, const ConsensusCheckpoint<CommentDeleteConsensus>& itm)
                {
                    return target < itm.Height(Params().NetworkIDString());
                }
            ))->m_func(m_height);
        }
    };
}

#endif // POCKETCONSENSUS_COMMENT_DELETE_HPP