// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETCONSENSUS_ARTICLE_H
#define POCKETCONSENSUS_ARTICLE_H

#include "pocketdb/consensus/Reputation.h"
#include "pocketdb/consensus/Social.h"
#include "pocketdb/models/dto/content/Article.h"

namespace PocketConsensus
{
    typedef shared_ptr<Article> ArticleRef;
    typedef shared_ptr<Content> ContentRef;

    class ArticleConsensus : public SocialConsensus<Article>
    {
    public:
        ArticleConsensus() : SocialConsensus<Article>()
        {
            // TODO (limits): set limits
            Limits.Set("payload_size", 120000, 60000, 60000);
        }

        tuple<bool, SocialConsensusResult> Validate(const CTransactionRef& tx, const ArticleRef& ptx, const PocketBlockRef& block) override
        {
            if (ptx->IsEdit())
                return ValidateEdit(ptx);

            return SocialConsensus::Validate(tx, ptx, block);
        }
        tuple<bool, SocialConsensusResult> Check(const CTransactionRef& tx, const ArticleRef& ptx) override
        {
            if (auto[baseCheck, baseCheckCode] = SocialConsensus::Check(tx, ptx); !baseCheck)
                return {false, baseCheckCode};

            // Check required fields
            if (IsEmpty(ptx->GetAddress())) return {false, ConsensusResult_Failed};

            // Repost not allowed
            if (!IsEmpty(ptx->GetRelayTxHash())) return {false, ConsensusResult_NotAllowed};

            return Success;
        }

    protected:

        virtual int64_t GetLimit(AccountMode mode)
        {
            return mode >= AccountMode_Full ? GetConsensusLimit(ConsensusLimit_full_article) : GetConsensusLimit(ConsensusLimit_trial_article);
        }

        tuple<bool, SocialConsensusResult> ValidateBlock(const ArticleRef& ptx, const PocketBlockRef& block) override
        {
            // Edit articles
            if (ptx->IsEdit())
                return ValidateEditBlock(ptx, block);

            // ---------------------------------------------------------
            // New articles

            // Get count from chain
            int count = GetChainCount(ptx);

            // Get count from block
            for (const auto& blockTx : *block)
            {
                if (!TransactionHelper::IsIn(*blockTx->GetType(), { CONTENT_ARTICLE }))
                    continue;

                const auto blockPtx = static_pointer_cast<Content>(blockTx);

                if (*ptx->GetAddress() != *blockPtx->GetAddress())
                    continue;

                if (blockPtx->IsEdit())
                    continue;

                if (*blockPtx->GetHash() == *ptx->GetHash())
                    continue;

                count += 1;
            }

            return ValidateLimit(ptx, count);
        }
        tuple<bool, SocialConsensusResult> ValidateMempool(const ArticleRef& ptx) override
        {
            // Edit articles
            if (ptx->IsEdit())
                return ValidateEditMempool(ptx);

            // ---------------------------------------------------------
            // New articles

            // Get count from chain
            int count = GetChainCount(ptx);

            // Get count from mempool
            count += ConsensusRepoInst.CountMempoolArticle(*ptx->GetAddress());

            return ValidateLimit(ptx, count);
        }
        vector<string> GetAddressesForCheckRegistration(const ArticleRef& ptx) override
        {
            return {*ptx->GetAddress()};
        }

        virtual tuple<bool, SocialConsensusResult> ValidateEdit(const ArticleRef& ptx)
        {
            auto[lastContentOk, lastContent] = PocketDb::ConsensusRepoInst.GetLastContent(
                *ptx->GetRootTxHash(),
                { CONTENT_ARTICLE }
            );
            if (!lastContentOk)
                return {false, ConsensusResult_NotFound};

            // First get original post transaction
            auto[originalTxOk, originalTx] = PocketDb::ConsensusRepoInst.GetFirstContent(*ptx->GetRootTxHash());
            if (!originalTxOk)
                return {false, ConsensusResult_NotFound};

            const auto originalPtx = static_pointer_cast<Content>(originalTx);

            // Change type not allowed
            if (*originalPtx->GetType() != *ptx->GetType())
                return {false, ConsensusResult_NotAllowed};

            // You are author? Really?
            if (*ptx->GetAddress() != *originalPtx->GetAddress())
                return {false, ConsensusResult_ContentEditUnauthorized};

            // Original post edit only 24 hours
            if (!AllowEditWindow(ptx, originalPtx))
                return {false, ConsensusResult_ContentEditLimit};

            // Check edit limit
            return ValidateEditOneLimit(ptx);
        }
        virtual tuple<bool, SocialConsensusResult> ValidateLimit(const ArticleRef& ptx, int count)
        {
            auto reputationConsensus = PocketConsensus::ConsensusFactoryInst_Reputation.Instance(Height);
            auto address = ptx->GetAddress();
            auto[mode, reputation, balance] = reputationConsensus->GetAccountMode(*address);
            if (count >= GetLimit(mode))
            {
                if (!CheckpointRepoInst.IsSocialCheckpoint(*ptx->GetHash(), *ptx->GetType(), ConsensusResult_ContentLimit))
                    return {false, ConsensusResult_ContentLimit};
            }

            return Success;
        }

