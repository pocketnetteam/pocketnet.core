// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include "pocketdb/models/dto/Subscribe.h"

namespace PocketTx
{
    Subscribe::Subscribe(const string& hash, int64_t time) : Transaction(hash, time)
    {
        SetType(PocketTxType::ACTION_SUBSCRIBE);
    }

    shared_ptr <UniValue> Subscribe::Serialize() const
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
        if (auto[ok, val] = TryGetStr(src, "txAddress"); ok) SetAddress(val);
        if (auto[ok, val] = TryGetStr(src, "address"); ok) SetAddressTo(val);
    }

    shared_ptr <string> Subscribe::GetAddress() const { return m_string1; }
    void Subscribe::SetAddress(string value) { m_string1 = make_shared<string>(value); }

    shared_ptr <string> Subscribe::GetAddressTo() const { return m_string2; }
    void Subscribe::SetAddressTo(string value) { m_string2 = make_shared<string>(value); }

    void Subscribe::DeserializePayload(const UniValue& src)
    {
    }

    void Subscribe::BuildHash()
    {
        std::string data;
        data += GetAddressTo() ? *GetAddressTo() : "";
        Transaction::GenerateHash(data);
    }
} // namespace PocketTx