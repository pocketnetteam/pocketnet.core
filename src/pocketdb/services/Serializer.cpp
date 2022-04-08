// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include "pocketdb/services/Serializer.h"
#include "script/standard.h"

namespace PocketServices
{
    tuple<bool, PocketBlock> Serializer::DeserializeBlock(const CBlock& block, CDataStream& stream)
    {
        // Get Serialized data from stream
        auto pocketData = parseStream(stream);
        return deserializeBlock(block, pocketData);
    }
    tuple<bool, PocketBlock> Serializer::DeserializeBlock(const CBlock& block)
    {
        UniValue fakeData(UniValue::VOBJ);
        return deserializeBlock(block, fakeData);
    }

    tuple<bool, PTransactionRef> Serializer::DeserializeTransactionRpc(const CTransactionRef& tx, const UniValue& pocketData)
    {
        auto ptx = buildInstanceRpc(tx, pocketData);
        return {ptx != nullptr, ptx};
    }

    tuple<bool, PTransactionRef> Serializer::DeserializeTransaction(const CTransactionRef& tx, CDataStream& stream)
    {
        auto pocketData = parseStream(stream);
        return deserializeTransaction(tx, pocketData);
    }

    tuple<bool, PTransactionRef> Serializer::DeserializeTransaction(const CTransactionRef& tx)
    {
        UniValue fakeData(UniValue::VOBJ);
        return deserializeTransaction(tx, fakeData);
    }

    // Serialize protocol compatible with Reindexer
    // It makes sense to serialize only Pocket transactions that contain a payload.
    shared_ptr<UniValue> Serializer::SerializeBlock(const PocketBlock& block)
    {
        auto result = make_shared<UniValue>(UniValue(UniValue::VOBJ));
        for (const auto& transaction : block)
        {
            auto dataPtr = SerializeTransaction(*transaction);
            if (!dataPtr)
                continue;

            result->pushKV(*transaction->GetHash(), dataPtr->write());
        }

        return result;
    }

    // Serialize protocol compatible with Reindexer
    // It makes sense to serialize only Pocket transactions that contain a payload.
    shared_ptr<UniValue> Serializer::SerializeTransaction(const Transaction& transaction)
    {
        if (!PocketHelpers::TransactionHelper::IsPocketTransaction(*transaction.GetType()))
            return nullptr;

        auto result = make_shared<UniValue>(UniValue(UniValue::VOBJ));

        auto serializedTransaction = transaction.Serialize();
        auto base64Transaction = EncodeBase64(serializedTransaction->write());

        result->pushKV("t", PocketHelpers::TransactionHelper::ConvertToReindexerTable(transaction));
        result->pushKV("d", base64Transaction);

        return result;
    }

    shared_ptr<Transaction> Serializer::buildInstance(const CTransactionRef& tx, const UniValue& src)
    {
        TxType txType = PocketHelpers::TransactionHelper::ParseType(tx);
        shared_ptr<Transaction> ptx = PocketHelpers::TransactionHelper::CreateInstance(txType, tx);
        if (!ptx)
            return nullptr;

        // Build inputs
        if (!buildInputs(tx, ptx))
            return nullptr;

        // Build outputs
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

    shared_ptr<Transaction> Serializer::buildInstanceRpc(const CTransactionRef& tx, const UniValue& src)
    {
        TxType txType = PocketHelpers::TransactionHelper::ParseType(tx);
        shared_ptr <Transaction> ptx = PocketHelpers::TransactionHelper::CreateInstance(txType, tx);
        if (!ptx)
            return nullptr;

        // Build inputs
        if (!buildInputs(tx, ptx))
            return nullptr;

        // Build outputs
        if (!buildOutputs(tx, ptx))
            return nullptr;

        ptx->DeserializeRpc(src);
        return ptx;
    }

    bool Serializer::buildInputs(const CTransactionRef& tx, shared_ptr<Transaction>& ptx)
    {
        string spentTxHash = tx->GetHash().GetHex();

        for (size_t i = 0; i < tx->vin.size(); i++)
        {
            const CTxIn& txin = tx->vin[i];

            auto inp = make_shared<TransactionInput>();
            inp->SetSpentTxHash(spentTxHash);
            inp->SetTxHash(txin.prevout.hash.GetHex());
            inp->SetNumber(txin.prevout.n);
            
            ptx->Inputs().push_back(inp);
        }

        return !ptx->Inputs().empty();
    }

    bool Serializer::buildOutputs(const CTransactionRef& tx, shared_ptr<Transaction>& ptx)
    {
        string txHash = tx->GetHash().GetHex();

        for (size_t i = 0; i < tx->vout.size(); i++)
        {
            const CTxOut& txout = tx->vout[i];

            auto out = make_shared<TransactionOutput>();
            out->SetTxHash(txHash);
            out->SetNumber((int) i);
            out->SetValue(txout.nValue);
            out->SetScriptPubKey(HexStr(txout.scriptPubKey));

            TxoutType type;
            std::vector <CTxDestination> vDest;
            int nRequired;
            if (ExtractDestinations(txout.scriptPubKey, type, vDest, nRequired))
            {
                for (const auto& dest : vDest)
                    out->SetAddressHash(EncodeDestination(dest));
            }
            else
            {
                out->SetAddressHash("");
            }

            ptx->Outputs().push_back(out);
        }

        return !ptx->Outputs().empty();
    }

    UniValue Serializer::parseStream(CDataStream& stream)
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


    tuple<bool, PocketBlock> Serializer::deserializeBlock(const CBlock& block, UniValue& pocketData)
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

            if (auto[ok, ptx] = deserializeTransaction(tx, entry); ok && ptx)
                pocketBlock.push_back(ptx);
        }

        return { true, pocketBlock };
    }

    tuple<bool, shared_ptr<Transaction>> Serializer::deserializeTransaction(const CTransactionRef& tx, UniValue& pocketData)
    {
        auto ptx = buildInstance(tx, pocketData);
        return { ptx != nullptr, ptx };
    }
}