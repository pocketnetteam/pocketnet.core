// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include "pocketdb/consensus/Helper.h"

namespace PocketConsensus
{
    PostConsensusFactory SocialConsensusHelper::m_postFactory;
    VideoConsensusFactory SocialConsensusHelper::m_videoFactory;
    ArticleConsensusFactory SocialConsensusHelper::m_articleFactory;
    AccountSettingConsensusFactory SocialConsensusHelper::m_accountSettingFactory;
    AccountDeleteConsensusFactory SocialConsensusHelper::m_accountDeleteFactory;
    AccountUserConsensusFactory SocialConsensusHelper::m_accountUserFactory;
    CommentConsensusFactory SocialConsensusHelper::m_commentFactory;
    CommentEditConsensusFactory SocialConsensusHelper::m_commentEditFactory;
    CommentDeleteConsensusFactory SocialConsensusHelper::m_commentDeleteFactory;
    ScoreContentConsensusFactory SocialConsensusHelper::m_scoreContentFactory;
    ScoreCommentConsensusFactory SocialConsensusHelper::m_scoreCommentFactory;
    SubscribeConsensusFactory SocialConsensusHelper::m_subscribeFactory;
    SubscribePrivateConsensusFactory SocialConsensusHelper::m_subscribePrivateFactory;
    SubscribeCancelConsensusFactory SocialConsensusHelper::m_subscribeCancelFactory;
    BlockingConsensusFactory SocialConsensusHelper::m_blockingFactory;
    BlockingCancelConsensusFactory SocialConsensusHelper::m_blockingCancelFactory;
    ComplainConsensusFactory SocialConsensusHelper::m_complainFactory;
    ContentDeleteConsensusFactory SocialConsensusHelper::m_contentDeleteFactory;
    BoostContentConsensusFactory SocialConsensusHelper::m_boostContentFactory;
    
    ModerationFlagConsensusFactory SocialConsensusHelper::m_moderationFlagFactory;

    tuple<bool, SocialConsensusResult> SocialConsensusHelper::Validate(const CBlock& block, const PocketBlockRef& pBlock, int height)
    {
        for (const auto& tx : block.vtx)
        {
            // We have to verify all transactions using consensus
            // The presence of data in pBlock is checked in the `check` function
            auto txHash = tx->GetHash().GetHex();
            auto it = find_if(pBlock->begin(), pBlock->end(), [&](PTransactionRef const& ptx) { return *ptx == txHash; });

            // Validate founded data
            if (it != pBlock->end())
            {
                if (auto[ok, result] = validate(tx, *it, pBlock, height); !ok)
                {
                    LogPrint(BCLog::CONSENSUS,
                        "Warning: SocialConsensus type:%d validate tx:%s blk:%s failed with result:%d at height:%d\n",
                        (int) *(*it)->GetType(), txHash, block.GetHash().GetHex(), (int) result, height);

                    return {false, result};
                }
            }
        }

        return {true, SocialConsensusResult_Success};
    }

    tuple<bool, SocialConsensusResult> SocialConsensusHelper::Validate(const CTransactionRef& tx, const PTransactionRef& ptx, int height)
    {
        // Not double validate for already in DB
        if (TransRepoInst.Exists(*ptx->GetHash()))
            return {true, SocialConsensusResult_Success};

        if (auto[ok, result] = validate(tx, ptx, nullptr, height); !ok)
        {
            LogPrint(BCLog::CONSENSUS, "Warning: SocialConsensus type:%d validate failed with result:%d for tx:%s at height:%d\n",
                (int) *ptx->GetType(), (int)result, *ptx->GetHash(), height);

            return {false, result};
        }

        return {true, SocialConsensusResult_Success};
    }

    tuple<bool, SocialConsensusResult> SocialConsensusHelper::Validate(const CTransactionRef& tx, const PTransactionRef& ptx, PocketBlockRef& pBlock, int height)
    {
        if (auto[ok, result] = validate(tx, ptx, pBlock, height); !ok)
        {
            LogPrint(BCLog::CONSENSUS, "Warning: SocialConsensus type:%d validate tx:%s failed with result:%d for block construction at height:%d\n",
                (int)*ptx->GetType(), *ptx->GetHash(), (int)result, height);

            return {false, result};
        }

        return {true, SocialConsensusResult_Success};
    }

