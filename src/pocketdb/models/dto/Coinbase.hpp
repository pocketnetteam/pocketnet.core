// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 Bitcoin developers
// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETTX_COINBASE_HPP
#define POCKETTX_COINBASE_HPP

#include "pocketdb/models/base/Transaction.hpp"

namespace PocketTx
{

    class Coinbase : public Transaction
    {
    public:

        Coinbase(string& hash, int64_t time, shared_ptr<string> opReturn) : Transaction(hash, time, opReturn)
        {
            SetType(PocketTxType::TX_COINBASE);
        }

        void Deserialize(const UniValue& src) override {}

    protected:

        void BuildPayload(const UniValue& src) override {}
        void BuildHash(const UniValue& src) override {}

    };

} // namespace PocketTx

#endif // POCKETTX_COINBASE_HPP
