// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETCONSENSUS_SCORECONTENT_HPP
#define POCKETCONSENSUS_SCORECONTENT_HPP

#include "pocketdb/ReputationConsensus.h"
#include "pocketdb/consensus/Social.h"
#include "pocketdb/models/dto/ScoreContent.hpp"

namespace PocketConsensus
{
    using namespace std;
    typedef shared_ptr<ScoreContent> ScoreContentRef;

    /*******************************************************************************************************************
    *  ScorePost consensus base class
    *******************************************************************************************************************/
    class ScoreContentConsensus : public SocialConsensus<ScoreContent>
    {
    public:
        ScoreContentConsensus(int height) : SocialConsensus<ScoreContent>(height) {}

        ConsensusValidateResult Validate(const ScoreContentRef& ptx, const PocketBlockRef& block) override
        {
            // Base validation with calling block or mempool check
            if (auto[baseValidate, baseValidateCode] = SocialConsensus::Validate(ptx, block); !baseValidate)
                return {false, baseValidateCode};

            // Check already scored content
            if (PocketDb::ConsensusRepoInst.ExistsScore(
                *ptx->GetAddress(), *ptx->GetContentTxHash(), ACTION_SCORE_CONTENT, false))
                return {false, SocialConsensusResult_DoubleScore};

            // Content should be exists in chain
            auto[lastContentOk, lastContent] = PocketDb::ConsensusRepoInst.GetLastContent(*ptx->GetContentTxHash());
            if (!lastContentOk && block)
            {
                // ... or in block
                for (auto& blockTx : *block)
                {
                    if (!IsIn(*blockTx->GetType(), {CONTENT_POST, CONTENT_VIDEO}))
                        continue;

                    auto blockPtx = static_pointer_cast<ScoreContent>(blockTx);

                    if (*blockPtx->GetContentTxHash() == *ptx->GetContentTxHash())
                    {
                        lastContent = blockTx;
                        break;
                    }
                }
            }
            if (!lastContent)
                return {false, SocialConsensusResult_NotFound};

            auto lastContentPtx = static_pointer_cast<ScoreContent>(lastContent);

            // Check score to self
            if (*ptx->GetAddress() == *lastContentPtx->GetAddress())
                return {false, SocialConsensusResult_SelfScore};

            // Check Blocking
            if (auto[ok, result] = ValidateBlocking(*lastContentPtx->GetAddress(), ptx); !ok)
                return {false, result};

            return Success;
        }

        ConsensusValidateResult Check(const CTransactionRef& tx, const ScoreContentRef& ptx) override
        {

            if (auto[baseCheck, baseCheckCode] = SocialConsensus::Check(tx, ptx); !baseCheck)
                return {false, baseCheckCode};

            // Check required fields
            if (IsEmpty(ptx->GetAddress())) return {false, SocialConsensusResult_Failed};
            if (IsEmpty(ptx->GetContentTxHash())) return {false, SocialConsensusResult_Failed};
            if (IsEmpty(ptx->GetValue())) return {false, SocialConsensusResult_Failed};

            auto value = *ptx->GetValue();
            if (value < 1 || value > 5)
                return {false, SocialConsensusResult_Failed};

            // TODO (brangr): implement - пробросить сюда исходную транзакцию для сверки хешей
            // + добавить чекпойнт с принудительным включением проверки данные по автору поста и значению лайка
            // по сути нужно пробрасывать хэш из транзакции всегда
            // Check OP_RETURN with Payload
            //if (IsEmpty(ptx->GetOPRAddress()) || *ptx->GetOPRAddress() != *ptx->GetAddress())
            //    LogPrintf("000 CHECKPOINT 1 %s\n", *ptx->GetHash());
            //return {false, SocialConsensusResult_OpReturnFailed};
            //if (IsEmpty(ptx->GetOPRValue()) || *ptx->GetOPRValue() != *ptx->GetValue())
            //    LogPrintf("000 CHECKPOINT 2 %s\n", *ptx->GetHash());
            //return {false, SocialConsensusResult_OpReturnFailed};

            return Success;
        }

    protected:
        virtual int64_t GetLimitWindow() { return 86400; }
        virtual int64_t GetFullLimit() { return 90; }
        virtual int64_t GetTrialLimit() { return 45; }
        virtual int64_t GetScoresLimit(AccountMode mode) { return mode >= AccountMode_Full ? GetFullLimit() : GetTrialLimit(); }

        virtual bool CheckBlockLimitTime(const ScoreContentRef& ptx, const ScoreContentRef& blockTx)
        {
            return *blockTx->GetTime() <= *ptx->GetTime();
        }

