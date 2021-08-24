// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 Bitcoin developers
// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETCONSENSUS_SOCIAL_HPP
#define POCKETCONSENSUS_SOCIAL_HPP

#include "pocketdb/helpers/TransactionHelper.hpp"
#include "pocketdb/models/base/Transaction.hpp"
#include "pocketdb/ReputationConsensus.h"
#include "pocketdb/SocialConsensus.h"

namespace PocketConsensus
{
    using namespace std;
    using namespace PocketTx;
    using namespace PocketDb;

    // This helper need for hide selector Consensus rules
    class SocialConsensusHelper
    {
    public:

        static bool Validate(const PocketBlockRef& block, int height)
        {
            for (const auto& tx : *block)
            {
                if (auto[ok, result] = validate(tx, block, height); !ok)
                    return false;
            }

            return true;
        }

        static tuple<bool, SocialConsensusResult> Validate(const PTransactionRef& tx, int height)
        {
            return validate(tx, nullptr, height);
        }

        static tuple<bool, SocialConsensusResult> Validate(const PTransactionRef& tx, PocketBlockRef& block, int height)
        {
            return validate(tx, block, height);
        }

        // Проверяет блок транзакций без привязки к цепи
        static bool Check(const CBlock& block, const PocketBlockRef& pBlock)
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
        static tuple<bool, SocialConsensusResult> Check(const CTransactionRef& tx, const PTransactionRef& ptx)
        {
            return check(tx, ptx);
        }

    protected:

        static tuple<bool, SocialConsensusResult> validate(const PTransactionRef& tx, const PocketBlockRef& block, int height)
        {
            auto txType = *tx->GetType();

            if (!isConsensusable(txType))
                return {true, SocialConsensusResult_Success};

            auto consensus = getConsensus(txType, height);
            if (!consensus)
            {
                LogPrintf("Warning: SocialConsensus type %d not found for transaction %s\n",
                    (int) txType, *tx->GetHash());

                return {false, SocialConsensusResult_Unknown};
            }

            if (auto[ok, result] = consensus->Validate(tx, block); !ok)
            {
                LogPrintf("Warning: SocialConsensus %d validate failed with result %d "
                            "for transaction %s with block at height %d\n",
                    (int) txType, (int) result, *tx->GetHash(), height);

                return {false, result};
            }

            return {true, SocialConsensusResult_Success};
        }

        static tuple<bool, SocialConsensusResult> check(const CTransactionRef& tx, const PTransactionRef& ptx)
        {
            auto txType = *ptx->GetType();

            if (!isConsensusable(txType))
                return {true, SocialConsensusResult_Success};

            auto consensus = getConsensus(txType);
            if (!consensus)
            {
                LogPrintf("Warning: SocialConsensus type %d not found for transaction %s\n",
                    (int) txType, *ptx->GetHash());

                return {false, SocialConsensusResult_Unknown};
            }

            if (auto[ok, result] = consensus->Check(tx, ptx); !ok)
            {
                LogPrintf("Warning: SocialConsensus %d check failed with result %d for transaction %s\n",
                    (int) txType, (int) result, *ptx->GetHash());

                return {false, result};
            }

            return {true, SocialConsensusResult_Success};
        }

        static bool isConsensusable(PocketTxType txType)
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

        static shared_ptr<SocialConsensus<PTransactionRef>> getConsensus(PocketTxType txType, int height = 0)
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
                case ACCOUNT_MESSAGE_SERVER:
                case CONTENT_TRANSLATE:
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
