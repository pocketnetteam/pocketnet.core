// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETCONSENSUS_SCORECONTENT_HPP
#define POCKETCONSENSUS_SCORECONTENT_HPP

#include "pocketdb/ReputationConsensus.h"
#include "pocketdb/consensus/Social.h"
#include "pocketdb/models/base/Transaction.h"
#include "pocketdb/models/dto/ScoreContent.hpp"

namespace PocketConsensus
{
    /*******************************************************************************************************************
    *  ScorePost consensus base class
    *******************************************************************************************************************/
    class ScoreContentConsensus : public SocialConsensus
    {
    public:
        ScoreContentConsensus(int height) : SocialConsensus(height) {}

        ConsensusValidateResult Validate(const PTransactionRef& ptx, const PocketBlockRef& block) override
        {
            auto _ptx = static_pointer_cast<ScoreContent>(ptx);

            // Base validation with calling block or mempool check
            if (auto[baseValidate, baseValidateCode] = SocialConsensus::Validate(ptx, block); !baseValidate)
                return {false, baseValidateCode};
                
            // Check already scored content
            if (PocketDb::ConsensusRepoInst.ExistsScore(
                *_ptx->GetAddress(), *_ptx->GetContentTxHash(), ACTION_SCORE_CONTENT, false))
                return {false, SocialConsensusResult_DoubleScore};

            // Content should be exists in chain
            auto[lastContentOk, lastContent] = PocketDb::ConsensusRepoInst.GetLastContent(*_ptx->GetContentTxHash());
            if (!lastContentOk && block)
            {
                // ... or in block
                for (auto& blockTx : *block)
                {
                    if (!IsIn(*blockTx->GetType(), {CONTENT_POST, CONTENT_VIDEO}))
                        continue;

                    if (*blockTx->GetString2() == *_ptx->GetContentTxHash())
                    {
                        lastContent = blockTx;
                        break;
                    }
                }
            }
            if (!lastContent)
                return {false, SocialConsensusResult_NotFound};

            // Check score to self
            // lastContent->GetString1() - author address post
            if (*_ptx->GetAddress() == *lastContent->GetString1())
                return {false, SocialConsensusResult_SelfScore};

            // Check Blocking
            if (auto[ok, result] = ValidateBlocking(*lastContent->GetString1(), _ptx); !ok)
                return {false, result};

            return Success;
        }

        ConsensusValidateResult Check(const CTransactionRef& tx, const PTransactionRef& ptx) override
        {
            auto _ptx = static_pointer_cast<ScoreContent>(ptx);

            if (auto[baseCheck, baseCheckCode] = SocialConsensus::Check(tx, ptx); !baseCheck)
                return {false, baseCheckCode};

            // Check required fields
            if (IsEmpty(_ptx->GetAddress())) return {false, SocialConsensusResult_Failed};
            if (IsEmpty(_ptx->GetContentTxHash())) return {false, SocialConsensusResult_Failed};
            if (IsEmpty(_ptx->GetValue())) return {false, SocialConsensusResult_Failed};

            auto value = *_ptx->GetValue();
            if (value < 1 || value > 5)
                return {false, SocialConsensusResult_Failed};

            // TODO (brangr): implement - пробросить сюда исходную транзакцию для сверки хешей
            // + добавить чекпойнт с принудительным включением проверки данные по автору поста и значению лайка
            // по сути нужно пробрасывать хэш из транзакции всегда
            // Check OP_RETURN with Payload
            //if (IsEmpty(_ptx->GetOPRAddress()) || *_ptx->GetOPRAddress() != *_ptx->GetAddress())
            //    LogPrintf("000 CHECKPOINT 1 %s\n", *_ptx->GetHash());
            //return {false, SocialConsensusResult_OpReturnFailed};
            //if (IsEmpty(_ptx->GetOPRValue()) || *_ptx->GetOPRValue() != *_ptx->GetValue())
            //    LogPrintf("000 CHECKPOINT 2 %s\n", *_ptx->GetHash());
            //return {false, SocialConsensusResult_OpReturnFailed};

            return Success;
        }