        ConsensusValidateResult ValidateBlock(const ScoreContentRef& ptx, const PocketBlockRef& block) override
        {

            // Get count from chain
            int count = GetChainCount(ptx);

            // Get count from block
            for (auto& blockTx : *block)
            {
                if (!IsIn(*blockTx->GetType(), {ACTION_SCORE_CONTENT}))
                    continue;

                auto blockPtx = static_pointer_cast<ScoreContent>(blockTx);

                if (*blockPtx->GetHash() == *ptx->GetHash())
                    continue;

                if (*ptx->GetAddress() == *blockPtx->GetAddress())
                {
                    if (CheckBlockLimitTime(ptx, blockPtx))
                        count += 1;

                    if (*blockTx->GetHash() == *ptx->GetContentTxHash())
                        return {false, SocialConsensusResult_DoubleScore};
                }
            }

            // Check count
            return ValidateLimit(ptx, count);
        }

        ConsensusValidateResult ValidateMempool(const ScoreContentRef& ptx) override
        {

            // Check already scored content
            if (PocketDb::ConsensusRepoInst.ExistsScore(
                *ptx->GetAddress(), *ptx->GetContentTxHash(), ACTION_SCORE_CONTENT, true))
                return {false, SocialConsensusResult_DoubleScore};

            // Get count from chain
            int count = GetChainCount(ptx);

            // Get count from mempool
            count += ConsensusRepoInst.CountMempoolScoreContent(*ptx->GetAddress());

            // Check count
            return ValidateLimit(ptx, count);
        }

        virtual bool ValidateLowReputation(const ScoreContentRef& ptx, AccountMode mode)
        {
            return true;
        }

        virtual ConsensusValidateResult ValidateLimit(const ScoreContentRef& ptx, int count)
        {
            auto reputationConsensus = PocketConsensus::ReputationConsensusFactoryInst.Instance(Height);
            auto accountMode = reputationConsensus->GetAccountMode(*ptx->GetAddress());
            if (count >= GetScoresLimit(accountMode))
                return {false, SocialConsensusResult_ScoreLimit};

            if (!ValidateLowReputation(ptx, accountMode))
                return {false, SocialConsensusResult_LowReputation};

            return Success;
        }

        virtual ConsensusValidateResult ValidateBlocking(const string& contentAddress, const ScoreContentRef& ptx)
        {
            return Success;
        }

        virtual int GetChainCount(const ScoreContentRef& ptx)
        {
            return ConsensusRepoInst.CountChainScoreContentTime(
                *ptx->GetAddress(),
                *ptx->GetTime() - GetLimitWindow()
            );
        }

        vector<string> GetAddressesForCheckRegistration(const ScoreContentRef& ptx) override
        {
            return {*ptx->GetAddress()};
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
        ConsensusValidateResult ValidateBlocking(const string& contentAddress, const ScoreContentRef& ptx) override
        {
            auto[existsBlocking, blockingType] = PocketDb::ConsensusRepoInst.GetLastBlockingType(
                contentAddress,
                *ptx->GetAddress()
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
        ConsensusValidateResult ValidateBlocking(const string& contentAddress, const ScoreContentRef& ptx) override
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
        bool CheckBlockLimitTime(const ScoreContentRef& ptx, const ScoreContentRef& blockPtx) override
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
        int GetChainCount(const ScoreContentRef& ptx) override
        {
            return ConsensusRepoInst.CountChainScoreContentHeight(
                *ptx->GetAddress(),
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
        bool ValidateLowReputation(const ScoreContentRef& ptx, AccountMode mode) override
        {
            return (mode != AccountMode_Trial || (*ptx->GetValue() == 1 || *ptx->GetValue() == 2));
        }
    };

    /*******************************************************************************************************************
    *  Factory for select actual rules version
    *******************************************************************************************************************/
    class ScoreContentConsensusFactory
    {
    private:
        const vector<ConsensusCheckpoint < ScoreContentConsensus>> m_rules = {
            { 0, -1, [](int height) { return make_shared<ScoreContentConsensus>(height); }},
            { 175600, -1, [](int height) { return make_shared<ScoreContentConsensus_checkpoint_175600>(height); }},
            { 430000, -1, [](int height) { return make_shared<ScoreContentConsensus_checkpoint_430000>(height); }},
            { 514184, -1, [](int height) { return make_shared<ScoreContentConsensus_checkpoint_514184>(height); }},
            { 1124000, -1, [](int height) { return make_shared<ScoreContentConsensus_checkpoint_1124000>(height); }},
            { 1180000, -1, [](int height) { return make_shared<ScoreContentConsensus_checkpoint_1180000>(height); }},
            { 1324655, 0, [](int height) { return make_shared<ScoreContentConsensus_checkpoint_1324655>(height); }},
        };
    public:
        shared_ptr<ScoreContentConsensus> Instance(int height)
        {
            int m_height = (height > 0 ? height : 0);
            return (--upper_bound(m_rules.begin(), m_rules.end(), m_height,
                [&](int target, const ConsensusCheckpoint<ScoreContentConsensus>& itm)
                {
                    return target < itm.Height(Params().NetworkIDString());
                }
            ))->m_func(height);
        }
    };
}

#endif // POCKETCONSENSUS_SCORECONTENT_HPP