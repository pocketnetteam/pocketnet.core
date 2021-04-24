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

        Complain() : Transaction()
        {
            SetTxType(PocketTxType::COMPLAIN_ACTION);
        }

        void Deserialize(const UniValue& src) override
        {
            Transaction::Deserialize(src);
            if (auto[ok, val] = TryGetInt64(src, "reason"); ok) SetReason(val);
            if (auto[ok, val] = TryGetStr(src, "posttxid"); ok) SetPostTxId(val);
        }

        shared_ptr<int64_t> GetReason() const { return m_int1; }
        void SetReason(int64_t value) { m_int1 = make_shared<int64_t>(value); }

        shared_ptr<string> GetPostTxId() const { return m_string1; }
        void SetPostTxId(std::string value) { m_string1 = make_shared<string>(value); }

    private:

        void BuildPayload(const UniValue &src) override
        {
        }

        void BuildHash(const UniValue &src) override
        {
            std::string data;
            if (auto[ok, val] = TryGetStr(src, "posttxid"); ok) data += val;
            data += "_";
            if (auto[ok, val] = TryGetStr(src, "reason"); ok) data += val;
            Transaction::GenerateHash(data);
        }
    };

} // namespace PocketTx

#endif // POCKETTX_COMPLAIN_HPP
