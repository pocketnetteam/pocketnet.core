// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETCONSENSUS_SCORECOMMENT_HPP
#define POCKETCONSENSUS_SCORECOMMENT_HPP

#include "pocketdb/consensus/Social.h"
#include "pocketdb/models/dto/ScoreComment.hpp"
#include "pocketdb/ReputationConsensus.h"

namespace PocketConsensus
{
    /*******************************************************************************************************************
    *  ScoreComment consensus base class
    *******************************************************************************************************************/
    class ScoreCommentConsensus : public SocialConsensus
    {
    public:
        ScoreCommentConsensus(int height) : SocialConsensus(height) {}

        ConsensusValidateResult Validate(const PTransactionRef& ptx, const PocketBlockRef& block) override
        {
            auto _ptx = static_pointer_cast<ScoreComment>(ptx);

            // Base validation with calling block or mempool check
            if (auto[baseValidate, baseValidateCode] = SocialConsensus::Validate(ptx, block); !baseValidate)
                return {false, baseValidateCode};

            // Check already scored content
            if (PocketDb::ConsensusRepoInst.ExistsScore(
                *_ptx->GetAddress(), *_ptx->GetCommentTxHash(), ACTION_SCORE_COMMENT, false))
                return {false, SocialConsensusResult_DoubleCommentScore};

            // Comment should be exists
            auto[lastContentOk, lastContent] = PocketDb::ConsensusRepoInst.GetLastContent(*_ptx->GetCommentTxHash());
            if (!lastContentOk && block)
            {
                // ... or in block
                for (auto& blockTx : *block)
                {
                    if (!IsIn(*blockTx->GetType(), {CONTENT_COMMENT, CONTENT_COMMENT_EDIT, CONTENT_COMMENT_DELETE}))
                        continue;

                    if (*blockTx->GetString2() == *_ptx->GetCommentTxHash())
                    {
                        lastContent = blockTx;
                        break;
                    }
                }
            }
            if (!lastContent)
                return {false, SocialConsensusResult_NotFound};

            // Scores to deleted comments not allowed
            if (*lastContent->GetType() == PocketTxType::CONTENT_COMMENT_DELETE)
            {
                PocketHelpers::SocialCheckpoints socialCheckpoints;
                if (!socialCheckpoints.IsCheckpoint(*_ptx->GetHash(), *_ptx->GetType(), SocialConsensusResult_NotFound))
                    return {false, SocialConsensusResult_NotFound};
            }

            // Check score to self
            if (*_ptx->GetAddress() == *lastContent->GetString1())
                return {false, SocialConsensusResult_SelfCommentScore};

            // Check Blocking
            if (auto[ok, result] = ValidateBlocking(*lastContent->GetString1(), _ptx); !ok)
                return {false, result};

            return Success;
        }

        ConsensusValidateResult Check(const CTransactionRef& tx, const PTransactionRef& ptx) override
        {
            auto _ptx = static_pointer_cast<ScoreComment>(ptx);

            if (auto[baseCheck, baseCheckCode] = SocialConsensus::Check(tx, ptx); !baseCheck)
                return {false, baseCheckCode};

            // Check required fields
            if (IsEmpty(_ptx->GetAddress())) return {false, SocialConsensusResult_Failed};
            if (IsEmpty(_ptx->GetCommentTxHash())) return {false, SocialConsensusResult_Failed};
            if (IsEmpty(_ptx->GetValue())) return {false, SocialConsensusResult_Failed};

            auto value = *_ptx->GetValue();
            if (value != 1 && value != -1)
                return {false, SocialConsensusResult_Failed};

            // TODO (brangr): DEBUG!
            // по сути нужно пробрасывать хэш из транзакции всегда
            // Check OP_RETURN with Payload
            //if (IsEmpty(_ptx->GetOPRAddress()) || *_ptx->GetOPRAddress() != *_ptx->GetAddress())
            //    LogPrintf("000 CHECKPOINT 11 %s\n", *_ptx->GetHash());
            //  return {false, SocialConsensusResult_OpReturnFailed};
            //if (IsEmpty(_ptx->GetOPRValue()) || *_ptx->GetOPRValue() != *_ptx->GetValue())
            //    LogPrintf("000 CHECKPOINT 22 %s\n", *_ptx->GetHash());
            //  return {false, SocialConsensusResult_OpReturnFailed};

            return Success;
        }

    protected:
        virtual int64_t GetLimitWindow() { return 86400; }
        virtual int64_t GetFullAccountScoresLimit() { return 600; }
        virtual int64_t GetTrialAccountScoresLimit() { return 300; }
        virtual int64_t GetScoresLimit(AccountMode mode)
        {
            return mode >= AccountMode_Full ? GetFullAccountScoresLimit() : GetTrialAccountScoresLimit();
        }

        virtual bool CheckBlockLimitTime(const PTransactionRef& ptx, const PTransactionRef& blockTx)
        {
            return *blockTx->GetTime() <= *ptx->GetTime();
        }

        ConsensusValidateResult ValidateBlock(const PTransactionRef& ptx, const PocketBlockRef& block) override
        {
            auto _ptx = static_pointer_cast<ScoreComment>(ptx);

            // Get count from chain
            int count = GetChainCount(ptx);

            // Get count from block
            for (auto& blockTx : *block)
            {
                if (!IsIn(*blockTx->GetType(), {ACTION_SCORE_COMMENT}))
                    continue;

                if (*blockTx->GetHash() == *ptx->GetHash())
                    continue;

                if (*_ptx->GetAddress() == *blockTx->GetString1())
                {
                    if (CheckBlockLimitTime(_ptx, blockTx))
                        count += 1;

                    if (*blockTx->GetHash() == *_ptx->GetCommentTxHash())
                        return {false, SocialConsensusResult_DoubleCommentScore};
                }
            }

            return ValidateLimit(ptx, count);
        }

