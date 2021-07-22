// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 Bitcoin developers
// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETCONSENSUS_COMMENT_HPP
#define POCKETCONSENSUS_COMMENT_HPP

#include <pocketdb/consensus/Reputation.hpp>
#include "utils/html.h"

#include "pocketdb/consensus/social/Base.hpp"
#include "pocketdb/models/dto/Comment.hpp"

namespace PocketConsensus
{
    /*******************************************************************************************************************
    *
    *  Comment consensus base class
    *
    *******************************************************************************************************************/
    class CommentConsensus : public SocialBaseConsensus
    {
    public:
        CommentConsensus(int height) : SocialBaseConsensus(height) {}

    protected:

        virtual int64_t GetLimitWindow() { return 86400; }

        virtual int64_t GetFullLimit() { return 300; }

        virtual int64_t GetTrialLimit() { return 150; }

        virtual int64_t GetLimit(AccountMode mode)
        {
            return mode == AccountMode_Full ? GetFullLimit() : GetTrialLimit();
        }

        virtual int64_t GetCommentMessageMaxSize() { return 2000; }


        tuple<bool, SocialConsensusResult> ValidateModel(const shared_ptr<Transaction>& tx) override
        {
            auto ptx = static_pointer_cast<Comment>(tx);

            // Parent comment
            if (!IsEmpty(ptx->GetParentTxHash()))
            {
                auto parentTx = PocketDb::TransRepoInst.GetByHash(*ptx->GetParentTxHash());
                if (!parentTx)
                    return {false, SocialConsensusResult_InvalidParentComment};
            }

            // Answer comment
            if (!IsEmpty(ptx->GetAnswerTxHash()))
            {
                auto answerTx = PocketDb::TransRepoInst.GetByHash(*ptx->GetAnswerTxHash());
                if (!answerTx)
                    return {false, SocialConsensusResult_InvalidAnswerComment};
            }

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

        virtual bool CheckBlockLimitTime(const PTransactionRef& ptx, const PTransactionRef& blockPtx)
        {
            return *blockPtx->GetTime() <= *ptx->GetTime();
        }

        tuple<bool, SocialConsensusResult> ValidateLimit(const shared_ptr<Transaction>& tx,
                                                         const PocketBlock& block) override
        {
            auto ptx = static_pointer_cast<Comment>(tx);

            // Get count from chain
            int count = GetChainCount(ptx);

            // Get count from block
            for (auto blockTx : block)
            {
                if (!IsIn(*blockTx->GetType(), {CONTENT_COMMENT}))
                    continue;

                if (*blockTx->GetHash() == *ptx->GetHash())
                    continue;

                auto blockPtx = static_pointer_cast<Comment>(blockTx);
                if (*ptx->GetAddress() == *blockPtx->GetAddress())
                {
                    if (CheckBlockLimitTime(tx, blockTx))
                        count += 1;
                }
            }

            return ValidateLimit(ptx, count);
        }

        tuple<bool, SocialConsensusResult> ValidateLimit(const shared_ptr<Transaction>& tx) override
        {
            auto ptx = static_pointer_cast<Comment>(tx);

            // Get count from chain
            int count = GetChainCount(ptx);

            // Get count from mempool
            count += ConsensusRepoInst.CountMempoolComment(*ptx->GetAddress());

            return ValidateLimit(ptx, count);
        }

        virtual tuple<bool, SocialConsensusResult> ValidateLimit(const shared_ptr<Comment>& tx, int count)
        {
            auto reputationConsensus = ReputationConsensusFactory::Instance(Height);
            auto[mode, reputation, balance] = reputationConsensus->GetAccountInfo(*tx->GetAddress());
            auto limit = GetLimit(mode);

            if (count >= limit)
                return {false, SocialConsensusResult_ContentLimit};

            return Success;
        }

        virtual int GetChainCount(const shared_ptr<Comment>& ptx)
        {
            return ConsensusRepoInst.CountChainCommentTime(
                *ptx->GetAddress(),
                *ptx->GetTime() - GetLimitWindow()
            );
        }

        tuple<bool, SocialConsensusResult> CheckModel(const shared_ptr<Transaction>& tx) override
        {
            auto ptx = static_pointer_cast<Comment>(tx);

            // Check required fields
            if (IsEmpty(ptx->GetAddress())) return {false, SocialConsensusResult_Failed};
            if (IsEmpty(ptx->GetPostTxHash())) return {false, SocialConsensusResult_Failed};

            // Maximum for message data
            if (!ptx->GetPayload()) return {false, SocialConsensusResult_Failed};
            if (IsEmpty(ptx->GetPayloadMsg())) return {false, SocialConsensusResult_Failed};
            if (HtmlUtils::UrlDecode(*ptx->GetPayloadMsg()).length() > GetCommentMessageMaxSize())
                return {false, SocialConsensusResult_Size};

            return Success;
        }

        vector<string> GetAddressesForCheckRegistration(const PTransactionRef& tx) override
        {
            auto ptx = static_pointer_cast<Comment>(tx);
            return {*ptx->GetAddress()};
        }

    };

    /*******************************************************************************************************************
    *
    *  Start checkpoint at 1124000 block
    *
    *******************************************************************************************************************/
    class CommentConsensus_checkpoint_1124000 : public CommentConsensus
    {
    public:
        CommentConsensus_checkpoint_1124000(int height) : CommentConsensus(height) {}

    protected:
        int CheckpointHeight() override { return 1124000; }

        bool CheckBlockLimitTime(const PTransactionRef& ptx, const PTransactionRef& blockPtx) override
        {
            return true;
        }
    };

    /*******************************************************************************************************************
    *
    *  Start checkpoint at 1180000 block
    *
    *******************************************************************************************************************/
    class CommentConsensus_checkpoint_1180000 : public CommentConsensus_checkpoint_1124000
    {
    public:
        CommentConsensus_checkpoint_1180000(int height) : CommentConsensus_checkpoint_1124000(height) {}

    protected:
        int CheckpointHeight() override { return 1180000; }

        int64_t GetLimitWindow() override { return 1440; }

        int GetChainCount(const shared_ptr<Comment>& ptx) override
        {
            return ConsensusRepoInst.CountChainCommentHeight(
                *ptx->GetAddress(),
                Height - (int) GetLimitWindow()
            );
        }
    };

    /*******************************************************************************************************************
    *
    *  Factory for select actual rules version
    *
    *******************************************************************************************************************/
    class CommentConsensusFactory
    {
    private:
        static inline const std::map<int, std::function<CommentConsensus*(int height)>> m_rules =
            {
                {1180000, [](int height) { return new CommentConsensus_checkpoint_1180000(height); }},
                {1124000, [](int height) { return new CommentConsensus_checkpoint_1124000(height); }},
                {0,       [](int height) { return new CommentConsensus(height); }},
            };
    public:
        shared_ptr<CommentConsensus> Instance(int height)
        {
            return shared_ptr<CommentConsensus>(
                (--m_rules.upper_bound(height))->second(height)
            );
        }
    };
}

#endif // POCKETCONSENSUS_COMMENT_HPP