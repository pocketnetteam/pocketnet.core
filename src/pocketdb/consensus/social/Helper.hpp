// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 Bitcoin developers
// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETCONSENSUS_SOCIAL_HPP
#define POCKETCONSENSUS_SOCIAL_HPP

#include "pocketdb/helpers/TransactionHelper.hpp"
#include "pocketdb/models/base/Transaction.hpp"
#include "pocketdb/consensus.h"
#include "pocketdb/consensus/social/Base.hpp"
#include "pocketdb/consensus/social/User.hpp"
#include "pocketdb/consensus/social/Video.hpp"
#include "pocketdb/consensus/social/Blocking.hpp"
#include "pocketdb/consensus/social/BlockingCancel.hpp"
#include "pocketdb/consensus/social/Comment.hpp"
#include "pocketdb/consensus/social/CommentEdit.hpp"
#include "pocketdb/consensus/social/CommentDelete.hpp"
#include "pocketdb/consensus/social/Complain.hpp"
#include "pocketdb/consensus/social/Post.hpp"
#include "pocketdb/consensus/social/ScoreComment.hpp"
#include "pocketdb/consensus/social/ScoreContent.hpp"
#include "pocketdb/consensus/social/Subscribe.hpp"
#include "pocketdb/consensus/social/SubscribeCancel.hpp"
#include "pocketdb/consensus/social/SubscribePrivate.hpp"

namespace PocketConsensus
{
    using namespace std;
    using namespace PocketTx;
    using namespace PocketDb;

    // This helper need for hide selector Consensus rules
    class SocialConsensusHelper
    {
    public:

        static bool Validate(const PocketBlock& block, int height)
        {
            for (const auto& tx : block)
            {
                if (auto[ok, result] = Validate(tx, block, true, height); !ok)
                    return false;
            }

            return true;
        }

        static tuple<bool, SocialConsensusResult> Validate(const shared_ptr<Transaction>& tx, int height)
        {
            PocketBlock block;
            return Validate(tx, block, false, height);
        }

        static tuple<bool, SocialConsensusResult> Validate(const shared_ptr<Transaction>& tx, PocketBlock& block, int height)
        {
            return Validate(tx, block, true, height);
        }

        // Проверяет блок транзакций без привязки к цепи
        static bool Check(const CBlock& block, const PocketBlock& pBlock)
        {
            auto coinstakeBlock = find_if(block.vtx.begin(), block.vtx.end(), [&](CTransactionRef const& tx)
            {
                return tx->IsCoinStake();
            }) != block.vtx.end();

            for (const auto& tx : block.vtx)
            {
                // NOT_SUPPORTED transactions not checked
                auto txType = PocketHelpers::ParseType(tx);
                if (txType == PocketTxType::NOT_SUPPORTED)
                    continue;
                if (coinstakeBlock && txType == PocketTxType::TX_COINBASE)
                    continue;

                // Maybe payload not exists?
                auto txHash = tx->GetHash().GetHex();
                auto it = find_if(pBlock.begin(), pBlock.end(),
                    [&](PTransactionRef const& ptx) { return *ptx == txHash; });
                if (it == pBlock.end())
                    return false;

                // Check founded payload
                if (auto[ok, result] = ValidateCheck(*it); !ok)
                    return false;
            }

            return true;
        }

        // Проверяет транзакцию без привязки к цепи
        static tuple<bool, SocialConsensusResult> Check(const shared_ptr<Transaction>& tx)
        {
            return ValidateCheck(tx);
        }

    protected:

