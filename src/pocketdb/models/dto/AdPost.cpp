// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include <primitives/transaction.h>
#include "pocketdb/models/dto/AdPost.h"

namespace PocketTx
{
    AdPost::AdPost() : Transaction()
    {
        SetType(TxType::ACTION_AD_POST);
    }

    AdPost::AdPost(const CTransactionRef& tx) : Transaction(tx)
    {
        SetType(TxType::ACTION_AD_POST);
    }

    shared_ptr <UniValue> AdPost::Serialize() const
    {
        auto result = Transaction::Serialize();

        result->pushKV("address", GetAddress() ? *GetAddress() : "");
        result->pushKV("contenttxhash", GetContentTxHash() ? *GetContentTxHash() : "");

        return result;
    }

    void AdPost::Deserialize(const UniValue& src)
    {
        Transaction::Deserialize(src);
        if (auto[ok, val] = TryGetStr(src, "address"); ok) SetAddress(val);
        if (auto[ok, val] = TryGetStr(src, "contenttxhash"); ok) SetContentTxHash(val);
    }

    void AdPost::DeserializeRpc(const UniValue& src)
    {
        if (auto[ok, val] = TryGetStr(src, "c"); ok) SetContentTxHash(val);
    }
    
    void AdPost::DeserializePayload(const UniValue& src)
    {
        Transaction::DeserializePayload(src);
    }
    

    shared_ptr <string> AdPost::GetAddress() const { return m_string1; }
    void AdPost::SetAddress(const string& value) { m_string1 = make_shared<string>(value); }

    shared_ptr <string> AdPost::GetContentTxHash() const { return m_string2; }
    void AdPost::SetContentTxHash(const string& value) { m_string2 = make_shared<string>(value); }


    string AdPost::BuildHash()
    {
        std::string data;

        data += GetContentTxHash() ? *GetContentTxHash() : "";

        return Transaction::GenerateHash(data);
    }
} // namespace PocketTx