// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include <primitives/transaction.h>
#include "pocketdb/models/dto/action/Blocking.h"

namespace PocketTx
{

    Blocking::Blocking() : Transaction()
    {
        SetType(TxType::ACTION_BLOCKING);
    }

    Blocking::Blocking(const std::shared_ptr<const CTransaction>& tx) : Transaction(tx)
    {
        SetType(TxType::ACTION_BLOCKING);
    }

    shared_ptr <UniValue> Blocking::Serialize() const
    {
        auto result = Transaction::Serialize();

        result->pushKV("address", GetAddress() ? *GetAddress() : "");
        result->pushKV("address_to", GetAddressTo() ? *GetAddressTo() : "");
        result->pushKV("addresses_to", GetAddressesTo() ? *GetAddressesTo() : "");
        result->pushKV("unblocking", false);

        return result;
    }

    void Blocking::Deserialize(const UniValue& src)
    {
        Transaction::Deserialize(src);
        if (auto[ok, val] = TryGetStr(src, "address"); ok) SetAddress(val);
        if (auto[ok, val] = TryGetStr(src, "address_to"); ok) SetAddressTo(val);
        if (auto[ok, val] = TryGetStr(src, "addresses_to"); ok) SetAddressesTo(val);
    }

    void Blocking::DeserializeRpc(const UniValue& src)
    {
        if (auto[ok, val] = TryGetStr(src, "address"); ok) SetAddressTo(val);
        if (auto[ok, val] = TryGetStr(src, "addresses"); ok) SetAddressesTo(val);
    }

    shared_ptr <string> Blocking::GetAddress() const { return m_string1; }
    void Blocking::SetAddress(const string& value) { m_string1 = make_shared<string>(value); }

    shared_ptr <string> Blocking::GetAddressTo() const { return m_string2; }
    shared_ptr <string> Blocking::GetAddressesTo() const { return m_string3; }
    void Blocking::SetAddressTo(const string& value) { m_string2 = make_shared<string>(value); }
    void Blocking::SetAddressesTo(const string& value) { m_string3 = make_shared<string>(value); }

    void Blocking::DeserializePayload(const UniValue& src)
    {
    }

    string Blocking::BuildHash()
    {
        string data;
        data += GetAddressTo() ? *GetAddressTo() : "";
        if (GetAddressesTo())
        {
            UniValue addrs(UniValue::VARR);
            addrs.read(*GetAddressesTo());
            for (size_t i = 0; i < addrs.size(); ++i)
            {
                data += (i ? "," : "");
                data += addrs[i].get_str();
            }
        }
        return Transaction::GenerateHash(data);
    }


} // namespace PocketTx
