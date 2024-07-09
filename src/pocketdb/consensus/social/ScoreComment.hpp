// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETCONSENSUS_SCORECOMMENT_HPP
#define POCKETCONSENSUS_SCORECOMMENT_HPP

#include "pocketdb/consensus/Reputation.h"
#include "pocketdb/consensus/Social.h"
#include "pocketdb/models/dto/action/ScoreComment.h"

namespace PocketConsensus
{
    typedef shared_ptr<ScoreComment> ScoreCommentRef;

    /*******************************************************************************************************************
    *  ScoreComment consensus base class
    *******************************************************************************************************************/
    class ScoreCommentConsensus : public SocialConsensus<ScoreComment>
    {
    public:
        explicit ScoreCommentConsensus() : SocialConsensus<ScoreComment>()
        {
            // TODO (limits): set limits
        }

        ConsensusValidateResult Validate(const CTransactionRef& tx, const ScoreCommentRef& ptx, const PocketBlockRef& block) override
        {
            // Check already scored content
            if (PocketDb::ConsensusRepoInst.ExistsScore(
                *ptx->GetAddress(), *ptx->GetCommentTxHash(), ACTION_SCORE_COMMENT, false))
                return {false, ConsensusResult_DoubleCommentScore};

            // Comment should be exists
            auto[lastContentOk, lastContent] = PocketDb::ConsensusRepoInst.GetLastContent(
                *ptx->GetCommentTxHash(),
                { CONTENT_COMMENT, CONTENT_COMMENT_EDIT, CONTENT_COMMENT_DELETE }
            );

            if (!lastContentOk && block)
            {
                // ... or in block
                for (auto& blockTx : *block)
                {
                    if (!TransactionHelper::IsIn(*blockTx->GetType(), {CONTENT_COMMENT, CONTENT_COMMENT_EDIT, CONTENT_COMMENT_DELETE}))
                        continue;

                    // TODO (aok): convert to Comment base class
                    if (*blockTx->GetString2() == *ptx->GetCommentTxHash())
                    {
                        lastContent = blockTx;
                        break;
                    }
                }
            }
            if (!lastContent)
                return {false, ConsensusResult_NotFound};

            // Scores to deleted comments not allowed
            if (*lastContent->GetType() == TxType::CONTENT_COMMENT_DELETE)
            {
                if (!CheckpointRepoInst.IsSocialCheckpoint(*ptx->GetHash(), *ptx->GetType(), ConsensusResult_NotFound))
                    return {false, ConsensusResult_NotFound};
            }

            auto lastContentPtx = static_pointer_cast<Comment>(lastContent);

            // Check score to self
            if (*ptx->GetAddress() == *lastContentPtx->GetAddress())
                return {false, ConsensusResult_SelfCommentScore};

            // Check Blocking
            if (ValidateBlocking(*lastContentPtx->GetAddress(), *ptx->GetAddress()))
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
        ConsensusValidateResult Check(const CTransactionRef& tx, const ScoreCommentRef& ptx) override
        {

            if (auto[baseCheck, baseCheckCode] = SocialConsensus::Check(tx, ptx); !baseCheck)
                return {false, baseCheckCode};

            // Check required fields
            if (IsEmpty(ptx->GetAddress())) return {false, ConsensusResult_Failed};
            if (IsEmpty(ptx->GetCommentTxHash())) return {false, ConsensusResult_Failed};
            if (IsEmpty(ptx->GetValue())) return {false, ConsensusResult_Failed};

            auto value = *ptx->GetValue();
            if (value != 1 && value != -1)
                return {false, ConsensusResult_Failed};

            return Success;
        }

    protected:
        ConsensusValidateResult ValidateBlock(const ScoreCommentRef& ptx, const PocketBlockRef& block) override
        {
            // Get count from chain
            int count = GetChainCount(ptx);

            // Get count from block
            for (auto& blockTx : *block)
            {
                if (!TransactionHelper::IsIn(*blockTx->GetType(), {ACTION_SCORE_COMMENT}))
                    continue;

                if (*blockTx->GetHash() == *ptx->GetHash())
                    continue;

                auto blockPtx = static_pointer_cast<ScoreComment>(blockTx);
                if (*ptx->GetAddress() == *blockPtx->GetAddress())
                {
                    if (CheckBlockLimitTime(ptx, blockPtx))
                        count += 1;

                    if (*blockPtx->GetCommentTxHash() == *ptx->GetCommentTxHash())
                        return {false, ConsensusResult_DoubleCommentScore};
                }
            }

            return ValidateLimit(ptx, count);
        }
        ConsensusValidateResult ValidateMempool(const ScoreCommentRef& ptx) override
        {
            // Check already scored content
            if (PocketDb::ConsensusRepoInst.ExistsScore(
                *ptx->GetAddress(), *ptx->GetCommentTxHash(), ACTION_SCORE_COMMENT, true))
                return {false, ConsensusResult_DoubleCommentScore};

            // Check count from chain
            int count = GetChainCount(ptx);

            // and from mempool
            count += ConsensusRepoInst.CountMempoolScoreComment(*ptx->GetAddress());

            return ValidateLimit(ptx, count);
        }
        vector<string> GetAddressesForCheckRegistration(const ScoreCommentRef& ptx) override
        {
            return {*ptx->GetAddress()};
        }

        virtual int64_t GetScoresLimit(AccountMode mode)
        {
            return mode >= AccountMode_Full ? GetConsensusLimit(ConsensusLimit_full_comment_score) : GetConsensusLimit(ConsensusLimit_trial_comment_score);
        }
        virtual bool CheckBlockLimitTime(const ScoreCommentRef& ptx, const ScoreCommentRef& blockTx)
        {
            return *blockTx->GetTime() <= *ptx->GetTime();
        }
        virtual ConsensusValidateResult ValidateLimit(const ScoreCommentRef& ptx, int count)
        {
            auto reputationConsensus = PocketConsensus::ConsensusFactoryInst_Reputation.Instance(Height);
            auto address = ptx->GetAddress();
            auto[mode, reputation, balance] = reputationConsensus->GetAccountMode(*address);
            if (count >= GetScoresLimit(mode))
                return {false, ConsensusResult_CommentScoreLimit};

            return Success;
        }
        virtual bool ValidateBlocking(const string& address1, const string& address2)
        {
            return false;
        }
        virtual int GetChainCount(const ScoreCommentRef& ptx)
        {

            return ConsensusRepoInst.CountChainScoreCommentTime(
                *ptx->GetAddress(),
                *ptx->GetTime() - GetConsensusLimit(ConsensusLimit_depth)
            );
        }
    };