    // Проверяет блок транзакций без привязки к цепи
    tuple<bool, SocialConsensusResult> SocialConsensusHelper::Check(const CBlock& block, const PocketBlockRef& pBlock, int height)
    {
        if (!pBlock)
        {
            LogPrint(BCLog::CONSENSUS, "Warning: SocialConsensus check failed with result:%d for blk:%s at height:%d\n",
                (int)SocialConsensusResult_PocketDataNotFound, block.GetHash().GetHex(), height);

            return {false, SocialConsensusResult_PocketDataNotFound};
        }

        // Detect block type
        auto coinstakeBlock = find_if(block.vtx.begin(), block.vtx.end(), [&](CTransactionRef const& tx)
        {
            return tx->IsCoinStake();
        }) != block.vtx.end();

        // Check all transactions in block and payload block
        for (const auto& tx : block.vtx)
        {
            // NOT_SUPPORTED transactions not checked
            auto txType = PocketHelpers::TransactionHelper::ParseType(tx);
            if (txType == TxType::NOT_SUPPORTED)
                continue;
            if (coinstakeBlock && txType == TxType::TX_COINBASE)
                continue;

            // Maybe payload not exists?
            auto txHash = tx->GetHash().GetHex();
            auto it = find_if(pBlock->begin(), pBlock->end(), [&](PTransactionRef const& ptx) { return *ptx == txHash; });
            if (it == pBlock->end())
            {
                LogPrint(BCLog::CONSENSUS, "Warning: SocialConsensus type:%d check failed with result:%d for tx:%s in blk:%s at height:%d\n",
                    (int)txType, (int)SocialConsensusResult_PocketDataNotFound, tx->GetHash().GetHex(), block.GetHash().GetHex(), height);

                return {false, SocialConsensusResult_PocketDataNotFound};
            }

            // Check founded payload
            if (auto[ok, result] = check(tx, *it, height); !ok)
            {
                LogPrint(BCLog::CONSENSUS, "Warning: SocialConsensus check type:%d failed with result:%d for tx:%s in blk:%s at height:%d\n",
                    (int)txType, (int)result, tx->GetHash().GetHex(), block.GetHash().GetHex(), height);

                return {false, result};
            }
        }

        return {true, SocialConsensusResult_Success};
    }

    // Проверяет транзакцию без привязки к цепи
    tuple<bool, SocialConsensusResult> SocialConsensusHelper::Check(const CTransactionRef& tx, const PTransactionRef& ptx, int height)
    {
        if (auto[ok, result] = check(tx, ptx, height); !ok)
        {
            LogPrint(BCLog::CONSENSUS, "Warning: SocialConsensus type:%d check failed with result:%d for tx:%s at height:%d\n",
                (int) *ptx->GetType(), (int)result, *ptx->GetHash(), height);

            return {false, result};
        }

        return {true, SocialConsensusResult_Success};
    }

    // -----------------------------------------------------------------

    tuple<bool, SocialConsensusResult> SocialConsensusHelper::check(const CTransactionRef& tx, const PTransactionRef& ptx, int height)
    {
        if (!isConsensusable(*ptx->GetType()))
            return {true, SocialConsensusResult_Success};

        // Check transactions with consensus logic
        tuple<bool, SocialConsensusResult> result;
        switch (*ptx->GetType())
        {
            case ACCOUNT_SETTING:
                return m_accountSettingFactory.Instance(height)->Check(tx, static_pointer_cast<AccountSetting>(ptx));
            case ACCOUNT_DELETE:
                return m_accountDeleteFactory.Instance(height)->Check(tx, static_pointer_cast<AccountDelete>(ptx));
            case ACCOUNT_USER:
                return m_accountUserFactory.Instance(height)->Check(tx, static_pointer_cast<User>(ptx));
            case CONTENT_POST:
                return m_postFactory.Instance(height)->Check(tx, static_pointer_cast<Post>(ptx));
            case CONTENT_VIDEO:
                return m_videoFactory.Instance(height)->Check(tx, static_pointer_cast<Video>(ptx));
            case CONTENT_ARTICLE:
                return m_articleFactory.Instance(height)->Check(tx, static_pointer_cast<Article>(ptx));
            case CONTENT_COMMENT:
                return m_commentFactory.Instance(height)->Check(tx, static_pointer_cast<Comment>(ptx));
            case CONTENT_COMMENT_EDIT:
                return m_commentEditFactory.Instance(height)->Check(tx, static_pointer_cast<CommentEdit>(ptx));
            case CONTENT_COMMENT_DELETE:
                return m_commentDeleteFactory.Instance(height)->Check(tx, static_pointer_cast<CommentDelete>(ptx));
            case CONTENT_DELETE:
                return m_contentDeleteFactory.Instance(height)->Check(tx, static_pointer_cast<ContentDelete>(ptx));
            case BOOST_CONTENT:
                return m_boostContentFactory.Instance(height)->Check(tx, static_pointer_cast<BoostContent>(ptx));
            case ACTION_SCORE_CONTENT:
                return m_scoreContentFactory.Instance(height)->Check(tx, static_pointer_cast<ScoreContent>(ptx));
            case ACTION_SCORE_COMMENT:
                return m_scoreCommentFactory.Instance(height)->Check(tx, static_pointer_cast<ScoreComment>(ptx));
            case ACTION_SUBSCRIBE:
                return m_subscribeFactory.Instance(height)->Check(tx, static_pointer_cast<Subscribe>(ptx));
            case ACTION_SUBSCRIBE_PRIVATE:
                return m_subscribePrivateFactory.Instance(height)->Check(tx, static_pointer_cast<SubscribePrivate>(ptx));
            case ACTION_SUBSCRIBE_CANCEL:
                return m_subscribeCancelFactory.Instance(height)->Check(tx, static_pointer_cast<SubscribeCancel>(ptx));
            case ACTION_BLOCKING:
                return m_blockingFactory.Instance(height)->Check(tx, static_pointer_cast<Blocking>(ptx));
            case ACTION_BLOCKING_CANCEL:
                return m_blockingCancelFactory.Instance(height)->Check(tx, static_pointer_cast<BlockingCancel>(ptx));
            case ACTION_COMPLAIN:
                return m_complainFactory.Instance(height)->Check(tx, static_pointer_cast<Complain>(ptx));

            // Moderation
            case MODERATION_FLAG:
                return m_moderationFlagFactory.Instance(height)->Check(tx, static_pointer_cast<ModerationFlag>(ptx));

            default:
                return {false, SocialConsensusResult_NotImplemeted};
        }
    }

