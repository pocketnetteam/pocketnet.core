// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include "pocketdb/models/dto/Coinstake.h"

namespace PocketTx
{
    Coinstake::Coinstake(const string& hash, int64_t time) : Transaction(hash, time)
    {
        SetType(PocketTxType::TX_COINSTAKE);
    }

    void Coinstake::Deserialize(const UniValue& src)
    {

    }

    void Coinstake::DeserializePayload(const UniValue& src)
    {

    }
    void Coinstake::BuildHash()
    {

    }

} // namespace PocketTx