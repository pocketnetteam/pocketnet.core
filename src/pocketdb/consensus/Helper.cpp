// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include "pocketdb/consensus/Helper.h"

namespace PocketConsensus
{
    PostConsensusFactory SocialConsensusHelper::m_postFactory;
    AccountSettingConsensusFactory SocialConsensusHelper::m_accountSettingFactory;
    UserConsensusFactory SocialConsensusHelper::m_userFactory;
    VideoConsensusFactory SocialConsensusHelper::m_videoFactory;
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

    bool SocialConsensusHelper::Validate(const PocketBlockRef& block, int height)
    {
        for (const auto& ptx : *block)
        {
            if (auto[ok, result] = validate(ptx, block, height); !ok)
                return false;
        }

        return true;
    }

    tuple<bool, SocialConsensusResult> SocialConsensusHelper::Validate(const PTransactionRef& tx, int height)
    {
        return validate(tx, nullptr, height);
    }

    tuple<bool, SocialConsensusResult> SocialConsensusHelper::Validate(const PTransactionRef& tx, PocketBlockRef& block, int height)
    {
        return validate(tx, block, height);
    }

    // Проверяет блок транзакций без привязки к цепи
    bool SocialConsensusHelper::Check(const CBlock& block, const PocketBlockRef& pBlock)
    {
        auto coinstakeBlock = find_if(block.vtx.begin(), block.vtx.end(), [&](CTransactionRef const& tx)
        {
            return tx->IsCoinStake();
        }) != block.vtx.end();

        for (const auto& tx : block.vtx)
        {
            // NOT_SUPPORTED transactions not checked
            auto txType = PocketHelpers::TransactionHelper::ParseType(tx);
            if (txType == PocketTxType::NOT_SUPPORTED)
                continue;
            if (coinstakeBlock && txType == PocketTxType::TX_COINBASE)
                continue;

            // Maybe payload not exists?
            auto txHash = tx->GetHash().GetHex();
            auto it = find_if(pBlock->begin(), pBlock->end(), [&](PTransactionRef const& ptx) { return *ptx == txHash; });
            if (it == pBlock->end())
                return false;

            // Check founded payload
            if (auto[ok, result] = check(tx, *it); !ok)
                return false;
        }

        return true;
    }

    // Проверяет транзакцию без привязки к цепи
    tuple<bool, SocialConsensusResult> SocialConsensusHelper::Check(const CTransactionRef& tx, const PTransactionRef& ptx)
    {
        return check(tx, ptx);
    }

    tuple<bool, SocialConsensusResult> SocialConsensusHelper::check(const CTransactionRef& tx, const PTransactionRef& ptx)
    {
        // TODO (brangr): implement base check for base transactions
        // check outputs maybe?

        if (!isConsensusable(*ptx->GetType()))
            return {true, SocialConsensusResult_Success};

        tuple<bool, SocialConsensusResult> result;
        switch (*ptx->GetType())
        {
            case ACCOUNT_SETTING:
                result = m_accountSettingFactory.Instance(0)->Check(tx, static_pointer_cast<AccountSetting>(ptx));
                break;
            case ACCOUNT_USER:
                result = m_userFactory.Instance(0)->Check(tx, static_pointer_cast<User>(ptx));
                break;
            case CONTENT_POST:
                result = m_postFactory.Instance(0)->Check(tx, static_pointer_cast<Post>(ptx));
                break;
            case CONTENT_VIDEO:
                result = m_videoFactory.Instance(0)->Check(tx, static_pointer_cast<Video>(ptx));
                break;
            case CONTENT_COMMENT:
                result = m_commentFactory.Instance(0)->Check(tx, static_pointer_cast<Comment>(ptx));
                break;
            case CONTENT_COMMENT_EDIT:
                result = m_commentEditFactory.Instance(0)->Check(tx, static_pointer_cast<CommentEdit>(ptx));
                break;
            case CONTENT_COMMENT_DELETE:
                result = m_commentDeleteFactory.Instance(0)->Check(tx, static_pointer_cast<CommentDelete>(ptx));
                break;
            case CONTENT_DELETE:
                result = m_contentDeleteFactory.Instance(0)->Check(tx, static_pointer_cast<ContentDelete>(ptx));
                break;
            case ACTION_SCORE_CONTENT:
                result = m_scoreContentFactory.Instance(0)->Check(tx, static_pointer_cast<ScoreContent>(ptx));
                break;
            case ACTION_SCORE_COMMENT:
                result = m_scoreCommentFactory.Instance(0)->Check(tx, static_pointer_cast<ScoreComment>(ptx));
                break;
            case ACTION_SUBSCRIBE:
                result = m_subscribeFactory.Instance(0)->Check(tx, static_pointer_cast<Subscribe>(ptx));
                break;
            case ACTION_SUBSCRIBE_PRIVATE:
                result = m_subscribePrivateFactory.Instance(0)->Check(tx, static_pointer_cast<SubscribePrivate>(ptx));
                break;
            case ACTION_SUBSCRIBE_CANCEL:
                result = m_subscribeCancelFactory.Instance(0)->Check(tx, static_pointer_cast<SubscribeCancel>(ptx));
                break;
            case ACTION_BLOCKING:
                result = m_blockingFactory.Instance(0)->Check(tx, static_pointer_cast<Blocking>(ptx));
                break;
            case ACTION_BLOCKING_CANCEL:
                result = m_blockingCancelFactory.Instance(0)->Check(tx, static_pointer_cast<BlockingCancel>(ptx));
                break;
            case ACTION_COMPLAIN:
                result = m_complainFactory.Instance(0)->Check(tx, static_pointer_cast<Complain>(ptx));
                break;
            case ACCOUNT_VIDEO_SERVER:
            case ACCOUNT_MESSAGE_SERVER:
            case CONTENT_TRANSLATE:
            case CONTENT_SERVERPING:
                // TODO (brangr): future realize types
                break;
            default:
                break;
        }

        if (auto[ok, code] = result; !ok)
        {
            LogPrintf("Warning: SocialConsensus %d check failed with result %d for transaction %s\n",
                *ptx->GetTypeInt(), (int) code, *ptx->GetHash());

            return {false, code};
        }

        return {true, SocialConsensusResult_Success};
    }

