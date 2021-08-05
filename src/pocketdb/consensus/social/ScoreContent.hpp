// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 Bitcoin developers
// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETCONSENSUS_SCORECONTENT_HPP
#define POCKETCONSENSUS_SCORECONTENT_HPP

#include "pocketdb/consensus/social/Base.hpp"
#include "pocketdb/models/base/Transaction.hpp"
#include "pocketdb/models/dto/ScoreContent.hpp"

namespace PocketConsensus
{
    using namespace std;

    /*******************************************************************************************************************
    *
    *  ScorePost consensus base class
    *
    *******************************************************************************************************************/
    class ScoreContentConsensus : public SocialBaseConsensus
    {
    public:
        ScoreContentConsensus(int height) : SocialBaseConsensus(height) {}

    protected:

        virtual int64_t GetLimitWindow() { return 86400; }

        virtual int64_t GetFullAccountScoresLimit() { return 90; }

        virtual int64_t GetTrialAccountScoresLimit() { return 45; }

        virtual int64_t GetScoresLimit(AccountMode mode)
        {
            return mode == AccountMode_Full ? GetFullAccountScoresLimit() : GetTrialAccountScoresLimit();
        }


        tuple<bool, SocialConsensusResult> ValidateModel(const PTransactionRef& tx) override
        {
            auto ptx = static_pointer_cast<ScoreContent>(tx);

            // Content should be exists
            auto[lastContentOk, lastContent] = PocketDb::ConsensusRepoInst.GetLastContent(*ptx->GetContentTxHash());
            if (!lastContentOk)
                return {false, SocialConsensusResult_NotFound};

            // Check score to self
            // lastContent->GetString1() - author address post
            if (*ptx->GetAddress() == *lastContent->GetString1())
                return {false, SocialConsensusResult_SelfScore};

            // Check Blocking
            if (auto[ok, result] = ValidateBlocking(*lastContent->GetString1(), ptx); !ok)
                return {false, result};

            // Check already scored content
            if (PocketDb::ConsensusRepoInst.ExistsScore(
                *ptx->GetAddress(), *ptx->GetContentTxHash(), ACTION_SCORE_CONTENT, false))
                return {false, SocialConsensusResult_DoubleScore};

            return Success;
        }

        virtual bool CheckBlockLimitTime(const PTransactionRef& ptx, const PTransactionRef& blockPtx)
        {
            return *blockPtx->GetTime() <= *ptx->GetTime();
        }

        tuple<bool, SocialConsensusResult> ValidateLimit(const PTransactionRef& tx,
            const PocketBlock& block) override
        {
            auto ptx = static_pointer_cast<ScoreContent>(tx);

            // Get count from chain
            int count = GetChainCount(ptx);

            // Get count from block
            for (auto& blockTx : block)
            {
                if (!IsIn(*blockTx->GetType(), {ACTION_SCORE_CONTENT}))
                    continue;

                if (*blockTx->GetHash() == *ptx->GetHash())
                    continue;

                auto blockPtx = static_pointer_cast<ScoreContent>(blockTx);
                if (*ptx->GetAddress() == *blockPtx->GetAddress())
                {
                    if (CheckBlockLimitTime(tx, blockTx))
                        count += 1;

                    if (*blockPtx->GetHash() == *ptx->GetContentTxHash())
                        return {false, SocialConsensusResult_DoubleScore};
                }
            }

            return ValidateLimit(ptx, count);
        }

        tuple<bool, SocialConsensusResult> ValidateLimit(const PTransactionRef& tx) override
        {
            auto ptx = static_pointer_cast<ScoreContent>(tx);

            // Check already scored content
            if (PocketDb::ConsensusRepoInst.ExistsScore(
                *ptx->GetAddress(), *ptx->GetContentTxHash(), ACTION_SCORE_CONTENT, true))
                return {false, SocialConsensusResult_DoubleScore};

            // Get count from chain
            int count = GetChainCount(ptx);

            count += ConsensusRepoInst.CountMempoolScoreContent(*ptx->GetAddress());

            return ValidateLimit(ptx, count);
        }

        virtual tuple<bool, SocialConsensusResult> ValidateLimit(const shared_ptr<ScoreContent>& tx, int count)
        {
            auto reputationConsensus = ReputationConsensusFactory::Instance(Height);
            auto accountMode = reputationConsensus->GetAccountMode(*tx->GetAddress());
            auto limit = GetScoresLimit(accountMode);

            if (count >= limit)
                return {false, SocialConsensusResult_ScoreLimit};

            return Success;
        }

        virtual tuple<bool, SocialConsensusResult> ValidateBlocking(const string& contentAddress,
            const shared_ptr<ScoreContent>& tx)
        {
            return Success;
        }

        virtual int GetChainCount(const shared_ptr<ScoreContent>& ptx)
        {
            return ConsensusRepoInst.CountChainScoreContentTime(
                *ptx->GetAddress(),
                *ptx->GetTime() - GetLimitWindow()
            );
        }

