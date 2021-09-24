// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include <primitives/transaction.h>
#include "pocketdb/models/dto/ContentDelete.h"

namespace PocketTx
{
    ContentDelete::ContentDelete() : Transaction()
    {
        SetType(PocketTxType::CONTENT_DELETE);
    }

    ContentDelete::ContentDelete(const CTransactionRef& tx) : Transaction(tx)
    {
        SetType(PocketTxType::CONTENT_DELETE);
    }

    shared_ptr <string> ContentDelete::GetAddress() const { return m_string1; }
    void ContentDelete::SetAddress(string value) { m_string1 = make_shared<string>(value); }

    shared_ptr<string> ContentDelete::GetRootTxHash() const { return m_string2; }
    void ContentDelete::SetRootTxHash(string value) { m_string2 = make_shared<string>(value); }


    shared_ptr <UniValue> ContentDelete::Serialize() const
    {
        auto result = Transaction::Serialize();

        result->pushKV("address", *GetAddress());
        result->pushKV("txidEdit", *GetRootTxHash());
        
        result->pushKV("settings", (m_payload && m_payload->GetString1()) ? *m_payload->GetString1() : "");

        return result;
    }

    void ContentDelete::Deserialize(const UniValue& src)
    {
        Transaction::Deserialize(src);
        if (auto[ok, val] = TryGetStr(src, "address"); ok) SetAddress(val);
        if (auto[ok, valTxId] = TryGetStr(src, "txid"); ok) SetRootTxHash(valTxId);
    }

    void ContentDelete::DeserializeRpc(const UniValue& src, const CTransactionRef& tx)
    {
        if (auto[ok, val] = TryGetStr(src, "address"); ok) SetAddress(val);
        if (auto[ok, val] = TryGetStr(src, "txidEdit"); ok) SetRootTxHash(val);

        GeneratePayload();
        if (auto[ok, val] = TryGetStr(src, "s"); ok) m_payload->SetString1(val);
    }
    
    void ContentDelete::DeserializePayload(const UniValue& src, const CTransactionRef& tx)
    {
        if (auto[ok, val] = TryGetStr(src, "settings"); ok)
        {
            Transaction::DeserializePayload(src, tx);
            m_payload->SetString1(val);
        }
    }

    void ContentDelete::BuildHash()
    {
        std::string data;

        data += *GetRootTxHash();
        data += m_payload->GetString1() ? *m_payload->GetString1() : "";

        Transaction::GenerateHash(data);
    }

} // namespace PocketTx

