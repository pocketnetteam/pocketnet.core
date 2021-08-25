// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include "pocketdb/models/dto/SubscribePrivate.h"

namespace PocketTx
{
    SubscribePrivate::SubscribePrivate(const string& hash, int64_t time) : Subscribe(hash, time)
    {
        SetType(PocketTxType::ACTION_SUBSCRIBE_PRIVATE);
    }

    shared_ptr <UniValue> SubscribePrivate::Serialize() const
    {
        auto result = Subscribe::Serialize();

        result->pushKV("private", true);

        return result;
    }
} // namespace PocketTx