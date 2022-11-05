// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETCONSENSUS_SCORECONTENT_HPP
#define POCKETCONSENSUS_SCORECONTENT_HPP

#include "pocketdb/consensus/Reputation.h"
#include "pocketdb/consensus/Social.h"
#include "pocketdb/models/dto/action/ScoreContent.h"

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

        ConsensusValidateResult Validate(const CTransactionRef& tx, const ScoreContentRef& ptx, const PocketBlockRef& block) override
        {
            // Check already scored content
            if (PocketDb::ConsensusRepoInst.ExistsScore(*ptx->GetAddress(), *ptx->GetContentTxHash(), ACTION_SCORE_CONTENT, false))
                return {false, SocialConsensusResult_DoubleScore};

            // Content should be exists in chain
            auto[lastContentOk, lastContent] = PocketDb::ConsensusRepoInst.GetLastContent(
                *ptx->GetContentTxHash(),
                { CONTENT_POST, CONTENT_VIDEO, CONTENT_ARTICLE, CONTENT_DELETE }
            );

            if (!lastContentOk && block)
            {
                // ... or in block
                for (auto& blockTx : *block)
                {
                    if (!TransactionHelper::IsIn(*blockTx->GetType(), {CONTENT_POST, CONTENT_VIDEO, CONTENT_ARTICLE, CONTENT_DELETE}))
                        continue;

                    // TODO (brangr): convert to Content base class
                    if (*blockTx->GetString2() == *ptx->GetContentTxHash())
                    {
                        lastContent = blockTx;
                        break;
                    }
                }
            }
            if (!lastContent)
                return {false, SocialConsensusResult_NotFound};

            // Check score to self
            if (*ptx->GetAddress() == *lastContent->GetString1())
                return {false, SocialConsensusResult_SelfScore};

            // Scores for deleted contents not allowed
            if (*lastContent->GetType() == TxType::CONTENT_DELETE)
                return {false, SocialConsensusResult_ScoreDeletedContent};

            // Check Blocking
            if (auto[ok, result] = ValidateBlocking(*lastContent->GetString1(), ptx); !ok)
                return {false, result};

            // Check OP_RETURN content author address and score value
            if (auto[ok, txOpReturnPayload] = TransactionHelper::ExtractOpReturnPayload(tx); ok)
            {
                string opReturnPayloadData = *lastContent->GetString1() + " " + to_string(*ptx->GetValue());
                string opReturnPayloadHex = HexStr(opReturnPayloadData);

                if (txOpReturnPayload != opReturnPayloadHex)
                    return {false, SocialConsensusResult_FailedOpReturn};
            }

            return SocialConsensus::Validate(tx, ptx, block);
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

            return Success;
        }

    protected:
        ConsensusValidateResult ValidateBlock(const ScoreContentRef& ptx, const PocketBlockRef& block) override
        {
            // Get count from chain
            int count = GetChainCount(ptx);

            // Get count from block
            for (auto& blockTx : *block)
            {
                if (!TransactionHelper::IsIn(*blockTx->GetType(), {ACTION_SCORE_CONTENT}))
                    continue;

                if (*blockTx->GetHash() == *ptx->GetHash())
                    continue;

                auto blockPtx = static_pointer_cast<ScoreContent>(blockTx);
                if (*ptx->GetAddress() == *blockPtx->GetAddress())
                {
                    if (CheckBlockLimitTime(ptx, blockPtx))
                        count += 1;

                    if (*blockPtx->GetContentTxHash() == *ptx->GetContentTxHash())
                        if (!CheckpointRepoInst.IsSocialCheckpoint(*ptx->GetHash(), *ptx->GetType(), SocialConsensusResult_DoubleScore))
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
        vector<string> GetAddressesForCheckRegistration(const ScoreContentRef& ptx) override
        {
            return {*ptx->GetAddress()};
        }

        virtual int64_t GetScoresLimit(AccountMode mode)
        {
            return mode >= AccountMode_Full ? GetConsensusLimit(ConsensusLimit_full_score) : GetConsensusLimit(ConsensusLimit_trial_score);
        }
        virtual bool CheckBlockLimitTime(const ScoreContentRef& ptx, const ScoreContentRef& blockTx)
        {
            return *blockTx->GetTime() <= *ptx->GetTime();
        }
        virtual bool ValidateLowReputation(const ScoreContentRef& ptx, AccountMode mode)
        {
            return true;
        }
        virtual ConsensusValidateResult ValidateLimit(const ScoreContentRef& ptx, int count)
        {
            auto reputationConsensus = PocketConsensus::ReputationConsensusFactoryInst.Instance(Height);
            auto[mode, reputation, balance] = reputationConsensus->GetAccountMode(*ptx->GetAddress());
            if (count >= GetScoresLimit(mode))
                return {false, SocialConsensusResult_ScoreLimit};

            if (!ValidateLowReputation(ptx, mode))
                return {false, SocialConsensusResult_ScoreLowReputation};

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
                *ptx->GetTime() - GetConsensusLimit(ConsensusLimit_depth)
            );
        }
    };

    /*******************************************************************************************************************
    *
    *  Consensus checkpoint at 430000 block
    *
    *******************************************************************************************************************/
    class ScoreContentConsensus_checkpoint_430000 : public ScoreContentConsensus
    {
    public:
        ScoreContentConsensus_checkpoint_430000(int height) : ScoreContentConsensus(height) {}
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
        int GetChainCount(const ScoreContentRef& ptx) override
        {
            return ConsensusRepoInst.CountChainScoreContentHeight(
                *ptx->GetAddress(),
                Height - (int) GetConsensusLimit(ConsensusLimit_depth)
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
        bool ValidateLowReputation(const ScoreContentRef& ptx, AccountMode mode) override
        {
            return mode != AccountMode_Trial || *ptx->GetValue() > 2;
        }
    };

    // Disable scores for blocked accounts
    class ScoreContentConsensus_checkpoint_disable_for_blocked : public ScoreContentConsensus_checkpoint_1324655
    {
    public:
        ScoreContentConsensus_checkpoint_disable_for_blocked(int height) : ScoreContentConsensus_checkpoint_1324655(height) {}
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
    *  Factory for select actual rules version
    *******************************************************************************************************************/
    class ScoreContentConsensusFactory
    {
    private:
        const vector<ConsensusCheckpoint < ScoreContentConsensus>> m_rules = {
            { 0,           -1, [](int height) { return make_shared<ScoreContentConsensus>(height); }},
            { 430000,      -1, [](int height) { return make_shared<ScoreContentConsensus_checkpoint_430000>(height); }},
            { 514184,      -1, [](int height) { return make_shared<ScoreContentConsensus_checkpoint_514184>(height); }},
            { 1124000,     -1, [](int height) { return make_shared<ScoreContentConsensus_checkpoint_1124000>(height); }},
            { 1180000,      0, [](int height) { return make_shared<ScoreContentConsensus_checkpoint_1180000>(height); }},
            { 1324655,  65000, [](int height) { return make_shared<ScoreContentConsensus_checkpoint_1324655>(height); }},
            { 1757000, 953000, [](int height) { return make_shared<ScoreContentConsensus_checkpoint_disable_for_blocked>(height); }},
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
            ))->m_func(m_height);
        }
    };
}

#endif // POCKETCONSENSUS_SCORECONTENT_HPP