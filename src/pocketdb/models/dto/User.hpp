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

        User() : Transaction()
        {
            SetTxType(PocketTxType::ACCOUNT_USER);
        }

        void Deserialize(const UniValue &src) override
        {
            Transaction::Deserialize(src);

            if (auto[ok, val] = TryGetStr(src, "regdate"); ok) SetRegistration(val);
            if (auto[ok, val] = TryGetStr(src, "referrer"); ok) SetReferrerAddress(val);
        }

        shared_ptr<int64_t> GetRegistration() const { return m_int1; }
        void SetRegistration(int64_t value) { m_int1 = make_shared<int64_t>(value); }

        shared_ptr<int64_t> GetReferrerId() const { return m_int2; }
        shared_ptr<string> GetReferrerAddress() const { return m_referrer_address; }
        void SetReferrer(std::int64_t value) { m_int2 = make_shared<int64_t>(value); }
        void SetReferrerAddress(std::string value) { m_referrer_address = make_shared<string>(value); }

    protected:

        shared_ptr<string> m_referrer_address = nullptr;

    private:

        void BuildPayload(const UniValue &src) override
        {
            // TODO (brangr): payload as object

            UniValue payload(UniValue::VOBJ);
            if (auto[ok, val] = TryGetInt64(src, "birthday"); ok) payload.pushKV("birthday", val);
            if (auto[ok, val] = TryGetInt64(src, "gender"); ok) payload.pushKV("gender", val);
            if (auto[ok, val] = TryGetStr(src, "avatar"); ok) payload.pushKV("avatar", val);
            if (auto[ok, val] = TryGetStr(src, "about"); ok) payload.pushKV("about", val);
            if (auto[ok, val] = TryGetStr(src, "url"); ok) payload.pushKV("url", val);
            if (auto[ok, val] = TryGetStr(src, "pubkey"); ok) payload.pushKV("pubkey", val);
            if (auto[ok, val] = TryGetStr(src, "donations"); ok) payload.pushKV("donations", val);
            SetPayload(payload);
        }

        void BuildHash(const UniValue &src) override
        {
            std::string data;
            if (auto[ok, val] = TryGetStr(src, "name"); ok) data += val;
            if (auto[ok, val] = TryGetStr(src, "url"); ok) data += val;
            if (auto[ok, val] = TryGetStr(src, "lang"); ok) data += val;
            if (auto[ok, val] = TryGetStr(src, "about"); ok) data += val;
            if (auto[ok, val] = TryGetStr(src, "avatar"); ok) data += val;
            if (auto[ok, val] = TryGetStr(src, "donations"); ok) data += val;
            if (auto[ok, val] = TryGetStr(src, "referrer"); ok) data += val;
            if (auto[ok, val] = TryGetStr(src, "pubkey"); ok) data += val;

            Transaction::GenerateHash(data);
        }

    }; // class User

} // namespace PocketTx

#endif // POCKETTX_USER_HPP