    tuple<bool, SocialConsensusResult> SocialConsensusHelper::validate(const CTransactionRef& tx, const PTransactionRef& ptx, const PocketBlockRef& pBlock, int height)
    {
        if (!isConsensusable(*ptx->GetType()))
            return {true, SocialConsensusResult_Success};

        // Validate transactions with consensus logic
        tuple<bool, SocialConsensusResult> result;
        switch (*ptx->GetType())
        {
            case ACCOUNT_SETTING:
                return m_accountSettingFactory.Instance(height)->Validate(tx, static_pointer_cast<AccountSetting>(ptx), pBlock);
            case ACCOUNT_DELETE:
                return m_accountDeleteFactory.Instance(height)->Validate(tx, static_pointer_cast<AccountDelete>(ptx), pBlock);
            case ACCOUNT_USER:
                return m_accountUserFactory.Instance(height)->Validate(tx, static_pointer_cast<User>(ptx), pBlock);
            case CONTENT_POST:
                return m_postFactory.Instance(height)->Validate(tx, static_pointer_cast<Post>(ptx), pBlock);
            case CONTENT_VIDEO:
                return m_videoFactory.Instance(height)->Validate(tx, static_pointer_cast<Video>(ptx), pBlock);
            case CONTENT_ARTICLE:
                return m_articleFactory.Instance(height)->Validate(tx, static_pointer_cast<Article>(ptx), pBlock);
            case CONTENT_COMMENT:
                return m_commentFactory.Instance(height)->Validate(tx, static_pointer_cast<Comment>(ptx), pBlock);
            case CONTENT_COMMENT_EDIT:
                return m_commentEditFactory.Instance(height)->Validate(tx, static_pointer_cast<CommentEdit>(ptx), pBlock);
            case CONTENT_COMMENT_DELETE:
                return m_commentDeleteFactory.Instance(height)->Validate(tx, static_pointer_cast<CommentDelete>(ptx), pBlock);
            case CONTENT_DELETE:
                return m_contentDeleteFactory.Instance(height)->Validate(tx, static_pointer_cast<ContentDelete>(ptx), pBlock);
            case BOOST_CONTENT:
                return m_boostContentFactory.Instance(height)->Validate(tx, static_pointer_cast<BoostContent>(ptx), pBlock);
            case ACTION_SCORE_CONTENT:
                return m_scoreContentFactory.Instance(height)->Validate(tx, static_pointer_cast<ScoreContent>(ptx), pBlock);
            case ACTION_SCORE_COMMENT:
                return m_scoreCommentFactory.Instance(height)->Validate(tx, static_pointer_cast<ScoreComment>(ptx), pBlock);
            case ACTION_SUBSCRIBE:
                return m_subscribeFactory.Instance(height)->Validate(tx, static_pointer_cast<Subscribe>(ptx), pBlock);
            case ACTION_SUBSCRIBE_PRIVATE:
                return m_subscribePrivateFactory.Instance(height)->Validate(tx, static_pointer_cast<SubscribePrivate>(ptx), pBlock);
            case ACTION_SUBSCRIBE_CANCEL:
                return m_subscribeCancelFactory.Instance(height)->Validate(tx, static_pointer_cast<SubscribeCancel>(ptx), pBlock);
            case ACTION_BLOCKING:
                return m_blockingFactory.Instance(height)->Validate(tx, static_pointer_cast<Blocking>(ptx), pBlock);
            case ACTION_BLOCKING_CANCEL:
                return m_blockingCancelFactory.Instance(height)->Validate(tx, static_pointer_cast<BlockingCancel>(ptx), pBlock);
            case ACTION_COMPLAIN:
                return m_complainFactory.Instance(height)->Validate(tx, static_pointer_cast<Complain>(ptx), pBlock);

            // Moderation
            case MODERATION_FLAG:
                return m_moderationFlagFactory.Instance(height)->Validate(tx, static_pointer_cast<ModerationFlag>(ptx), pBlock);

            default:
                return {false, SocialConsensusResult_NotImplemeted};
        }
    }

    bool SocialConsensusHelper::isConsensusable(TxType txType)
    {
        switch (txType)
        {
            case NOT_SUPPORTED:
            case TX_COINBASE:
            case TX_COINSTAKE:
            case TX_DEFAULT:
                return false;
            default:
                return true;
        }
    }

}