        ConsensusValidateResult ValidateMempool(const PTransactionRef& ptx) override
        {
            auto _ptx = static_pointer_cast<ScoreComment>(ptx);

            // Check already scored content
            if (PocketDb::ConsensusRepoInst.ExistsScore(
                *_ptx->GetAddress(), *_ptx->GetCommentTxHash(), ACTION_SCORE_COMMENT, true))
                return {false, SocialConsensusResult_DoubleCommentScore};

            // Check count from chain
            int count = GetChainCount(_ptx);

            // and from mempool
            count += ConsensusRepoInst.CountMempoolScoreComment(*_ptx->GetAddress());

            return ValidateLimit(ptx, count);
        }

        virtual ConsensusValidateResult ValidateLimit(const PTransactionRef& ptx, int count)
        {
            auto _ptx = static_pointer_cast<ScoreComment>(ptx);

            auto reputationConsensus = PocketConsensus::ReputationConsensusFactoryInst.Instance(Height);
            auto accountMode = reputationConsensus->GetAccountMode(*_ptx->GetAddress());
            if (count >= GetScoresLimit(accountMode))
                return {false, SocialConsensusResult_CommentScoreLimit};

            return Success;
        }

        virtual ConsensusValidateResult ValidateBlocking(const string& commentAddress, const PTransactionRef& tx)
        {
            return Success;
        }

        virtual int GetChainCount(const PTransactionRef& ptx)
        {
            auto _ptx = static_pointer_cast<ScoreComment>(ptx);

            return ConsensusRepoInst.CountChainScoreCommentTime(
                *_ptx->GetAddress(),
                *_ptx->GetTime() - GetLimitWindow()
            );
        }

        vector<string> GetAddressesForCheckRegistration(const PTransactionRef& ptx) override
        {
            auto _ptx = static_pointer_cast<ScoreComment>(ptx);
            return {*_ptx->GetAddress()};
        }
    };

    /*******************************************************************************************************************
    *  Consensus checkpoint at 430000 block
    *******************************************************************************************************************/
    class ScoreCommentConsensus_checkpoint_430000 : public ScoreCommentConsensus
    {
    public:
        ScoreCommentConsensus_checkpoint_430000(int height) : ScoreCommentConsensus(height) {}
    protected:
        ConsensusValidateResult ValidateBlocking(const string& commentAddress, const PTransactionRef& ptx) override
        {
            auto _ptx = static_pointer_cast<ScoreComment>(ptx);

            auto[existsBlocking, blockingType] = PocketDb::ConsensusRepoInst.GetLastBlockingType(
                commentAddress,
                *_ptx->GetAddress()
            );

            if (existsBlocking && blockingType == ACTION_BLOCKING)
                return {false, SocialConsensusResult_Blocking};

            return Success;
        }
    };

    /*******************************************************************************************************************
    *  Consensus checkpoint at 514184 block
    *******************************************************************************************************************/
    class ScoreCommentConsensus_checkpoint_514184 : public ScoreCommentConsensus_checkpoint_430000
    {
    public:
        ScoreCommentConsensus_checkpoint_514184(int height) : ScoreCommentConsensus_checkpoint_430000(height) {}
    protected:
        ConsensusValidateResult ValidateBlocking(const string& commentAddress, const PTransactionRef& ptx) override
        {
            return Success;
        }
    };

    /*******************************************************************************************************************
    *  Start checkpoint at 1124000 block
    *******************************************************************************************************************/
    class ScoreCommentConsensus_checkpoint_1124000 : public ScoreCommentConsensus_checkpoint_514184
    {
    public:
        ScoreCommentConsensus_checkpoint_1124000(int height) : ScoreCommentConsensus_checkpoint_514184(height) {}
    protected:
        bool CheckBlockLimitTime(const PTransactionRef& ptx, const PTransactionRef& blockPtx) override
        {
            return true;
        }
    };

    /*******************************************************************************************************************
    *  Start checkpoint at 1180000 block
    *******************************************************************************************************************/
    class ScoreCommentConsensus_checkpoint_1180000 : public ScoreCommentConsensus_checkpoint_1124000
    {
    public:
        ScoreCommentConsensus_checkpoint_1180000(int height) : ScoreCommentConsensus_checkpoint_1124000(height) {}
    protected:
        int64_t GetLimitWindow() override { return 1440; }
        int GetChainCount(const PTransactionRef& ptx) override
        {
            auto _ptx = static_pointer_cast<ScoreComment>(ptx);

            return ConsensusRepoInst.CountChainScoreCommentHeight(
                *_ptx->GetAddress(),
                Height - (int) GetLimitWindow()
            );
        }
    };

    /*******************************************************************************************************************
    *  Factory for select actual rules version
    *******************************************************************************************************************/
    class ScoreCommentConsensusFactory : public SocialConsensusFactory
    {
    private:
        const vector<ConsensusCheckpoint> _rules = {
            {0,       -1, [](int height) { return make_shared<ScoreCommentConsensus>(height); }},
            {430000,  -1, [](int height) { return make_shared<ScoreCommentConsensus_checkpoint_430000>(height); }},
            {514184,  -1, [](int height) { return make_shared<ScoreCommentConsensus_checkpoint_514184>(height); }},
            {1124000, -1, [](int height) { return make_shared<ScoreCommentConsensus_checkpoint_1124000>(height); }},
            {1180000, 0,  [](int height) { return make_shared<ScoreCommentConsensus_checkpoint_1180000>(height); }},
        };
    protected:
        const vector<ConsensusCheckpoint>& m_rules() override { return _rules; }
    };
}

#endif // POCKETCONSENSUS_SCORECOMMENT_HPP