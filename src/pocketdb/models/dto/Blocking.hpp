// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 Bitcoin developers
// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETTX_BLOCKING_HPP
#define POCKETTX_BLOCKING_HPP

#include "pocketdb/models/base/Transaction.hpp"

namespace PocketTx
{

    class Blocking : public Transaction
    {
    public:

        Blocking(string& hash, int64_t time) : Transaction(hash, time)
        {
            SetType(PocketTxType::ACTION_BLOCKING);
        }

        shared_ptr<UniValue> Serialize() const override
        {
            auto result = Transaction::Serialize();

            if (GetAddress()) result->pushKV("address", *GetAddress());
            if (GetAddressTo()) result->pushKV("address_to", *GetAddressTo());

            return result;
        }

        void Deserialize(const UniValue& src) override
        {
            Transaction::Deserialize(src);
            if (auto[ok, val] = TryGetStr(src, "address"); ok) SetAddress(val);
            if (auto[ok, val] = TryGetStr(src, "address_to"); ok) SetAddressTo(val);
        }

        shared_ptr<string> GetAddress() const { return m_string1; }
        void SetAddress(string value) { m_string1 = make_shared<string>(value); }

        shared_ptr<string> GetAddressTo() const { return m_string2; }
        void SetAddressTo(string value) { m_string2 = make_shared<string>(value); }

    protected:

        void BuildPayload(const UniValue& src) override {}

        void BuildHash(const UniValue& src) override
        {
            string data;
            data += GetAddressTo() ? *GetAddressTo() : "";
            Transaction::GenerateHash(data);
        }
    };

} // namespace PocketTx

#endif // POCKETTX_BLOCKING_HPP
