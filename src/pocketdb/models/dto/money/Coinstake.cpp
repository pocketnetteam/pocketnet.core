// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include <primitives/transaction.h>
#include "pocketdb/models/dto/money/Coinstake.h"

namespace PocketTx
{
    Coinstake::Coinstake() : Default()
    {
        SetType(TxType::TX_COINSTAKE);
    }

    Coinstake::Coinstake(const std::shared_ptr<const CTransaction>& tx) : Default(tx)
    {
        SetType(TxType::TX_COINSTAKE);
    }

} // namespace PocketTx