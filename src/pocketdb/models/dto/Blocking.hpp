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
        ~Blocking() = default;

        Blocking(const UniValue &src) : base(src)
        {
            SetTxType(PocketTxType::BLOCKING_ACTION);

            assert(src.exists("address_to") && src["address_to"].isStr());
            SetAddressTo(src["address_to"].get_str());
        }

        [[nodiscard]] std::string* GetAddressTo() const { return m_string1; }
        void SetAddressTo(std::string value) { m_string1 = new std::string(std::move(value)); }
    };

} // namespace PocketTx

#endif // POCKETTX_BLOCKING_HPP