    protected:
        virtual int64_t GetLimitWindow() { return 86400; }
        virtual int64_t GetFullLimit() { return 90; }
        virtual int64_t GetTrialLimit() { return 45; }
        virtual int64_t GetScoresLimit(AccountMode mode) { return mode >= AccountMode_Full ? GetFullLimit() : GetTrialLimit(); }

        virtual bool CheckBlockLimitTime(const PTransactionRef& ptx, const PTransactionRef& blockTx)
        {
            return *blockTx->GetTime() <= *ptx->GetTime();
        }

        ConsensusValidateResult ValidateBlock(const PTransactionRef& ptx, const PocketBlockRef& block) override
        {
            auto _ptx = static_pointer_cast<ScoreContent>(ptx);

            // Get count from chain
            int count = GetChainCount(ptx);

            // Get count from block
            for (auto& blockTx : *block)
            {
                if (!IsIn(*blockTx->GetType(), {ACTION_SCORE_CONTENT}))
                    continue;

                if (*blockTx->GetHash() == *_ptx->GetHash())
                    continue;

                if (*_ptx->GetAddress() == *blockTx->GetString1())
                {
                    if (CheckBlockLimitTime(_ptx, blockTx))
                        count += 1;

                    if (*blockTx->GetHash() == *_ptx->GetContentTxHash())
                        return {false, SocialConsensusResult_DoubleScore};
                }
            }

            // Check count
            return ValidateLimit(ptx, count);
        }

        ConsensusValidateResult ValidateMempool(const PTransactionRef& ptx) override
        {
            auto _ptx = static_pointer_cast<ScoreContent>(ptx);

            // Check already scored content
            if (PocketDb::ConsensusRepoInst.ExistsScore(
                *_ptx->GetAddress(), *_ptx->GetContentTxHash(), ACTION_SCORE_CONTENT, true))
                return {false, SocialConsensusResult_DoubleScore};

            // Get count from chain
            int count = GetChainCount(_ptx);

            // Get count from mempool
            count += ConsensusRepoInst.CountMempoolScoreContent(*_ptx->GetAddress());

            // Check count
            return ValidateLimit(_ptx, count);
        }

        virtual bool ValidateLowReputation(const PTransactionRef& ptx, AccountMode mode)
        {
            return true;
        }

        virtual ConsensusValidateResult ValidateLimit(const PTransactionRef& ptx, int count)
        {
            auto _ptx = static_pointer_cast<ScoreContent>(ptx);
            auto reputationConsensus = PocketConsensus::ReputationConsensusFactoryInst.Instance(Height);
            auto accountMode = reputationConsensus->GetAccountMode(*_ptx->GetAddress());
            if (count >= GetScoresLimit(accountMode))
                return {false, SocialConsensusResult_ScoreLimit};

            if (!ValidateLowReputation(_ptx, accountMode))
                return {false, SocialConsensusResult_LowReputation};

            return Success;
        }

        virtual ConsensusValidateResult ValidateBlocking(const string& contentAddress, const PTransactionRef& ptx)
        {
            return Success;
        }

        virtual int GetChainCount(const PTransactionRef& ptx)
        {
            auto _ptx = static_pointer_cast<ScoreContent>(ptx);
            return ConsensusRepoInst.CountChainScoreContentTime(
                *_ptx->GetAddress(),
                *_ptx->GetTime() - GetLimitWindow()
            );
        }

        vector<string> GetAddressesForCheckRegistration(const PTransactionRef& ptx) override
        {
            auto _ptx = static_pointer_cast<ScoreContent>(ptx);
            return {*_ptx->GetAddress()};
        }

    };

    /*******************************************************************************************************************
    *  Consensus checkpoint at 175600 block
    *******************************************************************************************************************/
    class ScoreContentConsensus_checkpoint_175600 : public ScoreContentConsensus
    {
    public:
        ScoreContentConsensus_checkpoint_175600(int height) : ScoreContentConsensus(height) {}
    protected:
        int64_t GetFullLimit() override { return 200; }
        int64_t GetTrialLimit() override { return 100; }
    };

