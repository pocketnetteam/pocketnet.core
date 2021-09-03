// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include <primitives/transaction.h>
#include "pocketdb/models/dto/Default.h"

namespace PocketTx
{
    Default::Default(const string& hash, int64_t time) : Transaction(hash, time)
    {
        SetType(PocketTxType::TX_DEFAULT);
    }

    Default::Default(const std::shared_ptr<const CTransaction>& tx) : Transaction(tx)
    {
        SetType(PocketTxType::TX_DEFAULT);
    }

    void Default::Deserialize(const UniValue& src) {}
    void Default::DeserializePayload(const UniValue& src) {}
    void Default::BuildHash() {}
} // namespace PocketTx