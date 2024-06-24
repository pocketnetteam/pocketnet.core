// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETCONSENSUS_COMMENT_HPP
#define POCKETCONSENSUS_COMMENT_HPP

#include "util/html.h"
#include "pocketdb/consensus/Reputation.h"
#include "pocketdb/consensus/Social.h"
#include "pocketdb/models/dto/content/Comment.h"

namespace PocketConsensus
{
    typedef shared_ptr<Comment> CommentRef;

    /*******************************************************************************************************************
    *  Comment consensus base class
    *******************************************************************************************************************/
    class CommentConsensus : public SocialConsensus<Comment>
    {
    public:
        CommentConsensus() : SocialConsensus<Comment>()
        {
            // TODO (limits): set limits
            Limits.Set("payload_size", 2000, 2000, 2000);
        }

        ConsensusValidateResult Validate(const CTransactionRef& tx, const CommentRef& ptx, const PocketBlockRef& block) override
        {
            // Parent comment
            if (!IsEmpty(ptx->GetParentTxHash()))
            {
                auto[ok, parentTx] = ConsensusRepoInst.GetLastContent(*ptx->GetParentTxHash(), { CONTENT_COMMENT, CONTENT_COMMENT_EDIT });

                if (!ok)
                    return {false, ConsensusResult_InvalidParentComment};
            }

            // Answer comment
            if (!IsEmpty(ptx->GetAnswerTxHash()))
            {
                auto[ok, answerTx] = ConsensusRepoInst.GetLastContent(*ptx->GetAnswerTxHash(), { CONTENT_COMMENT, CONTENT_COMMENT_EDIT });

                if (!ok)
                    return {false, ConsensusResult_InvalidParentComment};
            }

            // Check exists content transaction
            auto[contentOk, contentTx] = PocketDb::ConsensusRepoInst.GetLastContent(
                *ptx->GetPostTxHash(),
                { CONTENT_POST, CONTENT_VIDEO, CONTENT_ARTICLE, CONTENT_STREAM, CONTENT_AUDIO, CONTENT_DELETE, BARTERON_OFFER, APP }
            );

            if (!contentOk)
                return {false, ConsensusResult_NotFound};

            if (*contentTx->GetType() == CONTENT_DELETE)
                return {false, ConsensusResult_CommentDeletedContent};

            // Check Blocking
            if (ValidateBlocking(*contentTx->GetString1(), *ptx->GetAddress()))
                return {false, ConsensusResult_Blocking};

            return SocialConsensus::Validate(tx, ptx, block);
        }
        ConsensusValidateResult Check(const CTransactionRef& tx, const CommentRef& ptx) override
        {
            if (auto[baseCheck, baseCheckCode] = SocialConsensus::Check(tx, ptx); !baseCheck)
                return {false, baseCheckCode};

            // Check required fields
            if (IsEmpty(ptx->GetAddress())) return {false, ConsensusResult_Failed};
            if (IsEmpty(ptx->GetPostTxHash())) return {false, ConsensusResult_Failed};

            // Maximum for message data
            if (!ptx->GetPayload()) return {false, ConsensusResult_Failed};
            if (IsEmpty(ptx->GetPayloadMsg())) return {false, ConsensusResult_Failed};

            return Success;
        }

    protected:
        ConsensusValidateResult ValidateBlock(const CommentRef& ptx, const PocketBlockRef& block) override
        {
            int count = GetChainCount(ptx);
            for (auto& blockTx : *block)
            {
                if (!TransactionHelper::IsIn(*blockTx->GetType(), {CONTENT_COMMENT}))
                    continue;

                auto blockPtx = static_pointer_cast<Comment>(blockTx);

                if (*blockPtx->GetHash() == *ptx->GetHash())
                    continue;

                if (*ptx->GetAddress() == *blockPtx->GetAddress())
                {
                    if (CheckBlockLimitTime(ptx, blockPtx))
                        count += 1;
                }
            }

            return ValidateLimit(ptx, count);
        }
        ConsensusValidateResult ValidateMempool(const CommentRef& ptx) override
        {
            int count = GetChainCount(ptx) + ConsensusRepoInst.CountMempoolComment(*ptx->GetAddress());
            return ValidateLimit(ptx, count);
        }
        vector<string> GetAddressesForCheckRegistration(const CommentRef& ptx) override
        {
            return { *ptx->GetAddress() };
        }

