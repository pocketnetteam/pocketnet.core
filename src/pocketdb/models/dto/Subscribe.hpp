// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 Bitcoin developers
// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETTX_SUBSCRIBE_HPP
#define POCKETTX_SUBSCRIBE_HPP

#include "pocketdb/models/base/Transaction.hpp"

namespace PocketTx
{

    class Subscribe : public Transaction
    {
    public:

        Subscribe(string hash, int64_t time) : Transaction(hash, time)
        {
            SetType(PocketTxType::ACTION_SUBSCRIBE);
        }

        void Deserialize(const UniValue& src) override
        {
            Transaction::Deserialize(src);
            if (auto[ok, val] = TryGetStr(src, "address_to"); ok) SetAddressTo(val);
        }

        shared_ptr <int64_t> GetAddressToId() const { return m_int1; }
        string GetAddressToStr() const { return m_address_to == nullptr ? "" : *m_address_to; }
        void SetAddressToId(int64_t value) { m_int1 = make_shared<int64_t>(value); }
        void SetAddressTo(string value) { m_address_to = make_shared<string>(value); }

    protected:
        shared_ptr <string> m_address_to = nullptr;

    private:

        void BuildPayload(const UniValue& src) override
        {
        }

        void BuildHash(const UniValue& src) override
        {
            std::string data;
            data += GetAddressToStr();
            Transaction::GenerateHash(data);
        }
    };

} // namespace PocketTx

#endif //POCKETTX_SUBSCRIBE_HPP