        static tuple<bool, SocialConsensusResult> Validate(const shared_ptr<Transaction>& tx, const PocketBlock& block, bool inBlock, int height)
        {
            auto txType = *tx->GetType();

            if (!IsConsensusable(txType))
                return {true, SocialConsensusResult_Success};

            auto consensus = GetConsensus(txType, height);
            if (!consensus)
            {
                LogPrintf("Warning: SocialConsensus type %d not found for transaction %s\n",
                    (int) txType, *tx->GetHash());

                return {false, SocialConsensusResult_Unknown};
            }

            if (inBlock)
            {
                if (auto[ok, result] = consensus->Validate(tx, block); !ok)
                {
                    LogPrintf("Warning: SocialConsensus %d validate failed with result %d "
                              "for transaction %s with block at height %d\n",
                        (int) txType, (int) result, *tx->GetHash(), height);

                    return {false, result};
                }
            }
            else
            {
                if (auto[ok, result] = consensus->Validate(tx); !ok)
                {
                    LogPrintf("Warning: SocialConsensus %d validate failed with result %d "
                              "for transaction %s without block at height %d\n",
                        (int) txType, (int) result, *tx->GetHash(), height);

                    return {false, result};
                }
            }

            return {true, SocialConsensusResult_Success};
        }

        static tuple<bool, SocialConsensusResult> ValidateCheck(const shared_ptr<Transaction>& tx)
        {
            auto txType = *tx->GetType();

            if (!IsConsensusable(txType))
                return {true, SocialConsensusResult_Success};

            auto consensus = GetConsensus(txType);
            if (!consensus)
            {
                LogPrintf("Warning: SocialConsensus type %d not found for transaction %s\n",
                    (int) txType, *tx->GetHash());

                return {false, SocialConsensusResult_Unknown};
            }

            if (auto[ok, result] = consensus->Check(tx); !ok)
            {
                LogPrintf("Warning: SocialConsensus %d check failed with result %d for transaction %s\n",
                    (int) txType, (int) result, *tx->GetHash());

                return {false, result};
            }

            return {true, SocialConsensusResult_Success};
        }

        static bool IsConsensusable(PocketTxType txType)
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

        static shared_ptr<SocialConsensus> GetConsensus(PocketTxType txType, int height = 0)
        {
            switch (txType)
            {
                case ACCOUNT_USER:
                    return PocketConsensus::UserConsensusFactoryInst.Instance(height);
                case CONTENT_POST:
                    return PocketConsensus::PostConsensusFactoryInst.Instance(height);
                case CONTENT_VIDEO:
                    return PocketConsensus::VideoConsensusFactoryInst.Instance(height);
                case CONTENT_COMMENT:
                    return PocketConsensus::CommentConsensusFactoryInst.Instance(height);
                case CONTENT_COMMENT_EDIT:
                    return PocketConsensus::CommentEditConsensusFactoryInst.Instance(height);
                case CONTENT_COMMENT_DELETE:
                    return PocketConsensus::CommentDeleteConsensusFactoryInst.Instance(height);
                case ACTION_SCORE_CONTENT:
                    return PocketConsensus::ScoreContentConsensusFactoryInst.Instance(height);
                case ACTION_SCORE_COMMENT:
                    return PocketConsensus::ScoreCommentConsensusFactoryInst.Instance(height);
                case ACTION_SUBSCRIBE:
                    return PocketConsensus::SubscribeConsensusFactoryInst.Instance(height);
                case ACTION_SUBSCRIBE_PRIVATE:
                    return PocketConsensus::SubscribePrivateConsensusFactoryInst.Instance(height);
                case ACTION_SUBSCRIBE_CANCEL:
                    return PocketConsensus::SubscribeCancelConsensusFactoryInst.Instance(height);
                case ACTION_BLOCKING:
                    return PocketConsensus::BlockingConsensusFactoryInst.Instance(height);
                case ACTION_BLOCKING_CANCEL:
                    return PocketConsensus::BlockingCancelConsensusFactoryInst.Instance(height);
                case ACTION_COMPLAIN:
                    return PocketConsensus::ComplainConsensusFactoryInst.Instance(height);
                case ACCOUNT_VIDEO_SERVER:
                    // TODO (brangr): future realize types
                    break;
                case ACCOUNT_MESSAGE_SERVER:
                    // TODO (brangr): future realize types
                    break;
                case CONTENT_TRANSLATE:
                    // TODO (brangr): future realize types
                    break;
                case CONTENT_SERVERPING:
                    // TODO (brangr): future realize types
                    break;
                default:
                    break;
            }

            return nullptr;
        }

    };
}

#endif // POCKETCONSENSUS_SOCIAL_HPP
