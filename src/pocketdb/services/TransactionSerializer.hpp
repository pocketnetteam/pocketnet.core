// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETTX_TRANSACTIONSERIALIZER_HPP
#define POCKETTX_TRANSACTIONSERIALIZER_HPP

#include "pocketdb/helpers/TransactionHelper.hpp"

#include "pocketdb/models/base/Transaction.hpp"
#include "pocketdb/models/base/TransactionOutput.hpp"
#include "pocketdb/models/dto/Coinbase.hpp"
#include "pocketdb/models/dto/Coinstake.hpp"
#include "pocketdb/models/dto/Default.hpp"
#include "pocketdb/models/dto/User.hpp"
#include "pocketdb/models/dto/Post.hpp"
#include "pocketdb/models/dto/Video.hpp"
#include "pocketdb/models/dto/Blocking.hpp"
#include "pocketdb/models/dto/BlockingCancel.hpp"
#include "pocketdb/models/dto/Comment.hpp"
#include "pocketdb/models/dto/ScoreContent.hpp"
#include "pocketdb/models/dto/ScoreComment.hpp"
#include "pocketdb/models/dto/Subscribe.hpp"
#include "pocketdb/models/dto/SubscribePrivate.hpp"
#include "pocketdb/models/dto/SubscribeCancel.hpp"
#include "pocketdb/models/dto/Complain.hpp"

#include "key_io.h"
#include "streams.h"
#include "logging.h"

#include <utilstrencodings.h>

namespace PocketServices
{
    using namespace PocketTx;
    using namespace PocketHelpers;

    class TransactionSerializer
    {
    public:

        static tuple<bool, PocketBlock> DeserializeBlock(CDataStream& stream, CBlock& block)
        {
            // Get Serialized data from stream
            auto pocketData = parseStream(stream);

            return deserializeBlock(pocketData, block);
        }

        static tuple<bool, PocketBlock> DeserializeBlock(CBlock& block)
        {
            UniValue pocketData(UniValue::VOBJ);
            return deserializeBlock(pocketData, block);
        }

        static tuple<bool, PTransactionRef> DeserializeTransaction(const CTransactionRef& tx, const UniValue& pocketData)
        {
            auto ptx = buildInstanceRpc(tx, pocketData);
            return {ptx != nullptr, ptx};
        }

        static tuple<bool, PTransactionRef> DeserializeTransaction(const CTransactionRef& tx, CDataStream& stream)
        {
            // Get Serialized data from stream
            auto pocketData = parseStream(stream);
            
            // Build transaction instance
            return deserializeTransaction(pocketData, tx);
        }

        static shared_ptr<UniValue> SerializeBlock(const PocketBlock& block)
        {
            auto result = make_shared<UniValue>(UniValue(UniValue::VOBJ));
            for (const auto& transaction : block)
            {
                result->pushKV(*transaction->GetHash(), (*SerializeTransaction(*transaction)).write());
            }

            return result;
        }

        static shared_ptr<UniValue> SerializeTransaction(const Transaction& transaction)
        {
            auto result = make_shared<UniValue>(UniValue(UniValue::VOBJ));

            auto serializedTransaction = transaction.Serialize();
            auto base64Transaction = EncodeBase64(serializedTransaction->write());

            result->pushKV("t", ConvertToReindexerTable(transaction));
            result->pushKV("d", base64Transaction);

            return result;
        }

    private:

        static shared_ptr<Transaction> buildInstance(const CTransactionRef& tx, const UniValue& src)
        {
            auto txHash = tx->GetHash().GetHex();

            PocketTxType txType;
            if (!PocketHelpers::IsPocketSupportedTransaction(tx, txType))
                return nullptr;

            shared_ptr<Transaction> ptx = PocketHelpers::CreateInstance(txType, txHash, tx->nTime);
            if (!ptx)
                return nullptr;

            // Build outputs & inputs
            if (!buildOutputs(tx, ptx))
                return nullptr;

            // Deserialize payload if exists
            if (src.exists("d"))
            {
                UniValue txDataSrc(UniValue::VOBJ);
                auto txDataBase64 = src["d"].get_str();
                auto txJson = DecodeBase64(txDataBase64);
                txDataSrc.read(txJson);

                if (src.exists("t") && src["t"].get_str() == "Mempool" && txDataSrc.exists("data"))
                {
                    auto txMempoolDataBase64 = txDataSrc["data"].get_str();
                    auto txMempoolJson = DecodeBase64(txMempoolDataBase64);
                    txDataSrc.read(txMempoolJson);
                }

                ptx->Deserialize(txDataSrc);
                ptx->DeserializePayload(txDataSrc);
            }

            return ptx;
        }

        static shared_ptr<Transaction> buildInstanceRpc(const CTransactionRef& tx, const UniValue& src)
        {
            auto txHash = tx->GetHash().GetHex();

            PocketTxType txType;
            if (!PocketHelpers::IsPocketSupportedTransaction(tx, txType))
                return nullptr;

            shared_ptr<Transaction> ptx = PocketHelpers::CreateInstance(txType, txHash, tx->nTime);
            if (!ptx)
                return nullptr;

            // Build outputs & inputs
            if (!buildOutputs(tx, ptx))
                return nullptr;
          
            ptx->DeserializeRpc(src);
            return ptx;
        }

        static bool buildOutputs(const CTransactionRef& tx, shared_ptr<Transaction>& ptx)
        {
            // indexing Outputs
            for (size_t i = 0; i < tx->vout.size(); i++)
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
                        out->SetNumber((int)i);
                        out->SetAddressHash(EncodeDestination(dest));
                        out->SetValue(txout.nValue);

                        ptx->Outputs().push_back(out);
                    }
                }
            }

            return !ptx->Outputs().empty();
        }

        static UniValue parseStream(CDataStream& stream)
        {
            // Prepare source data - old format (Json)
            UniValue pocketData(UniValue::VOBJ);
            if (!stream.empty())
            {
                // TODO (brangr) (v0.21.0): speed up protocol
                string src;
                stream >> src;
                pocketData.read(src);
            }

            return pocketData;
        }

        static tuple<bool, PocketBlock> deserializeBlock(UniValue& pocketData, CBlock& block)
        {
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

                if (auto[ok, ptx] = deserializeTransaction(entry, tx); ok && ptx)
                    pocketBlock.push_back(ptx);
            }

            // TODO (brangr): check deserialize success
            //bool resultCheck = pocketBlock.size() == (block.vtx.size() - 1);
            return {true, pocketBlock};
        }

        static tuple<bool, shared_ptr<Transaction>> deserializeTransaction(UniValue& pocketData, const CTransactionRef& tx)
        {
            // Restore pocket transaction instance
            auto ptx = buildInstance(tx, pocketData);
            return {ptx != nullptr, ptx};
        }

    };

}

#endif // POCKETTX_TRANSACTIONSERIALIZER_HPP