    /*******************************************************************************************************************
    *  Consensus checkpoint at 430000 block
    *******************************************************************************************************************/
    class ScoreCommentConsensus_checkpoint_430000 : public ScoreCommentConsensus
    {
    public:
        explicit ScoreCommentConsensus_checkpoint_430000() : ScoreCommentConsensus() {}
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
    class ScoreCommentConsensus_checkpoint_514184 : public ScoreCommentConsensus_checkpoint_430000
    {
    public:
        explicit ScoreCommentConsensus_checkpoint_514184() : ScoreCommentConsensus_checkpoint_430000() {}
    protected:
        bool ValidateBlocking(const string& address1, const string& address2) override
        {
            return false;
        }
    };

    // ----------------------------------------------------------------------------------------------
    class ScoreCommentConsensus_checkpoint_1124000 : public ScoreCommentConsensus_checkpoint_514184
    {
    public:
        explicit ScoreCommentConsensus_checkpoint_1124000() : ScoreCommentConsensus_checkpoint_514184() {}
    protected:
        bool CheckBlockLimitTime(const ScoreCommentRef& ptx, const ScoreCommentRef& blockPtx) override
        {
            return true;
        }
    };

    // ----------------------------------------------------------------------------------------------
    class ScoreCommentConsensus_checkpoint_1180000 : public ScoreCommentConsensus_checkpoint_1124000
    {
    public:
        explicit ScoreCommentConsensus_checkpoint_1180000() : ScoreCommentConsensus_checkpoint_1124000() {}
    protected:
        int GetChainCount(const ScoreCommentRef& ptx) override
        {
            return ConsensusRepoInst.CountChainHeight(
                *ptx->GetType(),
                *ptx->GetAddress()
            );
        }
    };

    // ----------------------------------------------------------------------------------------------
    class ScoreCommentConsensus_checkpoint_disable_for_blocked : public ScoreCommentConsensus_checkpoint_1180000
    {
    public:
        explicit ScoreCommentConsensus_checkpoint_disable_for_blocked() : ScoreCommentConsensus_checkpoint_1180000() {}
    protected:
        bool ValidateBlocking(const string& address1, const string& address2) override
        {
            auto[existsBlocking, blockingType] = PocketDb::ConsensusRepoInst.GetLastBlockingType(address1, address2);
            return existsBlocking && blockingType == ACTION_BLOCKING;
        }
    };

    // ----------------------------------------------------------------------------------------------
    class ScoreCommentConsensus_checkpoint_pip_105 : public ScoreCommentConsensus_checkpoint_disable_for_blocked
    {
    public:
        explicit ScoreCommentConsensus_checkpoint_pip_105() : ScoreCommentConsensus_checkpoint_disable_for_blocked() {}
    protected:
        bool ValidateBlocking(const string& address1, const string& address2) override
        {
            return SocialConsensus::CheckBlocking(address1, address2);
        }
    };


    // ----------------------------------------------------------------------------------------------
    class ScoreCommentConsensusFactory : public BaseConsensusFactory<ScoreCommentConsensus>
    {
    public:
        ScoreCommentConsensusFactory()
        {
            Checkpoint({       0,      -1, -1, make_shared<ScoreCommentConsensus>() });
            Checkpoint({  430000,      -1, -1, make_shared<ScoreCommentConsensus_checkpoint_430000>() });
            Checkpoint({  514184,      -1, -1, make_shared<ScoreCommentConsensus_checkpoint_514184>() });
            Checkpoint({ 1124000,      -1, -1, make_shared<ScoreCommentConsensus_checkpoint_1124000>() });
            Checkpoint({ 1180000,       0, -1, make_shared<ScoreCommentConsensus_checkpoint_1180000>() });
            Checkpoint({ 1757000,  953000, -1, make_shared<ScoreCommentConsensus_checkpoint_disable_for_blocked>() });
            Checkpoint({ 2794500, 2574300,  0, make_shared<ScoreCommentConsensus_checkpoint_pip_105>() });
        }
    };

    static ScoreCommentConsensusFactory ConsensusFactoryInst_ScoreComment;
}

#endif // POCKETCONSENSUS_SCORECOMMENT_HPP