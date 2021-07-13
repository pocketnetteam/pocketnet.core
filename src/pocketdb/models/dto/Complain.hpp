// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 Bitcoin developers
// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETTX_COMPLAIN_HPP
#define POCKETTX_COMPLAIN_HPP

#include "pocketdb/models/base/Transaction.hpp"

namespace PocketTx
{

    class Complain : public Transaction
    {
    public:

        Complain(string& hash, int64_t time, string& opReturn) : Transaction(hash, time, opReturn)
        {
            SetType(PocketTxType::ACTION_COMPLAIN);
        }

        shared_ptr<UniValue> Serialize() const override
        {
            auto result = Transaction::Serialize();

            if (GetAddress()) result->pushKV("address", *GetAddress());
            if (GetReason()) result->pushKV("reason", *GetReason());
            if (GetPostTxHash()) result->pushKV("posttxid", *GetPostTxHash());

            return result;
        }

        void Deserialize(const UniValue& src) override
        {
            Transaction::Deserialize(src);
            if (auto[ok, val] = TryGetStr(src, "address"); ok) SetAddress(val);
            if (auto[ok, val] = TryGetInt64(src, "reason"); ok) SetReason(val);
            if (auto[ok, val] = TryGetStr(src, "posttxid"); ok) SetPostTxHash(val);
        }

        shared_ptr<string> GetAddress() const { return m_string1; }
        void SetAddress(string value) { m_string1 = make_shared<string>(value); }

        shared_ptr<string> GetPostTxHash() const { return m_string2; }
        void SetPostTxHash(string value) { m_string2 = make_shared<string>(value); }

        shared_ptr <int64_t> GetReason() const { return m_int1; }
        void SetReason(int64_t value) { m_int1 = make_shared<int64_t>(value); }

    protected:

        void DeserializePayload(const UniValue& src) override
        {
        }

        void BuildHash() override
        {
            string data;
            data += GetPostTxHash() ? *GetPostTxHash() : "";
            data += "_";
            data += GetReason() ? std::to_string(*GetReason()) : "";
            Transaction::GenerateHash(data);
        }
    };

} // namespace PocketTx

#endif // POCKETTX_COMPLAIN_HPP
