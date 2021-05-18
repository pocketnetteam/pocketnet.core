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

        User(string hash, int64_t time) : Transaction(hash, time)
        {
            SetType(PocketTxType::ACCOUNT_USER);
        }

        void Deserialize(const UniValue& src) override
        {
            Transaction::Deserialize(src);
            if (auto[ok, val] = TryGetStr(src, "address"); ok) SetAddress(val);
            if (auto[ok, val] = TryGetStr(src, "referrer"); ok) SetReferrerAddress(val);
            SetRegistration(*GetTime());
        }

        shared_ptr <string> GetAddress() const { return m_string1; }
        void SetAddress(string value) { m_string1 = make_shared<string>(value); }

        shared_ptr <string> GetReferrerAddress() const { return m_string2; }
        void SetReferrerAddress(std::string value) { m_string2 = make_shared<string>(value); }

        shared_ptr <int64_t> GetRegistration() const { return m_int1; }
        void SetRegistration(int64_t value) { m_int1 = make_shared<int64_t>(value); }

    protected:

    private:

        void BuildPayload(const UniValue& src) override
        {
            Transaction::BuildPayload(src);

            if (auto[ok, val] = TryGetStr(src, "lang"); ok) m_payload->SetString1(val);
            if (auto[ok, val] = TryGetStr(src, "name"); ok) m_payload->SetString2(val);
            if (auto[ok, val] = TryGetStr(src, "avatar"); ok) m_payload->SetString3(val);
            if (auto[ok, val] = TryGetStr(src, "about"); ok) m_payload->SetString4(val);
            if (auto[ok, val] = TryGetStr(src, "url"); ok) m_payload->SetString5(val);
            if (auto[ok, val] = TryGetStr(src, "pubkey"); ok) m_payload->SetString6(val);
            if (auto[ok, val] = TryGetStr(src, "donations"); ok) m_payload->SetString7(val);
        }

        void BuildHash(const UniValue& src) override
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
