// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include "pocketdb/models/dto/Blocking.h"

namespace PocketTx
{

    Blocking::Blocking(const string& hash, int64_t time) : Transaction(hash, time)
    {
        SetType(PocketTxType::ACTION_BLOCKING);
    }

    shared_ptr <UniValue> Blocking::Serialize() const
    {
        auto result = Transaction::Serialize();

        result->pushKV("address", GetAddress() ? *GetAddress() : "");
        result->pushKV("address_to", GetAddressTo() ? *GetAddressTo() : "");
        result->pushKV("unblocking", false);

        return result;
    }

    void Blocking::Deserialize(const UniValue& src)
    {
        Transaction::Deserialize(src);
        if (auto[ok, val] = TryGetStr(src, "address"); ok) SetAddress(val);
        if (auto[ok, val] = TryGetStr(src, "address_to"); ok) SetAddressTo(val);
    }

    void Blocking::DeserializeRpc(const UniValue& src)
    {
        if (auto[ok, val] = TryGetStr(src, "txAddress"); ok) SetAddress(val);
        if (auto[ok, val] = TryGetStr(src, "address"); ok) SetAddressTo(val);
    }

    shared_ptr <string> Blocking::GetAddress() const { return m_string1; }
    void Blocking::SetAddress(string value) { m_string1 = make_shared<string>(value); }

    shared_ptr <string> Blocking::GetAddressTo() const { return m_string2; }
    void Blocking::SetAddressTo(string value) { m_string2 = make_shared<string>(value); }

    void Blocking::DeserializePayload(const UniValue& src)
    {
    }

    void Blocking::BuildHash()
    {
        string data;
        data += GetAddressTo() ? *GetAddressTo() : "";
        Transaction::GenerateHash(data);
    }

} // namespace PocketTx
