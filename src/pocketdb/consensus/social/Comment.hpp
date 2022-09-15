// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETCONSENSUS_COMMENT_HPP
#define POCKETCONSENSUS_COMMENT_HPP

#include "utils/html.h"
#include "pocketdb/consensus/Reputation.h"
#include "pocketdb/consensus/Social.h"
#include "pocketdb/models/dto/content/Comment.h"

namespace PocketConsensus
{
    using namespace std;
    typedef shared_ptr<Comment> CommentRef;

    /*******************************************************************************************************************
    *  Comment consensus base class
    *******************************************************************************************************************/
    class CommentConsensus : public SocialConsensus<Comment>
    {
    public:
        CommentConsensus(int height) : SocialConsensus<Comment>(height) {}
        ConsensusValidateResult Validate(const CTransactionRef& tx, const CommentRef& ptx, const PocketBlockRef& block) override
        {
            // Parent comment
            if (!IsEmpty(ptx->GetParentTxHash()))
            {
                auto[ok, parentTx] = ConsensusRepoInst.GetLastContent(*ptx->GetParentTxHash(), { CONTENT_COMMENT, CONTENT_COMMENT_EDIT });

                if (!ok)
                    return {false, SocialConsensusResult_InvalidParentComment};
            }

            // Answer comment
            if (!IsEmpty(ptx->GetAnswerTxHash()))
            {
                auto[ok, answerTx] = ConsensusRepoInst.GetLastContent(*ptx->GetAnswerTxHash(), { CONTENT_COMMENT, CONTENT_COMMENT_EDIT });

                if (!ok)
                    return {false, SocialConsensusResult_InvalidParentComment};
            }

            // Check exists content transaction
            auto[contentOk, contentTx] = PocketDb::ConsensusRepoInst.GetLastContent(
                *ptx->GetPostTxHash(),
                { CONTENT_POST, CONTENT_VIDEO, CONTENT_ARTICLE, CONTENT_DELETE }
            );

            if (!contentOk)
                return {false, SocialConsensusResult_NotFound};

            if (*contentTx->GetType() == CONTENT_DELETE)
                return {false, SocialConsensusResult_CommentDeletedContent};

            // TODO (brangr): convert to Content base class
            // Check Blocking
            if (auto[existsBlocking, blockingType] = PocketDb::ConsensusRepoInst.GetLastBlockingType(
                    *contentTx->GetString1(), *ptx->GetAddress()
                ); existsBlocking && blockingType == ACTION_BLOCKING)
                return {false, SocialConsensusResult_Blocking};

            // Check payload size
            if (auto[ok, code] = ValidatePayloadSize(ptx); !ok)
                return {false, code};

            return SocialConsensus::Validate(tx, ptx, block);
        }
        ConsensusValidateResult Check(const CTransactionRef& tx, const CommentRef& ptx) override
        {
            if (auto[baseCheck, baseCheckCode] = SocialConsensus::Check(tx, ptx); !baseCheck)
                return {false, baseCheckCode};

            // Check required fields
            if (IsEmpty(ptx->GetAddress())) return {false, SocialConsensusResult_Failed};
            if (IsEmpty(ptx->GetPostTxHash())) return {false, SocialConsensusResult_Failed};

            // Maximum for message data
            if (!ptx->GetPayload()) return {false, SocialConsensusResult_Failed};
            if (IsEmpty(ptx->GetPayloadMsg())) return {false, SocialConsensusResult_Failed};
            if (HtmlUtils::UrlDecode(*ptx->GetPayloadMsg()).length() > (size_t)GetConsensusLimit(ConsensusLimit_max_comment_size))
                return {false, SocialConsensusResult_Size};

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
            int count = GetChainCount(ptx);
            count += ConsensusRepoInst.CountMempoolComment(*ptx->GetAddress());
            return ValidateLimit(ptx, count);
        }
        vector<string> GetAddressesForCheckRegistration(const CommentRef& ptx) override
        {
            return {*ptx->GetAddress()};
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
            auto reputationConsensus = PocketConsensus::ReputationConsensusFactoryInst.Instance(Height);
            auto[mode, reputation, balance] = reputationConsensus->GetAccountMode(*ptx->GetAddress());
            if (count >= GetLimit(mode))
                return {false, SocialConsensusResult_CommentLimit};

            return Success;
        }
        virtual int GetChainCount(const CommentRef& ptx)
        {
            return ConsensusRepoInst.CountChainCommentTime(
                *ptx->GetAddress(),
                *ptx->GetTime() - GetConsensusLimit(ConsensusLimit_depth)
            );
        }
        virtual ConsensusValidateResult ValidatePayloadSize(const CommentRef& ptx)
        {
            int64_t dataSize = (ptx->GetPayloadMsg() ? HtmlUtils::UrlDecode(*ptx->GetPayloadMsg()).size() : 0);

            if (dataSize > GetConsensusLimit(ConsensusLimit_max_comment_size))
                return {false, SocialConsensusResult_ContentSizeLimit};

            return Success;
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
        bool CheckBlockLimitTime(const CommentRef& ptx, const CommentRef& blockPtx) override
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
        int GetChainCount(const CommentRef& ptx) override
        {
            return ConsensusRepoInst.CountChainCommentHeight(
                *ptx->GetAddress(),
                Height - (int)GetConsensusLimit(ConsensusLimit_depth)
            );
        }
    };

    /*******************************************************************************************************************
    *  Factory for select actual rules version
    *******************************************************************************************************************/
    class CommentConsensusFactory
    {
    private:
        const vector<ConsensusCheckpoint < CommentConsensus>> m_rules = {
            { 0, -1, [](int height) { return make_shared<CommentConsensus>(height); }},
            { 1124000, -1, [](int height) { return make_shared<CommentConsensus_checkpoint_1124000>(height); }},
            { 1180000, 0, [](int height) { return make_shared<CommentConsensus_checkpoint_1180000>(height); }},
        };
    public:
        shared_ptr<CommentConsensus> Instance(int height)
        {
            int m_height = (height > 0 ? height : 0);
            return (--upper_bound(m_rules.begin(), m_rules.end(), m_height,
                [&](int target, const ConsensusCheckpoint<CommentConsensus>& itm)
                {
                    return target < itm.Height(Params().NetworkIDString());
                }
            ))->m_func(m_height);
        }
    };
}

#endif // POCKETCONSENSUS_COMMENT_HPP