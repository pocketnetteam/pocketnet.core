// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include "pocketdb/models/dto/Coinbase.h"

namespace PocketTx
{
    Coinbase::Coinbase(const string& hash, int64_t time) : Transaction(hash, time)
    {
        SetType(PocketTxType::TX_COINBASE);
    }

    void Coinbase::Deserialize(const UniValue& src)
    {

    }

    void Coinbase::DeserializePayload(const UniValue& src)
    {

    }
    void Coinbase::BuildHash()
    {
    }
} // namespace PocketTx