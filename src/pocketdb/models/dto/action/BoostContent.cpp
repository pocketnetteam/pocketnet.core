// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include <primitives/transaction.h>
#include "pocketdb/models/dto/action/BoostContent.h"

namespace PocketTx
{
    BoostContent::BoostContent() : Transaction()
    {
        SetType(TxType::BOOST_CONTENT);
    }

    BoostContent::BoostContent(const CTransactionRef& tx) : Transaction(tx)
    {
        SetType(TxType::BOOST_CONTENT);
    }

    optional <UniValue> BoostContent::Serialize() const
    {
        auto result = Transaction::Serialize();

        result->pushKV("address", GetAddress() ? *GetAddress() : "");
        result->pushKV("contenttxhash", GetContentTxHash() ? *GetContentTxHash() : "");

        return result;
    }

    void BoostContent::Deserialize(const UniValue& src)
    {
        Transaction::Deserialize(src);
        if (auto[ok, val] = TryGetStr(src, "address"); ok) SetAddress(val);
        if (auto[ok, val] = TryGetStr(src, "contenttxhash"); ok) SetContentTxHash(val);
    }

    void BoostContent::DeserializeRpc(const UniValue& src)
    {
        if (auto[ok, val] = TryGetStr(src, "content"); ok) SetContentTxHash(val);
    }
    
    void BoostContent::DeserializePayload(const UniValue& src)
    {
        Transaction::DeserializePayload(src);
    }
    

    const optional <string>& BoostContent::GetAddress() const { return m_string1; }
    void BoostContent::SetAddress(const string& value) { m_string1 = value; }

    const optional <string>& BoostContent::GetContentTxHash() const { return m_string2; }
    void BoostContent::SetContentTxHash(const string& value) { m_string2 = value; }


    string BoostContent::BuildHash()
    {
        std::string data;

        data += GetContentTxHash() ? *GetContentTxHash() : "";

        return Transaction::GenerateHash(data);
    }
} // namespace PocketTx