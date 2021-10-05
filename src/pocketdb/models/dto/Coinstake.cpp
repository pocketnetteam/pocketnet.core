// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include <primitives/transaction.h>
#include "pocketdb/models/dto/Coinstake.h"

namespace PocketTx
{
    Coinstake::Coinstake() : Transaction()
    {
        SetType(TxType::TX_COINSTAKE);
    }

    Coinstake::Coinstake(const std::shared_ptr<const CTransaction>& tx) : Transaction(tx)
    {
        SetType(TxType::TX_COINSTAKE);
    }

    void Coinstake::Deserialize(const UniValue& src)
    {

    }

    void Coinstake::DeserializePayload(const UniValue& src, const std::shared_ptr<const CTransaction>& tx)
    {

    }
    void Coinstake::BuildHash()
    {

    }

} // namespace PocketTx