        virtual bool AllowEditWindow(const ArticleRef& ptx, const ContentRef& originalPtx)
        {
            auto[ok, originalPtxHeight] = ConsensusRepoInst.GetTransactionHeight(*originalPtx->GetHash());
            if (!ok)
                return false;

            return (Height - originalPtxHeight) <= GetConsensusLimit(ConsensusLimit_edit_article_depth);
        }
        virtual int GetChainCount(const ArticleRef& ptx)
        {
            return ConsensusRepoInst.CountChainHeight(
                *ptx->GetType(),
                *ptx->GetAddress()
            );
        }
        virtual tuple<bool, SocialConsensusResult> ValidateEditBlock(const ArticleRef& ptx, const PocketBlockRef& block)
        {
            // Double edit in block not allowed
            for (auto& blockTx : *block)
            {
                if (!TransactionHelper::IsIn(*blockTx->GetType(), {CONTENT_ARTICLE, CONTENT_DELETE}))
                    continue;

                auto blockPtx = static_pointer_cast<Content>(blockTx);

                if (*blockPtx->GetHash() == *ptx->GetHash())
                    continue;

                if (*ptx->GetRootTxHash() == *blockPtx->GetRootTxHash())
                    return {false, ConsensusResult_DoubleContentEdit};
            }

            // Check edit limit
            return ValidateEditOneLimit(ptx);
        }
        virtual tuple<bool, SocialConsensusResult> ValidateEditMempool(const ArticleRef& ptx)
        {
            if (ConsensusRepoInst.CountMempoolArticleEdit(*ptx->GetAddress(), *ptx->GetRootTxHash()) > 0)
                return {false, ConsensusResult_DoubleContentEdit};

            // Check edit limit
            return ValidateEditOneLimit(ptx);
        }
        virtual tuple<bool, SocialConsensusResult> ValidateEditOneLimit(const ArticleRef& ptx)
        {
            int count = ConsensusRepoInst.CountChainArticleEdit(*ptx->GetAddress(), *ptx->GetRootTxHash());
            if (count >= GetConsensusLimit(ConsensusLimit_article_edit_count))
                return {false, ConsensusResult_ContentEditLimit};

            return Success;
        }
    };
    
    // Fix general validating
    class ArticleConsensus_checkpoint_tmp_fix : public ArticleConsensus
    {
    public:
        ArticleConsensus_checkpoint_tmp_fix() : ArticleConsensus() {}
        
        tuple<bool, SocialConsensusResult> Validate(const CTransactionRef& tx, const ArticleRef& ptx, const PocketBlockRef& block) override
        {
            // Base validation with calling block or mempool check
            if (auto[baseValidate, baseValidateCode] = SocialConsensus::Validate(tx, ptx, block); !baseValidate)
                return {false, baseValidateCode};

            if (ptx->IsEdit())
                return ValidateEdit(ptx);

            return Success;
        }
    };


    class ArticleConsensusFactory : public BaseConsensusFactory<ArticleConsensus>
    {
    public:
        ArticleConsensusFactory()
        {
            Checkpoint({       0,       0, -1, make_shared<ArticleConsensus>() });
            Checkpoint({ 2552000, 2280000,  0, make_shared<ArticleConsensus_checkpoint_tmp_fix>() });
        }
    };

    static ArticleConsensusFactory ConsensusFactoryInst_Article;
}

#endif // POCKETCONSENSUS_ARTICLE_H