    bool SocialConsensusHelper::isConsensusable(PocketTxType txType)
    {
        switch (txType)
        {
            case TX_COINBASE:
            case TX_COINSTAKE:
            case TX_DEFAULT:
                return false;
            default:
                return true;
        }
    }

    tuple<bool, SocialConsensusResult> SocialConsensusHelper::validate(const PTransactionRef& ptx, const PocketBlockRef& block, int height)
    {
        if (!isConsensusable(*ptx->GetType()))
            return {true, SocialConsensusResult_Success};

        tuple<bool, SocialConsensusResult> result;
        switch (*ptx->GetType())
        {
            case ACCOUNT_SETTING:
                result = m_accountSettingFactory.Instance(height)->Validate(static_pointer_cast<AccountSetting>(ptx), block);
                break;
            case ACCOUNT_USER:
                result = m_userFactory.Instance(height)->Validate(static_pointer_cast<User>(ptx), block);
                break;
            case CONTENT_POST:
                result = m_postFactory.Instance(height)->Validate(static_pointer_cast<Post>(ptx), block);
                break;
            case CONTENT_VIDEO:
                result = m_videoFactory.Instance(height)->Validate(static_pointer_cast<Video>(ptx), block);
                break;
            case CONTENT_COMMENT:
                result = m_commentFactory.Instance(height)->Validate(static_pointer_cast<Comment>(ptx), block);
                break;
            case CONTENT_COMMENT_EDIT:
                result = m_commentEditFactory.Instance(height)->Validate(static_pointer_cast<CommentEdit>(ptx), block);
                break;
            case CONTENT_COMMENT_DELETE:
                result = m_commentDeleteFactory.Instance(height)->Validate(static_pointer_cast<CommentDelete>(ptx), block);
                break;
            case CONTENT_DELETE:
                result = m_contentDeleteFactory.Instance(height)->Validate(static_pointer_cast<ContentDelete>(ptx), block);
                break;
            case ACTION_SCORE_CONTENT:
                result = m_scoreContentFactory.Instance(height)->Validate(static_pointer_cast<ScoreContent>(ptx), block);
                break;
            case ACTION_SCORE_COMMENT:
                result = m_scoreCommentFactory.Instance(height)->Validate(static_pointer_cast<ScoreComment>(ptx), block);
                break;
            case ACTION_SUBSCRIBE:
                result = m_subscribeFactory.Instance(height)->Validate(static_pointer_cast<Subscribe>(ptx), block);
                break;
            case ACTION_SUBSCRIBE_PRIVATE:
                result = m_subscribePrivateFactory.Instance(height)->Validate(static_pointer_cast<SubscribePrivate>(ptx), block);
                break;
            case ACTION_SUBSCRIBE_CANCEL:
                result = m_subscribeCancelFactory.Instance(height)->Validate(static_pointer_cast<SubscribeCancel>(ptx), block);
                break;
            case ACTION_BLOCKING:
                result = m_blockingFactory.Instance(height)->Validate(static_pointer_cast<Blocking>(ptx), block);
                break;
            case ACTION_BLOCKING_CANCEL:
                result = m_blockingCancelFactory.Instance(height)->Validate(static_pointer_cast<BlockingCancel>(ptx), block);
                break;
            case ACTION_COMPLAIN:
                result = m_complainFactory.Instance(height)->Validate(static_pointer_cast<Complain>(ptx), block);
                break;
            case ACCOUNT_VIDEO_SERVER:
            case ACCOUNT_MESSAGE_SERVER:
            case CONTENT_TRANSLATE:
            case CONTENT_SERVERPING:
                // TODO (brangr): future realize types
                break;
            default:
                break;
        }

        if (auto[ok, code] = result; !ok)
        {
            LogPrintf("Warning: SocialConsensus %d validate failed with result %d for transaction %s with block at height %d\n",
                *ptx->GetTypeInt(), (int) code, *ptx->GetHash(), height);

            return {false, code};
        }

        return {true, SocialConsensusResult_Success};
    }
}
