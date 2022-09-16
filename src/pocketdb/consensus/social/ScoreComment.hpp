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
    using namespace std;
    typedef shared_ptr<ScoreComment> ScoreCommentRef;

    /*******************************************************************************************************************
    *  ScoreComment consensus base class
    *******************************************************************************************************************/
    class ScoreCommentConsensus : public SocialConsensus<ScoreComment>
    {
    public:
        explicit ScoreCommentConsensus(int height) : SocialConsensus<ScoreComment>(height) {}

        ConsensusValidateResult Validate(const CTransactionRef& tx, const ScoreCommentRef& ptx, const PocketBlockRef& block) override
        {
            // Check already scored content
            if (PocketDb::ConsensusRepoInst.ExistsScore(
                *ptx->GetAddress(), *ptx->GetCommentTxHash(), ACTION_SCORE_COMMENT, false))
                return {false, SocialConsensusResult_DoubleCommentScore};

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

                    // TODO (brangr): convert to Comment base class
                    if (*blockTx->GetString2() == *ptx->GetCommentTxHash())
                    {
                        lastContent = blockTx;
                        break;
                    }
                }
            }
            if (!lastContent)
                return {false, SocialConsensusResult_NotFound};

            // Scores to deleted comments not allowed
            if (*lastContent->GetType() == TxType::CONTENT_COMMENT_DELETE)
            {
                if (!CheckpointRepoInst.IsSocialCheckpoint(*ptx->GetHash(), *ptx->GetType(), SocialConsensusResult_NotFound))
                    return {false, SocialConsensusResult_NotFound};
            }

            // TODO (brangr): convert to Comment base class
            auto lastContentPtx = static_pointer_cast<Comment>(lastContent);

            // Check score to self
            if (*ptx->GetAddress() == *lastContentPtx->GetAddress())
                return {false, SocialConsensusResult_SelfCommentScore};

            // Check Blocking
            if (auto[ok, result] = ValidateBlocking(*lastContentPtx->GetAddress(), ptx); !ok)
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
        ConsensusValidateResult Check(const CTransactionRef& tx, const ScoreCommentRef& ptx) override
        {

            if (auto[baseCheck, baseCheckCode] = SocialConsensus::Check(tx, ptx); !baseCheck)
                return {false, baseCheckCode};

            // Check required fields
            if (IsEmpty(ptx->GetAddress())) return {false, SocialConsensusResult_Failed};
            if (IsEmpty(ptx->GetCommentTxHash())) return {false, SocialConsensusResult_Failed};
            if (IsEmpty(ptx->GetValue())) return {false, SocialConsensusResult_Failed};

            auto value = *ptx->GetValue();
            if (value != 1 && value != -1)
                return {false, SocialConsensusResult_Failed};

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
                        return {false, SocialConsensusResult_DoubleCommentScore};
                }
            }

            return ValidateLimit(ptx, count);
        }
        ConsensusValidateResult ValidateMempool(const ScoreCommentRef& ptx) override
        {

            // Check already scored content
            if (PocketDb::ConsensusRepoInst.ExistsScore(
                *ptx->GetAddress(), *ptx->GetCommentTxHash(), ACTION_SCORE_COMMENT, true))
                return {false, SocialConsensusResult_DoubleCommentScore};

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

            auto reputationConsensus = PocketConsensus::ReputationConsensusFactoryInst.Instance(Height);
            auto[mode, reputation, balance] = reputationConsensus->GetAccountMode(*ptx->GetAddress());
            if (count >= GetScoresLimit(mode))
                return {false, SocialConsensusResult_CommentScoreLimit};

            return Success;
        }
        virtual ConsensusValidateResult ValidateBlocking(const string& commentAddress, const ScoreCommentRef& tx)
        {
            return Success;
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
        explicit ScoreCommentConsensus_checkpoint_430000(int height) : ScoreCommentConsensus(height) {}
    protected:
        ConsensusValidateResult ValidateBlocking(const string& commentAddress, const ScoreCommentRef& ptx) override
        {

            auto[existsBlocking, blockingType] = PocketDb::ConsensusRepoInst.GetLastBlockingType(
                commentAddress,
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
    class ScoreCommentConsensus_checkpoint_514184 : public ScoreCommentConsensus_checkpoint_430000
    {
    public:
        explicit ScoreCommentConsensus_checkpoint_514184(int height) : ScoreCommentConsensus_checkpoint_430000(height) {}
    protected:
        ConsensusValidateResult ValidateBlocking(const string& commentAddress, const ScoreCommentRef& ptx) override
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
        explicit ScoreCommentConsensus_checkpoint_1124000(int height) : ScoreCommentConsensus_checkpoint_514184(height) {}
    protected:
        bool CheckBlockLimitTime(const ScoreCommentRef& ptx, const ScoreCommentRef& blockPtx) override
        {
            return true;
        }
    };

    class ScoreCommentConsensus_checkpoint_1180000 : public ScoreCommentConsensus_checkpoint_1124000
    {
    public:
        explicit ScoreCommentConsensus_checkpoint_1180000(int height) : ScoreCommentConsensus_checkpoint_1124000(height) {}
    protected:
        int GetChainCount(const ScoreCommentRef& ptx) override
        {

            return ConsensusRepoInst.CountChainScoreCommentHeight(
                *ptx->GetAddress(),
                Height - (int) GetConsensusLimit(ConsensusLimit_depth)
            );
        }
    };

    class ScoreCommentConsensus_checkpoint_disable_for_blocked : public ScoreCommentConsensus_checkpoint_1180000
    {
    public:
        explicit ScoreCommentConsensus_checkpoint_disable_for_blocked(int height) : ScoreCommentConsensus_checkpoint_1180000(height) {}
    protected:
        ConsensusValidateResult ValidateBlocking(const string& commentAddress, const ScoreCommentRef& ptx) override
        {

            auto[existsBlocking, blockingType] = PocketDb::ConsensusRepoInst.GetLastBlockingType(
                commentAddress,
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
    class ScoreCommentConsensusFactory
    {
    private:
        const vector<ConsensusCheckpoint < ScoreCommentConsensus>> m_rules = {
            { 0, -1, [](int height) { return make_shared<ScoreCommentConsensus>(height); }},
            { 430000, -1, [](int height) { return make_shared<ScoreCommentConsensus_checkpoint_430000>(height); }},
            { 514184, -1, [](int height) { return make_shared<ScoreCommentConsensus_checkpoint_514184>(height); }},
            { 1124000, -1, [](int height) { return make_shared<ScoreCommentConsensus_checkpoint_1124000>(height); }},
            { 1180000, 0, [](int height) { return make_shared<ScoreCommentConsensus_checkpoint_1180000>(height); }},
            { 1757000, 953000, [](int height) { return make_shared<ScoreCommentConsensus_checkpoint_disable_for_blocked>(height); }},
        };
    public:
        shared_ptr<ScoreCommentConsensus> Instance(int height)
        {
            int m_height = (height > 0 ? height : 0);
            return (--upper_bound(m_rules.begin(), m_rules.end(), m_height,
                [&](int target, const ConsensusCheckpoint<ScoreCommentConsensus>& itm)
                {
                    return target < itm.Height(Params().NetworkIDString());
                }
            ))->m_func(m_height);
        }
    };
}

#endif // POCKETCONSENSUS_SCORECOMMENT_HPP