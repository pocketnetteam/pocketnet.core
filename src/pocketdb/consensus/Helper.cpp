// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include "pocketdb/consensus/Helper.h"

namespace PocketConsensus
{
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

        return {true, ConsensusResult_Success};
    }

    tuple<bool, SocialConsensusResult> SocialConsensusHelper::Validate(const CTransactionRef& tx, const PTransactionRef& ptx, int height)
    {
        // Not double validate for already in DB
        if (TransRepoInst.Exists(*ptx->GetHash()))
            return {true, ConsensusResult_Success};

        if (auto[ok, result] = validate(tx, ptx, nullptr, height); !ok)
        {
            LogPrint(BCLog::CONSENSUS, "Warning: SocialConsensus type:%d validate failed with result:%d for tx:%s at height:%d\n",
                (int) *ptx->GetType(), (int)result, *ptx->GetHash(), height);

            return {false, result};
        }

        return {true, ConsensusResult_Success};
    }

    tuple<bool, SocialConsensusResult> SocialConsensusHelper::Validate(const CTransactionRef& tx, const PTransactionRef& ptx, PocketBlockRef& pBlock, int height)
    {
        if (auto[ok, result] = validate(tx, ptx, pBlock, height); !ok)
        {
            LogPrint(BCLog::CONSENSUS, "Warning: SocialConsensus type:%d validate tx:%s failed with result:%d for block construction at height:%d\n",
                (int)*ptx->GetType(), *ptx->GetHash(), (int)result, height);

            return {false, result};
        }

        return {true, ConsensusResult_Success};
    }

    // Проверяет блок транзакций без привязки к цепи
    tuple<bool, SocialConsensusResult> SocialConsensusHelper::Check(const CBlock& block, const PocketBlockRef& pBlock, int height)
    {
        if (!pBlock)
        {
            LogPrint(BCLog::CONSENSUS, "Warning: SocialConsensus check failed with result:%d for blk:%s at height:%d\n",
                (int)ConsensusResult_PocketDataNotFound, block.GetHash().GetHex(), height);

            return {false, ConsensusResult_PocketDataNotFound};
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
                    (int)txType, (int)ConsensusResult_PocketDataNotFound, tx->GetHash().GetHex(), block.GetHash().GetHex(), height);

                return {false, ConsensusResult_PocketDataNotFound};
            }

            // Check founded payload
            if (auto[ok, result] = check(tx, *it, height); !ok)
            {
                LogPrint(BCLog::CONSENSUS, "Warning: SocialConsensus check type:%d failed with result:%d for tx:%s in blk:%s at height:%d\n",
                    (int)txType, (int)result, tx->GetHash().GetHex(), block.GetHash().GetHex(), height);

                return {false, result};
            }
        }

        return {true, ConsensusResult_Success};
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

        return {true, ConsensusResult_Success};
    }

    // -----------------------------------------------------------------

    tuple<bool, SocialConsensusResult> SocialConsensusHelper::check(const CTransactionRef& tx, const PTransactionRef& ptx, int height)
    {
        if (!isConsensusable(*ptx->GetType()))
            return {true, ConsensusResult_Success};

        // Check transactions with consensus logic
        switch (*ptx->GetType())
        {
            case ACCOUNT_SETTING:
                return ConsensusFactoryInst_AccountSetting.Instance(height)->Check(tx, static_pointer_cast<AccountSetting>(ptx));
            case ACCOUNT_DELETE:
                return ConsensusFactoryInst_AccountDelete.Instance(height)->Check(tx, static_pointer_cast<AccountDelete>(ptx));
            case ACCOUNT_USER:
                return ConsensusFactoryInst_AccountUser.Instance(height)->Check(tx, static_pointer_cast<User>(ptx));
            case CONTENT_POST:
                return ConsensusFactoryInst_Post.Instance(height)->Check(tx, static_pointer_cast<Post>(ptx));
            case CONTENT_VIDEO:
                return ConsensusFactoryInst_Video.Instance(height)->Check(tx, static_pointer_cast<Video>(ptx));
            case CONTENT_ARTICLE:
                return ConsensusFactoryInst_Article.Instance(height)->Check(tx, static_pointer_cast<Article>(ptx));
            case CONTENT_STREAM:
                return ConsensusFactoryInst_Stream.Instance(height)->Check(tx, static_pointer_cast<Stream>(ptx));
            case CONTENT_AUDIO:
                return ConsensusFactoryInst_Audio.Instance(height)->Check(tx, static_pointer_cast<Audio>(ptx));
            case CONTENT_COLLECTION:
                return ConsensusFactoryInst_Collection.Instance(height)->Check(tx, static_pointer_cast<Collection>(ptx));
            case CONTENT_APP:
                return ConsensusFactoryInst_App.Instance(height)->Check(tx, static_pointer_cast<App>(ptx));
            case CONTENT_COMMENT:
                return ConsensusFactoryInst_Comment.Instance(height)->Check(tx, static_pointer_cast<Comment>(ptx));
            case CONTENT_COMMENT_EDIT:
                return ConsensusFactoryInst_CommentEdit.Instance(height)->Check(tx, static_pointer_cast<CommentEdit>(ptx));
            case CONTENT_COMMENT_DELETE:
                return ConsensusFactoryInst_CommentDelete.Instance(height)->Check(tx, static_pointer_cast<CommentDelete>(ptx));
            case CONTENT_DELETE:
                return ConsensusFactoryInst_ContentDelete.Instance(height)->Check(tx, static_pointer_cast<ContentDelete>(ptx));
            case BOOST_CONTENT:
                return ConsensusFactoryInst_BoostContent.Instance(height)->Check(tx, static_pointer_cast<BoostContent>(ptx));
            case ACTION_SCORE_CONTENT:
                return ConsensusFactoryInst_ScoreContent.Instance(height)->Check(tx, static_pointer_cast<ScoreContent>(ptx));
            case ACTION_SCORE_COMMENT:
                return ConsensusFactoryInst_ScoreComment.Instance(height)->Check(tx, static_pointer_cast<ScoreComment>(ptx));
            case ACTION_SUBSCRIBE:
                return ConsensusFactoryInst_Subscribe.Instance(height)->Check(tx, static_pointer_cast<Subscribe>(ptx));
            case ACTION_SUBSCRIBE_PRIVATE:
                return ConsensusFactoryInst_SubscribePrivate.Instance(height)->Check(tx, static_pointer_cast<SubscribePrivate>(ptx));
            case ACTION_SUBSCRIBE_CANCEL:
                return ConsensusFactoryInst_SubscribeCancel.Instance(height)->Check(tx, static_pointer_cast<SubscribeCancel>(ptx));
            case ACTION_BLOCKING:
                return ConsensusFactoryInst_Blocking.Instance(height)->Check(tx, static_pointer_cast<Blocking>(ptx));
            case ACTION_BLOCKING_CANCEL:
                return ConsensusFactoryInst_BlockingCancel.Instance(height)->Check(tx, static_pointer_cast<BlockingCancel>(ptx));
            case ACTION_COMPLAIN:
                return ConsensusFactoryInst_Complain.Instance(height)->Check(tx, static_pointer_cast<Complain>(ptx));

            // Moderation
            case MODERATION_FLAG:
                return ConsensusFactoryInst_ModerationFlag.Instance(height)->Check(tx, static_pointer_cast<ModerationFlag>(ptx));
            case MODERATION_VOTE:
                return ConsensusFactoryInst_ModerationVote.Instance(height)->Check(tx, static_pointer_cast<ModerationVote>(ptx));

            // Barteron
            case BARTERON_OFFER:
                return ConsensusFactoryInst_BarteronOffer.Instance(height)->Check(tx, static_pointer_cast<BarteronOffer>(ptx));
            case BARTERON_ACCOUNT:
                return ConsensusFactoryInst_BarteronAccount.Instance(height)->Check(tx, static_pointer_cast<BarteronAccount>(ptx));

            default:
                return { false, ConsensusResult_NotImplemeted };
        }
    }

    tuple<bool, SocialConsensusResult> SocialConsensusHelper::validate(const CTransactionRef& tx, const PTransactionRef& ptx, const PocketBlockRef& pBlock, int height)
    {
        if (!isConsensusable(*ptx->GetType()))
            return {true, ConsensusResult_Success};

        // Validate transactions with consensus logic
        switch (*ptx->GetType())
        {
            case ACCOUNT_SETTING:
                return ConsensusFactoryInst_AccountSetting.Instance(height)->Validate(tx, static_pointer_cast<AccountSetting>(ptx), pBlock);
            case ACCOUNT_DELETE:
                return ConsensusFactoryInst_AccountDelete.Instance(height)->Validate(tx, static_pointer_cast<AccountDelete>(ptx), pBlock);
            case ACCOUNT_USER:
                return ConsensusFactoryInst_AccountUser.Instance(height)->Validate(tx, static_pointer_cast<User>(ptx), pBlock);
            case CONTENT_POST:
                return ConsensusFactoryInst_Post.Instance(height)->Validate(tx, static_pointer_cast<Post>(ptx), pBlock);
            case CONTENT_VIDEO:
                return ConsensusFactoryInst_Video.Instance(height)->Validate(tx, static_pointer_cast<Video>(ptx), pBlock);
            case CONTENT_ARTICLE:
                return ConsensusFactoryInst_Article.Instance(height)->Validate(tx, static_pointer_cast<Article>(ptx), pBlock);
            case CONTENT_STREAM:
                return ConsensusFactoryInst_Stream.Instance(height)->Validate(tx, static_pointer_cast<Stream>(ptx), pBlock);
            case CONTENT_AUDIO:
                return ConsensusFactoryInst_Audio.Instance(height)->Validate(tx, static_pointer_cast<Audio>(ptx), pBlock);
            case CONTENT_COLLECTION:
                return ConsensusFactoryInst_Collection.Instance(height)->Validate(tx, static_pointer_cast<Collection>(ptx), pBlock);
            case CONTENT_APP:
                return ConsensusFactoryInst_App.Instance(height)->Validate(tx, static_pointer_cast<App>(ptx), pBlock);
            case CONTENT_COMMENT:
                return ConsensusFactoryInst_Comment.Instance(height)->Validate(tx, static_pointer_cast<Comment>(ptx), pBlock);
            case CONTENT_COMMENT_EDIT:
                return ConsensusFactoryInst_CommentEdit.Instance(height)->Validate(tx, static_pointer_cast<CommentEdit>(ptx), pBlock);
            case CONTENT_COMMENT_DELETE:
                return ConsensusFactoryInst_CommentDelete.Instance(height)->Validate(tx, static_pointer_cast<CommentDelete>(ptx), pBlock);
            case CONTENT_DELETE:
                return ConsensusFactoryInst_ContentDelete.Instance(height)->Validate(tx, static_pointer_cast<ContentDelete>(ptx), pBlock);
            case BOOST_CONTENT:
                return ConsensusFactoryInst_BoostContent.Instance(height)->Validate(tx, static_pointer_cast<BoostContent>(ptx), pBlock);
            case ACTION_SCORE_CONTENT:
                return ConsensusFactoryInst_ScoreContent.Instance(height)->Validate(tx, static_pointer_cast<ScoreContent>(ptx), pBlock);
            case ACTION_SCORE_COMMENT:
                return ConsensusFactoryInst_ScoreComment.Instance(height)->Validate(tx, static_pointer_cast<ScoreComment>(ptx), pBlock);
            case ACTION_SUBSCRIBE:
                return ConsensusFactoryInst_Subscribe.Instance(height)->Validate(tx, static_pointer_cast<Subscribe>(ptx), pBlock);
            case ACTION_SUBSCRIBE_PRIVATE:
                return ConsensusFactoryInst_SubscribePrivate.Instance(height)->Validate(tx, static_pointer_cast<SubscribePrivate>(ptx), pBlock);
            case ACTION_SUBSCRIBE_CANCEL:
                return ConsensusFactoryInst_SubscribeCancel.Instance(height)->Validate(tx, static_pointer_cast<SubscribeCancel>(ptx), pBlock);
            case ACTION_BLOCKING:
                return ConsensusFactoryInst_Blocking.Instance(height)->Validate(tx, static_pointer_cast<Blocking>(ptx), pBlock);
            case ACTION_BLOCKING_CANCEL:
                return ConsensusFactoryInst_BlockingCancel.Instance(height)->Validate(tx, static_pointer_cast<BlockingCancel>(ptx), pBlock);
            case ACTION_COMPLAIN:
                return ConsensusFactoryInst_Complain.Instance(height)->Validate(tx, static_pointer_cast<Complain>(ptx), pBlock);

            // Moderation
            case MODERATION_FLAG:
                return ConsensusFactoryInst_ModerationFlag.Instance(height)->Validate(tx, static_pointer_cast<ModerationFlag>(ptx), pBlock);
            case MODERATION_VOTE:
                return ConsensusFactoryInst_ModerationVote.Instance(height)->Validate(tx, static_pointer_cast<ModerationVote>(ptx), pBlock);

            // Barteron
            case BARTERON_OFFER:
                return ConsensusFactoryInst_BarteronOffer.Instance(height)->Validate(tx, static_pointer_cast<BarteronOffer>(ptx), pBlock);
            case BARTERON_ACCOUNT:
                return ConsensusFactoryInst_BarteronAccount.Instance(height)->Validate(tx, static_pointer_cast<BarteronAccount>(ptx), pBlock);

            default:
                return { false, ConsensusResult_NotImplemeted };
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
