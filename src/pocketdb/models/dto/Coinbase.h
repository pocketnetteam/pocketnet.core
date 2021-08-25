// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETTX_COINBASE_H
#define POCKETTX_COINBASE_H

#include "pocketdb/models/base/Transaction.h"

namespace PocketTx
{

    class Coinbase : public Transaction
    {
    public:
        Coinbase(const string& hash, int64_t time);
        void Deserialize(const UniValue& src) override;
    protected:
        void DeserializePayload(const UniValue& src) override;
        void BuildHash() override;
    };

} // namespace PocketTx

#endif // POCKETTX_COINBASE_H
