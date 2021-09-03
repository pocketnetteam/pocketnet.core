// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETTX_COINSTAKE_H
#define POCKETTX_COINSTAKE_H

#include "pocketdb/models/base/Transaction.h"

namespace PocketTx
{
    class Coinstake : public Transaction
    {
    public:
        Coinstake(const string& hash, int64_t time);
        Coinstake(const std::shared_ptr<const CTransaction>& tx);

        void Deserialize(const UniValue& src) override;
    protected:
        void DeserializePayload(const UniValue& src) override;
        void BuildHash() override;
    };
} // namespace PocketTx

#endif // POCKETTX_COINSTAKE_H
