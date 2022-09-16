// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include <primitives/transaction.h>
#include "pocketdb/models/dto/account/Setting.h"

namespace PocketTx
{
    AccountSetting::AccountSetting() : Transaction()
    {
        SetType(TxType::ACCOUNT_SETTING);
    }

    AccountSetting::AccountSetting(const CTransactionRef& tx) : Transaction(tx)
    {
        SetType(TxType::ACCOUNT_SETTING);
    }

    
    shared_ptr <string> AccountSetting::GetAddress() const { return m_string1; }
    void AccountSetting::SetAddress(const string& value) { m_string1 = make_shared<string>(value); }

    shared_ptr <string> AccountSetting::GetPayloadData() const {return GetPayload() ? GetPayload()->GetString1() : nullptr; }


    shared_ptr <UniValue> AccountSetting::Serialize() const
    {
        auto result = Transaction::Serialize();

        result->pushKV("address", GetAddress() ? *GetAddress() : "");
        result->pushKV("data", (m_payload && m_payload->GetString1()) ? *m_payload->GetString1() : "");

        return result;
    }

    void AccountSetting::Deserialize(const UniValue& src)
    {
        Transaction::Deserialize(src);
        if (auto[ok, val] = TryGetStr(src, "address"); ok) SetAddress(val);
    }
    
    void AccountSetting::DeserializePayload(const UniValue& src)
    {
        Transaction::DeserializePayload(src);
        if (auto[ok, val] = TryGetStr(src, "data"); ok) m_payload->SetString1(val);
    }

    void AccountSetting::DeserializeRpc(const UniValue& src)
    {
        GeneratePayload();
        if (auto[ok, val] = TryGetStr(src, "d"); ok) m_payload->SetString1(val);
    }

    

    string AccountSetting::BuildHash()
    {
        std::string data;

        data += m_payload && m_payload->GetString1() ? *m_payload->GetString1() : "";

        return Transaction::GenerateHash(data);
    }

} // namespace PocketTx

