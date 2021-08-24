// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETCONSENSUS_COMMENT_HPP
#define POCKETCONSENSUS_COMMENT_HPP

#include "utils/html.h"
#include "pocketdb/ReputationConsensus.h"
#include "pocketdb/consensus/Social.hpp"
#include "pocketdb/models/dto/Comment.hpp"

namespace PocketConsensus
{
    typedef shared_ptr<Comment> CommentRef;

    /*******************************************************************************************************************
    *  Comment consensus base class
    *******************************************************************************************************************/
    class CommentConsensus : public SocialConsensus<CommentRef>
    {
    public:
        CommentConsensus(int height) : SocialConsensus(height) {}
        
        ConsensusValidateResult Validate(const CommentRef& ptx, const PocketBlockRef& block) override
        {
            // Base validation with calling block or mempool check
            if (auto[baseValidate, baseValidateCode] = SocialConsensus::Validate(ptx, block); !baseValidate)
                return {false, baseValidateCode};
                
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

        ConsensusValidateResult Check(const CTransactionRef& tx, const CommentRef& ptx) override
        {
            if (auto[baseCheck, baseCheckCode] = SocialConsensus::Check(tx, ptx); !baseCheck)
                return {false, baseCheckCode};

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

    protected:
        virtual int64_t GetLimitWindow() { return 86400; }
        virtual int64_t GetFullLimit() { return 300; }
        virtual int64_t GetTrialLimit() { return 150; }
        virtual size_t GetCommentMessageMaxSize() { return 2000; }
        virtual int64_t GetLimit(AccountMode mode) { return mode >= AccountMode_Full ? GetFullLimit() : GetTrialLimit(); }


        virtual bool CheckBlockLimitTime(const CommentRef& ptx, const PTransactionRef& blockPtx)
        {
            return *blockPtx->GetTime() <= *ptx->GetTime();
        }

        ConsensusValidateResult ValidateBlock(const CommentRef& ptx, const PocketBlockRef& block) override
        {
            int count = GetChainCount(ptx);
            for (auto& blockTx : *block)
            {
                if (!IsIn(*blockTx->GetType(), {CONTENT_COMMENT}))
                    continue;

                if (*blockTx->GetHash() == *ptx->GetHash())
                    continue;

                auto blockPtx = static_pointer_cast<Comment>(blockTx);
                if (*ptx->GetAddress() == *blockPtx->GetAddress())
                {
                    if (CheckBlockLimitTime(ptx, blockTx))
                        count += 1;
                }
            }

            return ValidateLimit(ptx, count);
        }

        ConsensusValidateResult ValidateMempool(const CommentRef& ptx) override
        {
            int count = GetChainCount(ptx);
            count += ConsensusRepoInst.CountMempoolComment(*ptx->GetAddress());
            return ValidateLimit(ptx, count);
        }

        virtual ConsensusValidateResult ValidateLimit(const CommentRef& ptx, int count)
        {
            auto reputationConsensus = PocketConsensus::ReputationConsensusFactoryInst.Instance(Height);
            auto[mode, reputation, balance] = reputationConsensus->GetAccountInfo(*ptx->GetAddress());
            if (count >= GetLimit(mode))
                return {false, SocialConsensusResult_CommentLimit};

            return Success;
        }

        virtual int GetChainCount(const CommentRef& ptx)
        {
            return ConsensusRepoInst.CountChainCommentTime(
                *ptx->GetAddress(),
                *ptx->GetTime() - GetLimitWindow()
            );
        }

        vector<string> GetAddressesForCheckRegistration(const CommentRef& tx) override
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
        bool CheckBlockLimitTime(const CommentRef& ptx, const PTransactionRef& blockPtx) override
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
        int64_t GetLimitWindow() override { return 1440; }
        int GetChainCount(const CommentRef& ptx) override
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
    class CommentConsensusFactory : public SocialConsensusFactory
    {
    private:
        const vector<ConsensusCheckpoint> _rules =
        {
            {0,       -1, [](int height) { return make_shared<CommentConsensus>(height); }},
            {1124000, -1, [](int height) { return make_shared<CommentConsensus_checkpoint_1124000>(height); }},
            {1180000,  0, [](int height) { return make_shared<CommentConsensus_checkpoint_1180000>(height); }},
        };
    protected:
        const vector<ConsensusCheckpoint>& m_rules() override { return _rules; }
    };
}

#endif // POCKETCONSENSUS_COMMENT_HPP