    /*******************************************************************************************************************
    *
    *  Consensus checkpoint at 430000 block
    *
    *******************************************************************************************************************/
    class ScoreContentConsensus_checkpoint_430000 : public ScoreContentConsensus_checkpoint_175600
    {
    public:
        ScoreContentConsensus_checkpoint_430000(int height) : ScoreContentConsensus_checkpoint_175600(height) {}
    protected:
        ConsensusValidateResult ValidateBlocking(const string& contentAddress, const PTransactionRef& ptx) override
        {
            auto _ptx = static_pointer_cast<ScoreContent>(ptx);
            auto[existsBlocking, blockingType] = PocketDb::ConsensusRepoInst.GetLastBlockingType(
                contentAddress,
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
    class ScoreContentConsensus_checkpoint_514184 : public ScoreContentConsensus_checkpoint_430000
    {
    public:
        ScoreContentConsensus_checkpoint_514184(int height) : ScoreContentConsensus_checkpoint_430000(height) {}
    protected:
        ConsensusValidateResult ValidateBlocking(const string& contentAddress, const PTransactionRef& ptx) override
        {
            return Success;
        }
    };

    /*******************************************************************************************************************
    *  Start checkpoint at 1124000 block
    *******************************************************************************************************************/
    class ScoreContentConsensus_checkpoint_1124000 : public ScoreContentConsensus_checkpoint_514184
    {
    public:
        ScoreContentConsensus_checkpoint_1124000(int height) : ScoreContentConsensus_checkpoint_514184(height) {}
    protected:
        bool CheckBlockLimitTime(const PTransactionRef& ptx, const PTransactionRef& blockPtx) override
        {
            return true;
        }
    };

    /*******************************************************************************************************************
    *  Start checkpoint at 1180000 block
    *******************************************************************************************************************/
    class ScoreContentConsensus_checkpoint_1180000 : public ScoreContentConsensus_checkpoint_1124000
    {
    public:
        ScoreContentConsensus_checkpoint_1180000(int height) : ScoreContentConsensus_checkpoint_1124000(height) {}
    protected:
        int64_t GetLimitWindow() override { return 1440; }
        int GetChainCount(const PTransactionRef& ptx) override
        {
            auto _ptx = static_pointer_cast<ScoreContent>(ptx);
            return ConsensusRepoInst.CountChainScoreContentHeight(
                *_ptx->GetAddress(),
                Height - (int) GetLimitWindow()
            );
        }
    };

    /*******************************************************************************************************************
    *  Start checkpoint at 1324655 block
    *******************************************************************************************************************/
    class ScoreContentConsensus_checkpoint_1324655 : public ScoreContentConsensus_checkpoint_1180000
    {
    public:
        ScoreContentConsensus_checkpoint_1324655(int height) : ScoreContentConsensus_checkpoint_1180000(height) {}
    protected:
        int64_t GetTrialLimit() override { return 15; }
        bool ValidateLowReputation(const PTransactionRef& ptx, AccountMode mode) override
        {
            auto _ptx = static_pointer_cast<ScoreContent>(ptx);
            return (mode != AccountMode_Trial || (*_ptx->GetValue() == 1 || *_ptx->GetValue() == 2));
        }
    };

    /*******************************************************************************************************************
    *  Factory for select actual rules version
    *******************************************************************************************************************/
    class ScoreContentConsensusFactory : public SocialConsensusFactory
    {
    private:
        const vector<ConsensusCheckpoint> _rules = {
            {0,       -1, [](int height) { return make_shared<ScoreContentConsensus>(height); }},
            {175600,  -1, [](int height) { return make_shared<ScoreContentConsensus_checkpoint_175600>(height); }},
            {430000,  -1, [](int height) { return make_shared<ScoreContentConsensus_checkpoint_430000>(height); }},
            {514184,  -1, [](int height) { return make_shared<ScoreContentConsensus_checkpoint_514184>(height); }},
            {1124000, -1, [](int height) { return make_shared<ScoreContentConsensus_checkpoint_1124000>(height); }},
            {1180000, -1, [](int height) { return make_shared<ScoreContentConsensus_checkpoint_1180000>(height); }},
            {1324655, 0,  [](int height) { return make_shared<ScoreContentConsensus_checkpoint_1324655>(height); }},
        };
    protected:
        const vector<ConsensusCheckpoint>& m_rules() override { return _rules; }
    };
}

#endif // POCKETCONSENSUS_SCORECONTENT_HPP