// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include "pocketdb/models/base/SocialTransaction.h"

namespace PocketTx
{
    SocialTransaction::SocialTransaction() : Transaction()
    {
    }

    SocialTransaction::SocialTransaction(const CTransactionRef& tx) : Transaction(tx)
    {
    }

    shared_ptr<UniValue> SocialTransaction::Serialize() const
    {
        auto result = Transaction::Serialize();

        if (GetString1()) result->pushKV("s1", *GetString1());
        if (GetString2()) result->pushKV("s2", *GetString2());
        if (GetString3()) result->pushKV("s3", *GetString3());
        if (GetString4()) result->pushKV("s4", *GetString4());
        if (GetString5()) result->pushKV("s5", *GetString5());
        if (GetInt1()) result->pushKV("i1", *GetInt1());

        if (m_payload)
        {
            UniValue payload(UniValue::VOBJ);

            if (m_payload->GetTxHash()) payload.pushKV("h", *m_payload->GetTxHash());
            if (m_payload->GetString1()) payload.pushKV("s1", *m_payload->GetString1());
            if (m_payload->GetString2()) payload.pushKV("s2", *m_payload->GetString2());
            if (m_payload->GetString3()) payload.pushKV("s3", *m_payload->GetString3());
            if (m_payload->GetString4()) payload.pushKV("s4", *m_payload->GetString4());
            if (m_payload->GetString5()) payload.pushKV("s5", *m_payload->GetString5());
            if (m_payload->GetString6()) payload.pushKV("s6", *m_payload->GetString6());
            if (m_payload->GetString7()) payload.pushKV("s7", *m_payload->GetString7());
            if (m_payload->GetInt1()) payload.pushKV("i1", *m_payload->GetInt1());

            if (!payload.empty())
                result->pushKV("p", payload);
        }

        return result;
    }

    void SocialTransaction::Deserialize(const UniValue& src)
    {
        Transaction::Deserialize(src);

        // Deserialize all general fields
        if (auto[ok, val] = TryGetStr(src, "s1"); ok) SetString1(val);
        if (auto[ok, val] = TryGetStr(src, "s2"); ok) SetString2(val);
        if (auto[ok, val] = TryGetStr(src, "s3"); ok) SetString3(val);
        if (auto[ok, val] = TryGetStr(src, "s4"); ok) SetString4(val);
        if (auto[ok, val] = TryGetStr(src, "s5"); ok) SetString5(val);
        if (auto[ok, val] = TryGetInt64(src, "i1"); ok) SetInt1(val);

        // Deserialize payload part if exists non-empty "p" object
        if (auto[pOk, pVal] = TryGetObj(src, "p"); pOk)
        {
            m_payload = make_shared<Payload>();
            if (auto[ok, val] = TryGetStr(pVal, "h"); ok) m_payload->SetTxHash(val);
            if (auto[ok, val] = TryGetStr(pVal, "s1"); ok) m_payload->SetString1(val);
            if (auto[ok, val] = TryGetStr(pVal, "s2"); ok) m_payload->SetString2(val);
            if (auto[ok, val] = TryGetStr(pVal, "s3"); ok) m_payload->SetString3(val);
            if (auto[ok, val] = TryGetStr(pVal, "s4"); ok) m_payload->SetString4(val);
            if (auto[ok, val] = TryGetStr(pVal, "s5"); ok) m_payload->SetString5(val);
            if (auto[ok, val] = TryGetStr(pVal, "s6"); ok) m_payload->SetString6(val);
            if (auto[ok, val] = TryGetStr(pVal, "s7"); ok) m_payload->SetString7(val);
            if (auto[ok, val] = TryGetInt64(pVal, "i1"); ok) m_payload->SetInt1(val);
        }
    }
    
    shared_ptr<string> SocialTransaction::GetAddress() const { return m_string1; }
    void SocialTransaction::SetAddress(const string& value) { m_string1 = make_shared<string>(value); }

} // namespace PocketTx

