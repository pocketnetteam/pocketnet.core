// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 Bitcoin developers
// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETTX_USER_HPP
#define POCKETTX_USER_HPP

#include "pocketdb/models/base/Transaction.hpp"

namespace PocketTx
{
    class User : public PocketTx::Transaction
    {
    public:

        User(string& hash, int64_t time) : Transaction(hash, time)
        {
            SetType(PocketTxType::ACCOUNT_USER);
        }

        shared_ptr<UniValue> Serialize() const override
        {
            auto result = Transaction::Serialize();

            result->pushKV("address", GetAddress() ? *GetAddress() : "");
            result->pushKV("referrer", GetReferrerAddress() ? *GetReferrerAddress() : "");
            result->pushKV("regdate", *GetTime());
            
            result->pushKV("lang", (m_payload && m_payload->GetString1()) ? *m_payload->GetString1() : "en");
            result->pushKV("name", (m_payload && m_payload->GetString2()) ? *m_payload->GetString2() : "");
            result->pushKV("avatar", (m_payload && m_payload->GetString3()) ? *m_payload->GetString3() : "");
            result->pushKV("about", (m_payload && m_payload->GetString4()) ? *m_payload->GetString4() : "");
            result->pushKV("url", (m_payload && m_payload->GetString5()) ? *m_payload->GetString5() : "");
            result->pushKV("pubkey", (m_payload && m_payload->GetString6()) ? *m_payload->GetString6() : "");
            result->pushKV("donations", (m_payload && m_payload->GetString7()) ? *m_payload->GetString7() : "");

            result->pushKV("birthday", 0);
            result->pushKV("gender", 0);
            result->pushKV("id", 0);

            return result;
        }

        void Deserialize(const UniValue& src) override
        {
            Transaction::Deserialize(src);
            if (auto[ok, val] = TryGetStr(src, "address"); ok) SetAddress(val);
            if (auto[ok, val] = TryGetStr(src, "referrer"); ok) SetReferrerAddress(val);
        }

        void DeserializeRpc(const UniValue& src) override
        {
            if (auto[ok, val] = TryGetStr(src, "txAddress"); ok) SetAddress(val);
            if (auto[ok, val] = TryGetStr(src, "r"); ok) SetReferrerAddress(val);

            GeneratePayload();
            if (auto[ok, val] = TryGetStr(src, "l"); ok) m_payload->SetString1(val);
            else m_payload->SetString1("en");
            if (auto[ok, val] = TryGetStr(src, "n"); ok) m_payload->SetString2(val);
            if (auto[ok, val] = TryGetStr(src, "i"); ok) m_payload->SetString3(val);
            if (auto[ok, val] = TryGetStr(src, "a"); ok) m_payload->SetString4(val);
            if (auto[ok, val] = TryGetStr(src, "s"); ok) m_payload->SetString5(val);
            if (auto[ok, val] = TryGetStr(src, "k"); ok) m_payload->SetString6(val);
            if (auto[ok, val] = TryGetStr(src, "b"); ok) m_payload->SetString7(val);
        }

        shared_ptr <string> GetAddress() const { return m_string1; }
        void SetAddress(string value) { m_string1 = make_shared<string>(value); }

        shared_ptr <string> GetReferrerAddress() const { return m_string2; }
        void SetReferrerAddress(std::string value) { m_string2 = make_shared<string>(value); }

        // Payload getters
        shared_ptr <string> GetPayloadName() const { return Transaction::GetPayload()->GetString2(); }
        shared_ptr <string> GetPayloadAvatar() const { return Transaction::GetPayload()->GetString3(); }

    protected:

        void DeserializePayload(const UniValue& src) override
        {
            Transaction::DeserializePayload(src);

            if (auto[ok, val] = TryGetStr(src, "lang"); ok) m_payload->SetString1(val);
            if (auto[ok, val] = TryGetStr(src, "name"); ok) m_payload->SetString2(val);
            if (auto[ok, val] = TryGetStr(src, "avatar"); ok) m_payload->SetString3(val);
            if (auto[ok, val] = TryGetStr(src, "about"); ok) m_payload->SetString4(val);
            if (auto[ok, val] = TryGetStr(src, "url"); ok) m_payload->SetString5(val);
            if (auto[ok, val] = TryGetStr(src, "pubkey"); ok) m_payload->SetString6(val);
            if (auto[ok, val] = TryGetStr(src, "donations"); ok) m_payload->SetString7(val);
        }

        void BuildHash() override
        {
            std::string data;

            data += m_payload->GetString2() ? *m_payload->GetString2() : "";
            data += m_payload->GetString5() ? *m_payload->GetString5() : "";
            data += m_payload->GetString1() ? *m_payload->GetString1() : "";
            data += m_payload->GetString4() ? *m_payload->GetString4() : "";
            data += m_payload->GetString3() ? *m_payload->GetString3() : "";
            data += m_payload->GetString7() ? *m_payload->GetString7() : "";
            data += GetReferrerAddress() ? *GetReferrerAddress() : "";
            data += m_payload->GetString6() ? *m_payload->GetString6() : "";

            Transaction::GenerateHash(data);
        }

    }; // class User

} // namespace PocketTx

#endif // POCKETTX_USER_HPP
