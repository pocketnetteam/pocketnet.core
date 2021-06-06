// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 Bitcoin developers
// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETTX_TRANSACTIONSERIALIZER_HPP
#define POCKETTX_TRANSACTIONSERIALIZER_HPP

#include "pocketdb/helpers/TransactionHelper.hpp"
#include "pocketdb/helpers/TypesHelper.hpp"

#include "pocketdb/models/base/Transaction.hpp"
#include "pocketdb/models/base/TransactionOutput.hpp"
#include "pocketdb/models/dto/Coinbase.hpp"
#include "pocketdb/models/dto/Coinstake.hpp"
#include "pocketdb/models/dto/Default.hpp"
#include "pocketdb/models/dto/User.hpp"
#include "pocketdb/models/dto/Post.hpp"
#include "pocketdb/models/dto/Blocking.hpp"
#include "pocketdb/models/dto/BlockingCancel.hpp"
#include "pocketdb/models/dto/Comment.hpp"
#include "pocketdb/models/dto/ScorePost.hpp"
#include "pocketdb/models/dto/Subscribe.hpp"
#include "pocketdb/models/dto/SubscribePrivate.hpp"
#include "pocketdb/models/dto/SubscribeCancel.hpp"
#include "pocketdb/models/dto/ScoreComment.hpp"
#include "pocketdb/models/dto/Complain.hpp"

#include "key_io.h"
#include "streams.h"

#include <utilstrencodings.h>

namespace PocketServices
{
    using namespace PocketTx;
    using namespace PocketHelpers;

    class TransactionSerializer
    {
    public:

        static PocketBlock DeserializeBlock(CDataStream& stream, CBlock& block)
        {
            LogPrint(BCLog::SYNC, "+++ DeserializeBlock: %s\n", block.GetHash().GetHex());

            // Prepare source data - old format (Json)
            UniValue pocketData(UniValue::VOBJ);
            if (!stream.empty())
            {
                // TODO (brangr) (v0.21.0): speed up protocol
                string src;
                stream >> src;
                pocketData.read(src);
                LogPrint(BCLog::STAT, src.c_str());
            }

            // Restore pocket transaction instance
            PocketBlock pocketBlock;
            for (const auto& tx : block.vtx)
            {
                auto txHash = tx->GetHash().GetHex();

                UniValue entry(UniValue::VOBJ);
                if (pocketData.exists(txHash))
                {
                    auto entrySrc = pocketData[txHash];

                    try
                    {
                        entry.read(entrySrc.get_str());
                    }
                    catch (std::exception& ex)
                    {
                        LogPrintf("Error deserialize transaction: %s: %s\n", txHash, ex.what());
                    }
                }

                auto ptx = BuildInstance(tx, entry);
                if (ptx) pocketBlock.push_back(ptx);
            }

            return pocketBlock;
        }

        static shared_ptr<Transaction> DeserializeTransaction(CDataStream& stream, const CTransactionRef& tx)
        {
            // TODO (brangr): implement
        }

        static shared_ptr<UniValue> SerializeBlock(PocketBlock block)
        {
            auto result = make_shared<UniValue>(UniValue(UniValue::VOBJ));
            for (const auto& transaction : block)
            {
                result->pushKV(*transaction->GetHash(), *SerializeTransaction(*transaction));
            }

            return result;
        }

        static shared_ptr<UniValue> SerializeTransaction(const Transaction& transaction)
        {
            auto result = make_shared<UniValue>(UniValue(UniValue::VOBJ));

            auto serializedTransaction = transaction.Serialize();
            auto base64Transaction = EncodeBase64(serializedTransaction->write());

            result->pushKV("t", ParseType(transaction));
            result->pushKV("d", base64Transaction);

            return result;
        }

    private:

        static shared_ptr<Transaction> BuildInstance(const CTransactionRef& tx, const UniValue& src)
        {
            auto txHash = tx->GetHash().GetHex();
            shared_ptr<Transaction> ptx = nullptr;
            PocketTxType txType = ParseType(tx);

            switch (txType)
            {
                case NOT_SUPPORTED:
                    return nullptr;
                case TX_COINBASE:
                    ptx = make_shared<Coinbase>(txHash, tx->nTime);
                    break;
                case TX_COINSTAKE:
                    ptx = make_shared<Coinstake>(txHash, tx->nTime);
                    break;
                case TX_DEFAULT:
                    ptx = make_shared<Default>(txHash, tx->nTime);
                    break;
                case ACCOUNT_USER:
                    ptx = make_shared<User>(txHash, tx->nTime);
                    break;
                case ACCOUNT_VIDEO_SERVER:
                    break;
                case ACCOUNT_MESSAGE_SERVER:
                    break;
                case CONTENT_POST:
                    ptx = make_shared<Post>(txHash, tx->nTime);
                    break;
                case CONTENT_VIDEO:
                    break;
                case CONTENT_TRANSLATE:
                    break;
                case CONTENT_SERVERPING:
                    break;
                case CONTENT_COMMENT:
                    ptx = make_shared<Comment>(txHash, tx->nTime);
                    break;
                case ACTION_SCORE_POST:
                    ptx = make_shared<ScorePost>(txHash, tx->nTime);
                    break;
                case ACTION_SCORE_COMMENT:
                    ptx = make_shared<ScoreComment>(txHash, tx->nTime);
                    break;
                case ACTION_SUBSCRIBE:
                    ptx = make_shared<Subscribe>(txHash, tx->nTime);
                    break;
                case ACTION_SUBSCRIBE_PRIVATE:
                    ptx = make_shared<SubscribePrivate>(txHash, tx->nTime);
                    break;
                case ACTION_SUBSCRIBE_CANCEL:
                    ptx = make_shared<SubscribeCancel>(txHash, tx->nTime);
                    break;
                case ACTION_BLOCKING:
                    ptx = make_shared<Blocking>(txHash, tx->nTime);
                    break;
                case ACTION_BLOCKING_CANCEL:
                    ptx = make_shared<BlockingCancel>(txHash, tx->nTime);
                    break;
                case ACTION_COMPLAIN:
                    ptx = make_shared<Complain>(txHash, tx->nTime);
                    break;
                default:
                    return nullptr;
            }

            // Build outputs & inputs
            if (ptx)
                BuildOutputs(tx, ptx);

            // Skip if outputs empty
            if (ptx->Outputs().empty())
                return nullptr;

            // Deserialize payload if exists
            if (ptx && src.exists("d"))
            {
                UniValue txDataSrc(UniValue::VOBJ);
                auto txDataBase64 = src["d"].get_str();
                auto txJson = DecodeBase64(txDataBase64);
                txDataSrc.read(txJson);

                ptx->Deserialize(txDataSrc);
                ptx->BuildPayload(txDataSrc);
                ptx->BuildHash(txDataSrc);
            }

            LogPrint(BCLog::SYNC, " ++ BuildInstance: %s (type: %d) (payload: %b)\n", txHash, txType, ptx->HasPayload());

            return ptx;
        }

        static void BuildOutputs(const CTransactionRef& tx, shared_ptr<Transaction> ptx)
        {
            // indexing Outputs
            for (int i = 0; i < tx->vout.size(); i++)
            {
                const CTxOut& txout = tx->vout[i];

                txnouttype type;
                std::vector<CTxDestination> vDest;
                int nRequired;
                if (ExtractDestinations(txout.scriptPubKey, type, vDest, nRequired))
                {
                    for (const auto& dest : vDest)
                    {
                        auto out = make_shared<TransactionOutput>();
                        out->SetTxHash(tx->GetHash().GetHex());
                        out->SetNumber(i);
                        out->SetAddressHash(EncodeDestination(dest));
                        out->SetValue(txout.nValue);

                        ptx->Outputs().push_back(out);
                    }
                }
            }
        }
    };

}

#endif // POCKETTX_TRANSACTIONSERIALIZER_HPP
