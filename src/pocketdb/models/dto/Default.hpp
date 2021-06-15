// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 Bitcoin developers
// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETTX_DEFAULT_HPP
#define POCKETTX_DEFAULT_HPP

#include "pocketdb/models/base/Transaction.hpp"

namespace PocketTx
{

    class Default : public Transaction
    {
    public:

        Default(string& hash, int64_t time) : Transaction(hash, time)
        {
            SetType(PocketTxType::TX_DEFAULT);
        }

        void Deserialize(const UniValue& src) override {}

    protected:

        void BuildPayload(const UniValue& src) override {}
        void BuildHash(const UniValue& src) override {}

    };

} // namespace PocketTx

#endif // POCKETTX_DEFAULT_HPP
