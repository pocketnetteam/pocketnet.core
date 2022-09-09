// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include <primitives/transaction.h>
#include "pocketdb/models/dto/money/Coinbase.h"

namespace PocketTx
{
    Coinbase::Coinbase() : Default()
    {
        SetType(TxType::TX_COINBASE);
    }

    Coinbase::Coinbase(const std::shared_ptr<const CTransaction>& tx) : Default(tx)
    {
        SetType(TxType::TX_COINBASE);
    }
} // namespace PocketTx