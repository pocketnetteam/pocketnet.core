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
    typedef shared_ptr<ScoreContent> ScoreContentRef;

    /*******************************************************************************************************************
    *  ScorePost consensus base class
    *******************************************************************************************************************/
    class ScoreContentConsensus : public SocialConsensus<ScoreContent>
    {
    public:
        ScoreContentConsensus() : SocialConsensus<ScoreContent>()
        {
            // TODO (limits): set limits
        }

        ConsensusValidateResult Validate(const CTransactionRef& tx, const ScoreContentRef& ptx, const PocketBlockRef& block) override
        {
            // Check already scored content
            if (PocketDb::ConsensusRepoInst.ExistsScore(*ptx->GetAddress(), *ptx->GetContentTxHash(), ACTION_SCORE_CONTENT, false))
                return {false, ConsensusResult_DoubleScore};

            // Content should be exists in chain
            auto[lastContentOk, lastContent] = PocketDb::ConsensusRepoInst.GetLastContent(
                *ptx->GetContentTxHash(),
                { CONTENT_POST, CONTENT_VIDEO, CONTENT_ARTICLE, CONTENT_STREAM, CONTENT_AUDIO, CONTENT_DELETE, BARTERON_OFFER, CONTENT_APP }
            );

            if (!lastContentOk && block)
            {
                // ... or in block
                for (auto& blockTx : *block)
                {
                    if (!TransactionHelper::IsIn(*blockTx->GetType(), {CONTENT_POST, CONTENT_VIDEO, CONTENT_ARTICLE, CONTENT_STREAM, CONTENT_AUDIO, CONTENT_DELETE, BARTERON_OFFER, CONTENT_APP}))
                        continue;

                    // TODO (aok): convert to Content base class
                    if (*blockTx->GetString2() == *ptx->GetContentTxHash())
                    {
                        lastContent = blockTx;
                        break;
                    }
                }
            }
            if (!lastContent)
                return {false, ConsensusResult_NotFound};

            // Check score to self
            if (*ptx->GetAddress() == *lastContent->GetString1())
                return {false, ConsensusResult_SelfScore};

            // Scores for deleted contents not allowed
            if (*lastContent->GetType() == TxType::CONTENT_DELETE)
                return {false, ConsensusResult_ScoreDeletedContent};

            // Check Blocking
            if (ValidateBlocking(*lastContent->GetString1(), *ptx->GetAddress()))
                return {false, ConsensusResult_Blocking};

            // Check OP_RETURN content author address and score value
            if (auto[ok, txOpReturnPayload] = TransactionHelper::ExtractOpReturnPayload(tx); ok)
            {
                string opReturnPayloadData = *lastContent->GetString1() + " " + to_string(*ptx->GetValue());
                string opReturnPayloadHex = HexStr(opReturnPayloadData);

                if (txOpReturnPayload != opReturnPayloadHex)
                    return {false, ConsensusResult_FailedOpReturn};
            }

            return SocialConsensus::Validate(tx, ptx, block);
        }
        ConsensusValidateResult Check(const CTransactionRef& tx, const ScoreContentRef& ptx) override
        {

            if (auto[baseCheck, baseCheckCode] = SocialConsensus::Check(tx, ptx); !baseCheck)
                return {false, baseCheckCode};

            // Check required fields
            if (IsEmpty(ptx->GetAddress())) return {false, ConsensusResult_Failed};
            if (IsEmpty(ptx->GetContentTxHash())) return {false, ConsensusResult_Failed};
            if (IsEmpty(ptx->GetValue())) return {false, ConsensusResult_Failed};

            auto value = *ptx->GetValue();
            if (value < 1 || value > 5)
                return {false, ConsensusResult_Failed};

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
                        if (!CheckpointRepoInst.IsSocialCheckpoint(*ptx->GetHash(), *ptx->GetType(), ConsensusResult_DoubleScore))
                            return {false, ConsensusResult_DoubleScore};
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
                return {false, ConsensusResult_DoubleScore};

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
            auto reputationConsensus = PocketConsensus::ConsensusFactoryInst_Reputation.Instance(Height);
            auto address = ptx->GetAddress();
            auto[mode, reputation, balance] = reputationConsensus->GetAccountMode(*address);
            if (count >= GetScoresLimit(mode))
                return {false, ConsensusResult_ScoreLimit};

            if (!ValidateLowReputation(ptx, mode))
                return {false, ConsensusResult_ScoreLowReputation};

            return Success;
        }
        virtual bool ValidateBlocking(const string& address1, const string& address2)
        {
            return false;
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
        ScoreContentConsensus_checkpoint_430000() : ScoreContentConsensus() {}
    protected:
        bool ValidateBlocking(const string& address1, const string& address2) override
        {
            auto[existsBlocking, blockingType] = PocketDb::ConsensusRepoInst.GetLastBlockingType(address1, address2);
            return existsBlocking && blockingType == ACTION_BLOCKING;
        }
    };

    /*******************************************************************************************************************
    *  Consensus checkpoint at 514184 block
    *******************************************************************************************************************/
    class ScoreContentConsensus_checkpoint_514184 : public ScoreContentConsensus_checkpoint_430000
    {
    public:
        ScoreContentConsensus_checkpoint_514184() : ScoreContentConsensus_checkpoint_430000() {}
    protected:
        bool ValidateBlocking(const string& address1, const string& address2) override
        {
            return false;
        }
    };

    /*******************************************************************************************************************
    *  Start checkpoint at 1124000 block
    *******************************************************************************************************************/
    class ScoreContentConsensus_checkpoint_1124000 : public ScoreContentConsensus_checkpoint_514184
    {
    public:
        ScoreContentConsensus_checkpoint_1124000() : ScoreContentConsensus_checkpoint_514184() {}
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
        ScoreContentConsensus_checkpoint_1180000() : ScoreContentConsensus_checkpoint_1124000() {}
    protected:
        int GetChainCount(const ScoreContentRef& ptx) override
        {
            return ConsensusRepoInst.CountChainHeight(
                *ptx->GetType(),
                *ptx->GetAddress()
            );
        }
    };

    // Disable 1-2 scores for trial users
    class ScoreContentConsensus_checkpoint_1324655 : public ScoreContentConsensus_checkpoint_1180000
    {
    public:
        ScoreContentConsensus_checkpoint_1324655() : ScoreContentConsensus_checkpoint_1180000() {}
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
        ScoreContentConsensus_checkpoint_disable_for_blocked() : ScoreContentConsensus_checkpoint_1324655() {}
    protected:
        bool ValidateBlocking(const string& address1, const string& address2) override
        {
            auto[existsBlocking, blockingType] = PocketDb::ConsensusRepoInst.GetLastBlockingType(address1, address2);
            return existsBlocking && blockingType == ACTION_BLOCKING;
        }
    };

    // Disable 1-2-3 scores for trial users
    class ScoreContentConsensus_checkpoint_pip_103 : public ScoreContentConsensus_checkpoint_disable_for_blocked
    {
    public:
        ScoreContentConsensus_checkpoint_pip_103() : ScoreContentConsensus_checkpoint_disable_for_blocked() {}
    protected:
        bool ValidateLowReputation(const ScoreContentRef& ptx, AccountMode mode) override
        {
            return mode != AccountMode_Trial || *ptx->GetValue() > 3;
        }
    };

    // ----------------------------------------------------------------------------------------------
    class ScoreContentConsensus_checkpoint_pip_105 : public ScoreContentConsensus_checkpoint_pip_103
    {
    public:
        ScoreContentConsensus_checkpoint_pip_105() : ScoreContentConsensus_checkpoint_pip_103() {}
    protected:
        bool ValidateBlocking(const string& address1, const string& address2) override
        {
            return SocialConsensus::CheckBlocking(address1, address2);
        }
    };


    // ----------------------------------------------------------------------------------------------
    class ScoreContentConsensusFactory : public BaseConsensusFactory<ScoreContentConsensus>
    {
    public:
        ScoreContentConsensusFactory()
        {
            Checkpoint({       0,      -1, -1, make_shared<ScoreContentConsensus>() });
            Checkpoint({  430000,      -1, -1, make_shared<ScoreContentConsensus_checkpoint_430000>() });
            Checkpoint({  514184,      -1, -1, make_shared<ScoreContentConsensus_checkpoint_514184>() });
            Checkpoint({ 1124000,      -1, -1, make_shared<ScoreContentConsensus_checkpoint_1124000>() });
            Checkpoint({ 1180000,       0, -1, make_shared<ScoreContentConsensus_checkpoint_1180000>() });
            Checkpoint({ 1324655,   65000, -1, make_shared<ScoreContentConsensus_checkpoint_1324655>() });
            Checkpoint({ 1757000,  953000, -1, make_shared<ScoreContentConsensus_checkpoint_disable_for_blocked>() });
            Checkpoint({ 2583000, 2267333, -1, make_shared<ScoreContentConsensus_checkpoint_pip_103>() });
            Checkpoint({ 2794500, 2574300,  0, make_shared<ScoreContentConsensus_checkpoint_pip_105>() });
        }
    };

    static ScoreContentConsensusFactory ConsensusFactoryInst_ScoreContent;
}

#endif // POCKETCONSENSUS_SCORECONTENT_HPP