        tuple<bool, SocialConsensusResult> CheckModel(const PTransactionRef& tx) override
        {
            auto ptx = static_pointer_cast<ScoreContent>(tx);

            // Check required fields
            if (IsEmpty(ptx->GetAddress())) return {false, SocialConsensusResult_Failed};
            if (IsEmpty(ptx->GetContentTxHash())) return {false, SocialConsensusResult_Failed};
            if (IsEmpty(ptx->GetValue())) return {false, SocialConsensusResult_Failed};

            auto value = *ptx->GetValue();
            if (value < 1 || value > 5)
                return {false, SocialConsensusResult_Failed};

            // TODO (brangr): tmp disable
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

        vector<string> GetAddressesForCheckRegistration(const PTransactionRef& tx) override
        {
            auto ptx = static_pointer_cast<ScoreContent>(tx);
            return {*ptx->GetAddress()};
        }

    };

    /*******************************************************************************************************************
    *
    *  Consensus checkpoint at 175600 block
    *
    *******************************************************************************************************************/
    class ScoreContentConsensus_checkpoint_175600 : public ScoreContentConsensus
    {
    protected:
        int CheckpointHeight() override { return 175600; }

        int64_t GetFullAccountScoresLimit() override { return 200; }

        int64_t GetTrialAccountScoresLimit() override { return 100; }

    public:
        ScoreContentConsensus_checkpoint_175600(int height) : ScoreContentConsensus(height) {}
    };

    /*******************************************************************************************************************
    *
    *  Consensus checkpoint at 430000 block
    *
    *******************************************************************************************************************/
    class ScoreContentConsensus_checkpoint_430000 : public ScoreContentConsensus_checkpoint_175600
    {
    protected:
        int CheckpointHeight() override { return 430000; }

        tuple<bool, SocialConsensusResult> ValidateBlocking(const string& contentAddress,
            const shared_ptr<ScoreContent>& tx) override
        {
            auto[existsBlocking, blockingType] = PocketDb::ConsensusRepoInst.GetLastBlockingType(
                contentAddress,
                *tx->GetAddress()
            );

            if (existsBlocking && blockingType == ACTION_BLOCKING)
                return {false, SocialConsensusResult_Blocking};

            return Success;
        }

    public:
        ScoreContentConsensus_checkpoint_430000(int height) : ScoreContentConsensus_checkpoint_175600(height) {}
    };

    /*******************************************************************************************************************
    *
    *  Consensus checkpoint at 514184 block
    *
    *******************************************************************************************************************/
    class ScoreContentConsensus_checkpoint_514184 : public ScoreContentConsensus_checkpoint_430000
    {
    protected:
        int CheckpointHeight() override { return 514184; }

        tuple<bool, SocialConsensusResult> ValidateBlocking(const string& contentAddress,
            const shared_ptr<ScoreContent>& tx) override
        {
            return Success;
        }

    public:
        ScoreContentConsensus_checkpoint_514184(int height) : ScoreContentConsensus_checkpoint_430000(height) {}
    };

    /*******************************************************************************************************************
    *
    *  Start checkpoint at 1124000 block
    *
    *******************************************************************************************************************/
    class ScoreContentConsensus_checkpoint_1124000 : public ScoreContentConsensus_checkpoint_514184
    {
    public:
        ScoreContentConsensus_checkpoint_1124000(int height) : ScoreContentConsensus_checkpoint_514184(height) {}

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
    class ScoreContentConsensus_checkpoint_1180000 : public ScoreContentConsensus_checkpoint_1124000
    {
    public:
        ScoreContentConsensus_checkpoint_1180000(int height) : ScoreContentConsensus_checkpoint_1124000(height) {}

    protected:
        int CheckpointHeight() override { return 1180000; }

        int64_t GetLimitWindow() override { return 1440; }

        int GetChainCount(const shared_ptr<ScoreContent>& ptx) override
        {
            return ConsensusRepoInst.CountChainScoreContentHeight(
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
    class ScoreContentConsensusFactory
    {
    private:
        static inline const std::map<int, std::function<ScoreContentConsensus*(int height)>> m_rules =
            {
                {1180000, [](int height) { return new ScoreContentConsensus_checkpoint_1180000(height); }},
                {1124000, [](int height) { return new ScoreContentConsensus_checkpoint_1124000(height); }},
                {514184,  [](int height) { return new ScoreContentConsensus_checkpoint_514184(height); }},
                {430000,  [](int height) { return new ScoreContentConsensus_checkpoint_430000(height); }},
                {175600,  [](int height) { return new ScoreContentConsensus_checkpoint_175600(height); }},
                {0,       [](int height) { return new ScoreContentConsensus(height); }},
            };
    public:
        shared_ptr<ScoreContentConsensus> Instance(int height)
        {
            return shared_ptr<ScoreContentConsensus>(
                (--m_rules.upper_bound(height))->second(height)
            );
        }
    };
}

#endif // POCKETCONSENSUS_SCORECONTENT_HPP