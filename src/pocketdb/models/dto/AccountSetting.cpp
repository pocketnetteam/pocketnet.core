// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include <primitives/transaction.h>
#include "pocketdb/models/dto/AccountSetting.h"

namespace PocketTx
{
    AccountSetting::AccountSetting() : Transaction()
    {
        SetType(PocketTxType::ACCOUNT_SETTING);
    }

    AccountSetting::AccountSetting(const CTransactionRef& tx) : Transaction(tx)
    {
        SetType(PocketTxType::ACCOUNT_SETTING);
    }

    
    shared_ptr <string> AccountSetting::GetAddress() const { return m_string1; }
    void AccountSetting::SetAddress(string value) { m_string1 = make_shared<string>(value); }


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
    
    void AccountSetting::DeserializePayload(const UniValue& src, const CTransactionRef& tx)
    {
        Transaction::DeserializePayload(src, tx);
        if (auto[ok, val] = TryGetStr(src, "data"); ok) m_payload->SetString1(val);
    }

    void AccountSetting::DeserializeRpc(const UniValue& src, const CTransactionRef& tx)
    {
        if (auto[ok, val] = TryGetStr(src, "address"); ok) SetAddress(val);

        GeneratePayload();
        if (auto[ok, val] = TryGetStr(src, "d"); ok) m_payload->SetString1(val);
    }

    void AccountSetting::BuildHash()
    {
        std::string data;

        data += m_payload->GetString1() ? *m_payload->GetString1() : "";

        Transaction::GenerateHash(data);
    }

} // namespace PocketTx

