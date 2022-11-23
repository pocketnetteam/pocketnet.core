// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include <primitives/transaction.h>
#include "pocketdb/models/dto/action/Subscribe.h"

namespace PocketTx
{
    Subscribe::Subscribe() : Transaction()
    {
        SetType(TxType::ACTION_SUBSCRIBE);
    }

    Subscribe::Subscribe(const std::shared_ptr<const CTransaction>& tx) : Transaction(tx)
    {
        SetType(TxType::ACTION_SUBSCRIBE);
    }

    optional <UniValue> Subscribe::Serialize() const
    {
        auto result = Transaction::Serialize();

        result->pushKV("address", GetAddress() ? *GetAddress() : "");
        result->pushKV("address_to", GetAddressTo() ? *GetAddressTo() : "");
        result->pushKV("private", false);
        result->pushKV("unsubscribe", false);

        return result;
    }

    void Subscribe::Deserialize(const UniValue& src)
    {
        Transaction::Deserialize(src);
        if (auto[ok, val] = TryGetStr(src, "address"); ok) SetAddress(val);
        if (auto[ok, val] = TryGetStr(src, "address_to"); ok) SetAddressTo(val);
    }

    void Subscribe::DeserializeRpc(const UniValue& src)
    {
        if (auto[ok, val] = TryGetStr(src, "address"); ok) SetAddressTo(val);
    }

    const optional <string>& Subscribe::GetAddress() const { return m_string1; }
    void Subscribe::SetAddress(const string& value) { m_string1 = value; }

    const optional <string>& Subscribe::GetAddressTo() const { return m_string2; }
    void Subscribe::SetAddressTo(const string& value) { m_string2 = value; }

    void Subscribe::DeserializePayload(const UniValue& src)
    {
    }

    string Subscribe::BuildHash()
    {
        std::string data;

        data += GetAddressTo() ? *GetAddressTo() : "";

        return Transaction::GenerateHash(data);
    }
} // namespace PocketTx