// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include <primitives/transaction.h>
#include "pocketdb/models/dto/content/ContentDelete.h"

namespace PocketTx
{
    ContentDelete::ContentDelete() : Content()
    {
        SetType(TxType::CONTENT_DELETE);
    }

    ContentDelete::ContentDelete(const CTransactionRef& tx) : Content(tx)
    {
        SetType(TxType::CONTENT_DELETE);
    }

    shared_ptr <UniValue> ContentDelete::Serialize() const
    {
        auto result = Transaction::Serialize();

        result->pushKV("address", *GetAddress());
        
        // txid - this is original content hash
        result->pushKV("txid", *GetRootTxHash());
        
        result->pushKV("settings", (m_payload && m_payload->GetString1()) ? *m_payload->GetString1() : "");
        
        return result;
    }

    void ContentDelete::Deserialize(const UniValue& src)
    {
        Transaction::Deserialize(src);
        if (auto[ok, val] = TryGetStr(src, "address"); ok) SetAddress(val);
        if (auto[ok, valTxId] = TryGetStr(src, "txid"); ok) SetRootTxHash(valTxId);
    }

    void ContentDelete::DeserializeRpc(const UniValue& src)
    {
        if (auto[ok, val] = TryGetStr(src, "txidEdit"); ok) SetRootTxHash(val);

        GeneratePayload();
        if (auto[ok, val] = TryGetStr(src, "s"); ok) m_payload->SetString1(val);
    }
    
    void ContentDelete::DeserializePayload(const UniValue& src)
    {
        if (auto[ok, val] = TryGetStr(src, "settings"); ok)
        {
            Transaction::DeserializePayload(src);
            m_payload->SetString1(val);
        }
    }

    string ContentDelete::BuildHash()
    {
        std::string data;

        data += *GetRootTxHash();
        data += m_payload && m_payload->GetString1() ? *m_payload->GetString1() : "";

        return Transaction::GenerateHash(data);
    }

} // namespace PocketTx