        virtual bool ValidateBlocking(const string& address1, const string& address2)
        {
            auto[existsBlocking, blockingType] = PocketDb::ConsensusRepoInst.GetLastBlockingType(address1, address2);
            return existsBlocking && blockingType == ACTION_BLOCKING;
        }
        virtual int64_t GetLimit(AccountMode mode) { 
            return mode >= AccountMode_Full ? 
                GetConsensusLimit(ConsensusLimit_full_comment) : 
                GetConsensusLimit(ConsensusLimit_trial_comment); 
        }
        virtual bool CheckBlockLimitTime(const CommentRef& ptx, const CommentRef& blockPtx)
        {
            return *blockPtx->GetTime() <= *ptx->GetTime();
        }
        virtual ConsensusValidateResult ValidateLimit(const CommentRef& ptx, int count)
        {
            auto reputationConsensus = PocketConsensus::ConsensusFactoryInst_Reputation.Instance(Height);
            auto address = ptx->GetAddress();
            auto[mode, reputation, balance] = reputationConsensus->GetAccountMode(*address);
            if (count >= GetLimit(mode))
                return {false, ConsensusResult_CommentLimit};

            return Success;
        }
        virtual int GetChainCount(const CommentRef& ptx)
        {
            return ConsensusRepoInst.CountChainCommentTime(
                *ptx->GetAddress(),
                *ptx->GetTime() - GetConsensusLimit(ConsensusLimit_depth)
            );
        }
    };

    // ----------------------------------------------------------------------------------------------
    class CommentConsensus_checkpoint_1124000 : public CommentConsensus
    {
    public:
        CommentConsensus_checkpoint_1124000() : CommentConsensus() {}
    protected:
        bool CheckBlockLimitTime(const CommentRef& ptx, const CommentRef& blockPtx) override
        {
            return true;
        }
    };

    // ----------------------------------------------------------------------------------------------
    class CommentConsensus_checkpoint_1180000 : public CommentConsensus_checkpoint_1124000
    {
    public:
        CommentConsensus_checkpoint_1180000() : CommentConsensus_checkpoint_1124000() {}
    protected:
        int GetChainCount(const CommentRef& ptx) override
        {
            return ConsensusRepoInst.CountChainCommentHeight(
                *ptx->GetAddress(),
                Height - (int)GetConsensusLimit(ConsensusLimit_depth)
            );
        }
    };

    // ----------------------------------------------------------------------------------------------
    class CommentConsensus_checkpoint_pip_105 : public CommentConsensus_checkpoint_1180000
    {
    public:
        CommentConsensus_checkpoint_pip_105() : CommentConsensus_checkpoint_1180000() {}
    protected:
        bool ValidateBlocking(const string& address1, const string& address2) override
        {
            return SocialConsensus::CheckBlocking(address1, address2);
        }
    };


    // ----------------------------------------------------------------------------------------------
    // Factory for select actual rules version
    class CommentConsensusFactory : public BaseConsensusFactory<CommentConsensus>
    {
    public:
        CommentConsensusFactory()
        {
            Checkpoint({       0,      -1, -1, make_shared<CommentConsensus>() });
            Checkpoint({ 1124000,      -1, -1, make_shared<CommentConsensus_checkpoint_1124000>() });
            Checkpoint({ 1180000,       0, -1, make_shared<CommentConsensus_checkpoint_1180000>() });
            Checkpoint({ 2794500, 2574300,  0, make_shared<CommentConsensus_checkpoint_pip_105>() });
        }
    };

    static CommentConsensusFactory ConsensusFactoryInst_Comment;
}

#endif // POCKETCONSENSUS_COMMENT_HPP