// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include <primitives/transaction.h>
#include "pocketdb/models/dto/Coinbase.h"

namespace PocketTx
{
    Coinbase::Coinbase() : Transaction()
    {
        SetType(PocketTxType::TX_COINBASE);
    }

    Coinbase::Coinbase(const std::shared_ptr<const CTransaction>& tx) : Transaction(tx)
    {
        SetType(PocketTxType::TX_COINBASE);
    }

    void Coinbase::Deserialize(const UniValue& src)
    {

    }

    void Coinbase::DeserializePayload(const UniValue& src, const std::shared_ptr<const CTransaction>& tx)
    {

    }
    void Coinbase::BuildHash()
    {
    }
} // namespace PocketTx