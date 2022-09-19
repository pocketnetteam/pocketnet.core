// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include <primitives/transaction.h>
#include "pocketdb/models/dto/money/Default.h"

namespace PocketTx
{
    Default::Default() : Transaction()
    {
        SetType(TxType::TX_DEFAULT);
    }

    Default::Default(const std::shared_ptr<const CTransaction>& tx) : Transaction(tx)
    {
        SetType(TxType::TX_DEFAULT);
    }

    void Default::Deserialize(const UniValue& src) {}
    void Default::DeserializePayload(const UniValue& src)
    {
    }

    string Default::BuildHash()
    {
        return "";
    }
} // namespace PocketTx