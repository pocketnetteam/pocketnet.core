// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 Bitcoin developers
// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETCONSENSUS_COMMENT_EDIT_HPP
#define POCKETCONSENSUS_COMMENT_EDIT_HPP

#include "pocketdb/consensus/social/Base.hpp"
#include "pocketdb/models/dto/CommentEdit.hpp"

namespace PocketConsensus
{
    using namespace std;

    /*******************************************************************************************************************
    *
    *  CommentEdit consensus base class
    *
    *******************************************************************************************************************/
    class CommentEditConsensus : public SocialBaseConsensus
    {
    public:
        CommentEditConsensus(int height) : SocialBaseConsensus(height) {}

    protected:

        virtual int64_t GetEditWindow() { return 86400; }

        virtual size_t GetCommentMessageMaxSize() { return 2000; }

        virtual int64_t GetFullEditLimit() { return 5; }

        virtual int64_t GetTrialEditLimit() { return 5; }

        virtual int64_t GetEditLimit(AccountMode mode)
        {
            return mode == AccountMode_Full ? GetFullEditLimit() : GetTrialEditLimit();
        }


        tuple<bool, SocialConsensusResult> ValidateModel(const shared_ptr<Transaction>& tx) override
        {
            auto ptx = static_pointer_cast<CommentEdit>(tx);

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

            // Original comment edit only 24 hours
            if (!AllowEditWindow(tx, originalTx))
                return {false, SocialConsensusResult_CommentEditLimit};

            // Check exists content transaction
            auto contentTx = PocketDb::TransRepoInst.GetByHash(*ptx->GetPostTxHash());
            if (!contentTx)
                return {false, SocialConsensusResult_NotFound};

            // Check Blocking
            if (auto[existsBlocking, blockingType] = PocketDb::ConsensusRepoInst.GetLastBlockingType(
                    *contentTx->GetString1(), *ptx->GetAddress() // GetString1() returned author content
                ); existsBlocking && blockingType == ACTION_BLOCKING)
                return {false, SocialConsensusResult_Blocking};

            return Success;
        }

        virtual bool AllowEditWindow(const PTransactionRef& ptx, const PTransactionRef& blockPtx)
        {
            return (*ptx->GetTime() - *blockPtx->GetTime()) <= GetEditWindow();
        }

        tuple<bool, SocialConsensusResult> ValidateLimit(const PTransactionRef& tx,
                                                         const PocketBlock& block) override
        {
            auto ptx = static_pointer_cast<CommentEdit>(tx);

            for (auto& blockTx : block)
            {
                if (!IsIn(*blockTx->GetType(), {CONTENT_COMMENT, CONTENT_COMMENT_EDIT, CONTENT_COMMENT_DELETE}))
                    continue;

                if (*blockTx->GetHash() == *ptx->GetHash())
                    continue;

                auto blockPtx = static_pointer_cast<Comment>(blockTx);
                if (*ptx->GetRootTxHash() == *blockPtx->GetRootTxHash())
                    return {false, SocialConsensusResult_DoubleCommentEdit};
            }

            // Check edit limit
            return ValidateEditOneLimit(ptx);
        }

        tuple<bool, SocialConsensusResult> ValidateLimit(const PTransactionRef& tx) override
        {
            auto ptx = static_pointer_cast<CommentEdit>(tx);

            if (ConsensusRepoInst.CountMempoolCommentEdit(*ptx->GetRootTxHash()) > 0)
                return {false, SocialConsensusResult_DoubleCommentEdit};

            // Check edit limit
            return ValidateEditOneLimit(ptx);
        }

        virtual tuple<bool, SocialConsensusResult> ValidateEditOneLimit(const shared_ptr<Comment>& tx)
        {
            int count = ConsensusRepoInst.CountChainCommentEdit(*tx->GetRootTxHash());

            auto reputationConsensus = ReputationConsensusFactory::Instance(Height);
            auto[mode, reputation, balance] = reputationConsensus->GetAccountInfo(*tx->GetAddress());
            auto limit = GetEditLimit(mode);

            if (count >= limit)
                return {false, SocialConsensusResult_ContentEditLimit};

            return Success;
        }

        tuple<bool, SocialConsensusResult> CheckModel(const shared_ptr<Transaction>& tx) override
        {
            auto ptx = static_pointer_cast<CommentEdit>(tx);

            // Check required fields
            if (IsEmpty(ptx->GetAddress())) return {false, SocialConsensusResult_Failed};
            if (IsEmpty(ptx->GetPostTxHash())) return {false, SocialConsensusResult_Failed};
            if (IsEmpty(ptx->GetRootTxHash())) return {false, SocialConsensusResult_Failed};

            // Maximum for message data
            if (!ptx->GetPayload()) return {false, SocialConsensusResult_Failed};
            if (IsEmpty(ptx->GetPayloadMsg())) return {false, SocialConsensusResult_Failed};
            if (HtmlUtils::UrlDecode(*ptx->GetPayloadMsg()).length() > GetCommentMessageMaxSize())
                return {false, SocialConsensusResult_Size};

            return Success;
        }

        vector<string> GetAddressesForCheckRegistration(const PTransactionRef& tx) override
        {
            auto ptx = static_pointer_cast<CommentEdit>(tx);
            return {*ptx->GetAddress()};
        }

    };

    /*******************************************************************************************************************
    *
    *  Start checkpoint at 1180000 block
    *
    *******************************************************************************************************************/
    class CommentEditConsensus_checkpoint_1180000 : public CommentEditConsensus
    {
    public:
        CommentEditConsensus_checkpoint_1180000(int height) : CommentEditConsensus(height) {}

    protected:
        int CheckpointHeight() override { return 1180000; }

        int64_t GetEditWindow() override { return 1440; }

        bool AllowEditWindow(const PTransactionRef& ptx, const PTransactionRef& originalTx) override
        {
            auto[ok, originalTxHeight] = ConsensusRepoInst.GetTransactionHeight(*originalTx->GetHash());
            if (!ok)
                return false;

            return (Height - originalTxHeight) <= GetEditWindow();
        }
    };

    /*******************************************************************************************************************
    *
    *  Factory for select actual rules version
    *
    *******************************************************************************************************************/
    class CommentEditConsensusFactory
    {
    private:
        const std::map<int, std::function<CommentEditConsensus*(int height)>> m_rules =
            {
                {1180000, [](int height) { return new CommentEditConsensus_checkpoint_1180000(height); }},
                {0,       [](int height) { return new CommentEditConsensus(height); }},
            };
    public:
        shared_ptr<CommentEditConsensus> Instance(int height)
        {
            return shared_ptr<CommentEditConsensus>(
                (--m_rules.upper_bound(height))->second(height)
            );
        }
    };
}

#endif // POCKETCONSENSUS_COMMENT_EDIT_HPP