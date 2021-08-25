// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETCONSENSUS_COMMENT_HPP
#define POCKETCONSENSUS_COMMENT_HPP

#include "utils/html.h"
#include "pocketdb/ReputationConsensus.h"
#include "pocketdb/consensus/Social.h"
#include "pocketdb/models/dto/Comment.hpp"

namespace PocketConsensus
{
    /*******************************************************************************************************************
    *  Comment consensus base class
    *******************************************************************************************************************/
    class CommentConsensus : public SocialConsensus
    {
    public:
        CommentConsensus(int height) : SocialConsensus(height) {}
        
        ConsensusValidateResult Validate(const PTransactionRef& ptx, const PocketBlockRef& block) override
        {
            auto _ptx = static_pointer_cast<Comment>(ptx);

            // Base validation with calling block or mempool check
            if (auto[baseValidate, baseValidateCode] = SocialConsensus::Validate(ptx, block); !baseValidate)
                return {false, baseValidateCode};
                
            // Parent comment
            if (!IsEmpty(_ptx->GetParentTxHash()))
            {
                auto parentTx = PocketDb::TransRepoInst.GetByHash(*_ptx->GetParentTxHash());
                if (!parentTx)
                    return {false, SocialConsensusResult_InvalidParentComment};
            }

            // Answer comment
            if (!IsEmpty(_ptx->GetAnswerTxHash()))
            {
                auto answerTx = PocketDb::TransRepoInst.GetByHash(*_ptx->GetAnswerTxHash());
                if (!answerTx)
                    return {false, SocialConsensusResult_InvalidAnswerComment};
            }

            // Check exists content transaction
            auto contentTx = PocketDb::TransRepoInst.GetByHash(*_ptx->GetPostTxHash());
            if (!contentTx)
                return {false, SocialConsensusResult_NotFound};

            // Check Blocking
            if (auto[existsBlocking, blockingType] = PocketDb::ConsensusRepoInst.GetLastBlockingType(
                    *contentTx->GetString1(), *_ptx->GetAddress() // GetString1() returned author content
                ); existsBlocking && blockingType == ACTION_BLOCKING)
                return {false, SocialConsensusResult_Blocking};

            return Success;
        }

        ConsensusValidateResult Check(const CTransactionRef& tx, const PTransactionRef& ptx) override
        {
            auto _ptx = static_pointer_cast<Comment>(ptx);

            if (auto[baseCheck, baseCheckCode] = SocialConsensus::Check(tx, ptx); !baseCheck)
                return {false, baseCheckCode};

            // Check required fields
            if (IsEmpty(_ptx->GetAddress())) return {false, SocialConsensusResult_Failed};
            if (IsEmpty(_ptx->GetPostTxHash())) return {false, SocialConsensusResult_Failed};

            // Maximum for message data
            if (!_ptx->GetPayload()) return {false, SocialConsensusResult_Failed};
            if (IsEmpty(_ptx->GetPayloadMsg())) return {false, SocialConsensusResult_Failed};
            if (HtmlUtils::UrlDecode(*_ptx->GetPayloadMsg()).length() > GetCommentMessageMaxSize())
                return {false, SocialConsensusResult_Size};

            return Success;
        }

    protected:
        virtual int64_t GetLimitWindow() { return 86400; }
        virtual int64_t GetFullLimit() { return 300; }
        virtual int64_t GetTrialLimit() { return 150; }
        virtual size_t GetCommentMessageMaxSize() { return 2000; }
        virtual int64_t GetLimit(AccountMode mode) { return mode >= AccountMode_Full ? GetFullLimit() : GetTrialLimit(); }


        virtual bool CheckBlockLimitTime(const PTransactionRef& ptx, const PTransactionRef& blockPtx)
        {
            return *blockPtx->GetTime() <= *ptx->GetTime();
        }

        ConsensusValidateResult ValidateBlock(const PTransactionRef& ptx, const PocketBlockRef& block) override
        {
            int count = GetChainCount(ptx);
            for (auto& blockTx : *block)
            {
                if (!IsIn(*blockTx->GetType(), {CONTENT_COMMENT}))
                    continue;

                if (*blockTx->GetHash() == *ptx->GetHash())
                    continue;

                if (*ptx->GetString1() == *blockTx->GetString1())
                {
                    if (CheckBlockLimitTime(ptx, blockTx))
                        count += 1;
                }
            }

            return ValidateLimit(ptx, count);
        }

        ConsensusValidateResult ValidateMempool(const PTransactionRef& ptx) override
        {
            int count = GetChainCount(ptx);
            count += ConsensusRepoInst.CountMempoolComment(*ptx->GetString1());
            return ValidateLimit(ptx, count);
        }

        virtual ConsensusValidateResult ValidateLimit(const PTransactionRef& ptx, int count)
        {
            auto reputationConsensus = PocketConsensus::ReputationConsensusFactoryInst.Instance(Height);
            auto[mode, reputation, balance] = reputationConsensus->GetAccountInfo(*ptx->GetString1());
            if (count >= GetLimit(mode))
                return {false, SocialConsensusResult_CommentLimit};

            return Success;
        }

        virtual int GetChainCount(const PTransactionRef& ptx)
        {
            return ConsensusRepoInst.CountChainCommentTime(
                *ptx->GetString1(),
                *ptx->GetTime() - GetLimitWindow()
            );
        }

        vector<string> GetAddressesForCheckRegistration(const PTransactionRef& ptx) override
        {
            return {*ptx->GetString1()};
        }

    };

    /*******************************************************************************************************************
    *  Start checkpoint at 1124000 block
    *******************************************************************************************************************/
    class CommentConsensus_checkpoint_1124000 : public CommentConsensus
    {
    public:
        CommentConsensus_checkpoint_1124000(int height) : CommentConsensus(height) {}
    protected:
        bool CheckBlockLimitTime(const PTransactionRef& ptx, const PTransactionRef& blockPtx) override
        {
            return true;
        }
    };

    /*******************************************************************************************************************
    *  Start checkpoint at 1180000 block
    *******************************************************************************************************************/
    class CommentConsensus_checkpoint_1180000 : public CommentConsensus_checkpoint_1124000
    {
    public:
        CommentConsensus_checkpoint_1180000(int height) : CommentConsensus_checkpoint_1124000(height) {}
    protected:
        int64_t GetLimitWindow() override { return 1440; }
        int GetChainCount(const PTransactionRef& ptx) override
        {
            return ConsensusRepoInst.CountChainCommentHeight(*ptx->GetString1(), Height - (int) GetLimitWindow());
        }
    };

    /*******************************************************************************************************************
    *  Factory for select actual rules version
    *******************************************************************************************************************/
    class CommentConsensusFactory : public SocialConsensusFactory
    {
    private:
        const vector<ConsensusCheckpoint> _rules = {
            {0,       -1, [](int height) { return make_shared<CommentConsensus>(height); }},
            {1124000, -1, [](int height) { return make_shared<CommentConsensus_checkpoint_1124000>(height); }},
            {1180000,  0, [](int height) { return make_shared<CommentConsensus_checkpoint_1180000>(height); }},
        };
    protected:
        const vector<ConsensusCheckpoint>& m_rules() override { return _rules; }
    };
}

#endif // POCKETCONSENSUS_COMMENT_HPP