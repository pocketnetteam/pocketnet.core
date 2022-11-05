// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include <primitives/transaction.h>
#include "pocketdb/models/dto/action/Complain.h"

namespace PocketTx
{
    Complain::Complain() : Transaction()
    {
        SetType(TxType::ACTION_COMPLAIN);
    }

    Complain::Complain(const std::shared_ptr<const CTransaction>& tx) : Transaction(tx)
    {
        SetType(TxType::ACTION_COMPLAIN);
    }

    shared_ptr <UniValue> Complain::Serialize() const
    {
        auto result = Transaction::Serialize();

        result->pushKV("address", GetAddress() ? *GetAddress() : "");
        result->pushKV("reason", GetReason() ? *GetReason() : 0);
        result->pushKV("posttxid", GetPostTxHash() ? *GetPostTxHash() : "");

        return result;
    }

    void Complain::Deserialize(const UniValue& src)
    {
        Transaction::Deserialize(src);
        if (auto[ok, val] = TryGetStr(src, "address"); ok) SetAddress(val);
        if (auto[ok, val] = TryGetInt64(src, "reason"); ok) SetReason(val);
        if (auto[ok, val] = TryGetStr(src, "posttxid"); ok) SetPostTxHash(val);
    }

    void Complain::DeserializeRpc(const UniValue& src)
    {
        if (auto[ok, val] = TryGetStr(src, "share"); ok) SetPostTxHash(val);
        if (auto[ok, val] = TryGetInt64(src, "reason"); ok) SetReason(val);
    }

    shared_ptr <string> Complain::GetAddress() const { return m_string1; }
    void Complain::SetAddress(const string& value) { m_string1 = make_shared<string>(value); }

    shared_ptr <string> Complain::GetPostTxHash() const { return m_string2; }
    void Complain::SetPostTxHash(const string& value) { m_string2 = make_shared<string>(value); }

    shared_ptr <int64_t> Complain::GetReason() const { return m_int1; }
    void Complain::SetReason(int64_t value) { m_int1 = make_shared<int64_t>(value); }

    void Complain::DeserializePayload(const UniValue& src)
    {
    }

    string Complain::BuildHash()
    {
        string data;

        data += GetPostTxHash() ? *GetPostTxHash() : "";
        data += "_";
        data += GetReason() ? std::to_string(*GetReason()) : "";

        return Transaction::GenerateHash(data);
    }
} // namespace PocketTx