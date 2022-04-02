// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include <primitives/transaction.h>
#include "pocketdb/models/dto/Empty.h"

namespace PocketTx
{
    Empty::Empty() : Default()
    {
        SetType(TxType::TX_EMPTY);
    }

    Empty::Empty(const shared_ptr<const CTransaction>& tx) : Default(tx)
    {
        SetType(TxType::TX_EMPTY);
    }
} // namespace PocketTx