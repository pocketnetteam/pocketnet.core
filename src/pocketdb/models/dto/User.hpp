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
            if (auto[ok, val] = TryGetInt64(src, "regdate"); ok) SetRegistration(val);
            if (auto[ok, val] = TryGetStr(src, "referrer"); ok) SetReferrerAddress(val);
        }

        shared_ptr <int64_t> GetRegistration() const { return m_int1; }
        void SetRegistration(int64_t value) { m_int1 = make_shared<int64_t>(value); }

        shared_ptr <int64_t> GetReferrerId() const { return m_int2; }
        shared_ptr <string> GetReferrerAddress() const { return m_referrer_address; }
        void SetReferrer(std::int64_t value) { m_int2 = make_shared<int64_t>(value); }
        void SetReferrerAddress(std::string value) { m_referrer_address = make_shared<string>(value); }

    protected:

        shared_ptr <string> m_referrer_address = nullptr;

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
            data += m_payload->GetString2Str();
            data += m_payload->GetString5Str();
            data += m_payload->GetString1Str();
            data += m_payload->GetString4Str();
            data += m_payload->GetString3Str();
            data += m_payload->GetString7Str();

            // TODO (brangr): реферер учитывается только для новой записи
            // в остальных случаях не использовать
            data += GetReferrerAddress() == nullptr ? "" : *GetReferrerAddress();

            data += m_payload->GetString6Str();

            Transaction::GenerateHash(data);
        }

    }; // class User

} // namespace PocketTx

#endif // POCKETTX_USER_HPP
