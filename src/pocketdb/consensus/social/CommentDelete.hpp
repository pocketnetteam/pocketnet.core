// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 Bitcoin developers
// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETCONSENSUS_COMMENT_DELETE_HPP
#define POCKETCONSENSUS_COMMENT_DELETE_HPP

#include "pocketdb/consensus/social/Social.hpp"
#include "pocketdb/models/dto/CommentDelete.hpp"

namespace PocketConsensus
{
    /*******************************************************************************************************************
    *
    *  CommentDelete consensus base class
    *
    *******************************************************************************************************************/
    class CommentDeleteConsensus : public SocialConsensus
    {
    public:
        CommentDeleteConsensus(int height) : SocialConsensus(height) {}

    protected:

        tuple<bool, SocialConsensusResult> ValidateModel(const shared_ptr <Transaction>& tx) override
        {
            auto ptx = static_pointer_cast<CommentDelete>(tx);

            // Actual comment not deleted
            if (auto[ok, actuallTx] = ConsensusRepoInst.GetLastContent(*ptx->GetRootTxHash());
                !ok || *actuallTx->GetType() == PocketTxType::CONTENT_COMMENT_DELETE)
                return {false, SocialConsensusResult_NotFound};

            // Original comment exists
            auto originalTx = PocketDb::TransRepoInst.GetByHash(*ptx->GetRootTxHash());
            if (!originalTx)
                return {false, SocialConsensusResult_NotFound};

            // Parent comment
            {
                // GetString4() = ParentTxHash
                auto currParentTxHash = IsEmpty(ptx->GetParentTxHash()) ? "" : *ptx->GetParentTxHash();
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
                auto currAnswerTxHash = IsEmpty(ptx->GetAnswerTxHash()) ? "" : *ptx->GetAnswerTxHash();
                auto origAnswerTxHash = IsEmpty(originalTx->GetString5()) ? "" : *originalTx->GetString5();

                if (currAnswerTxHash != origAnswerTxHash)
                    return {false, SocialConsensusResult_InvalidAnswerComment};

                if (!IsEmpty(originalTx->GetString5()))
                    if (!PocketDb::TransRepoInst.GetByHash(origAnswerTxHash))
                        return {false, SocialConsensusResult_InvalidAnswerComment};
            }

            return Success;
        }

        tuple<bool, SocialConsensusResult> ValidateLimit(const PTransactionRef& tx, const PocketBlock& block) override
        {
            for (auto& blockTx : block)
            {
                if (!IsIn(*blockTx->GetType(), {CONTENT_COMMENT, CONTENT_COMMENT_EDIT, CONTENT_COMMENT_DELETE}))
                    continue;

                if (*blockTx->GetHash() == *tx->GetHash())
                    continue;

                // GetString2() = RootTxHash
                if (*tx->GetString2() == *blockTx->GetString2())
                    return {false, SocialConsensusResult_DoubleCommentDelete};
            }

            return Success;
        }

        tuple<bool, SocialConsensusResult> ValidateLimit(const PTransactionRef& tx) override
        {
            // GetString2() = RootTxHash
            if (ConsensusRepoInst.CountMempoolCommentEdit(*tx->GetString1(), *tx->GetString2()) > 0)
                return {false, SocialConsensusResult_DoubleCommentDelete};

            return Success;
        }

        tuple<bool, SocialConsensusResult> CheckModel(const shared_ptr <Transaction>& tx) override
        {
            auto ptx = static_pointer_cast<CommentDelete>(tx);

            // Check required fields
            if (IsEmpty(ptx->GetAddress())) return {false, SocialConsensusResult_Failed};
            if (IsEmpty(ptx->GetPostTxHash())) return {false, SocialConsensusResult_Failed};
            if (ptx->GetPayload()) return {false, SocialConsensusResult_Failed};

            return Success;
        }

        vector <string> GetAddressesForCheckRegistration(const PTransactionRef& tx) override
        {
            auto ptx = static_pointer_cast<CommentDelete>(tx);
            return {*ptx->GetAddress()};
        }

    };

    /*******************************************************************************************************************
    *
    *  Factory for select actual rules version
    *
    *******************************************************************************************************************/
    class CommentDeleteConsensusFactory : public SocialConsensusFactory
    {
    public:
        CommentDeleteConsensusFactory() : SocialConsensusFactory()
        {
            m_rules =
            {
                {0, 0, [](int height) { return new CommentDeleteConsensus(height); }},
            };
        }
    };
}

#endif // POCKETCONSENSUS_COMMENT_DELETE_HPP