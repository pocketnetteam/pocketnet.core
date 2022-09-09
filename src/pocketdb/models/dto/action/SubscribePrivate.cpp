// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include <primitives/transaction.h>
#include "pocketdb/models/dto/action/SubscribePrivate.h"

namespace PocketTx
{
    SubscribePrivate::SubscribePrivate() : Subscribe()
    {
        SetType(TxType::ACTION_SUBSCRIBE_PRIVATE);
    }

    SubscribePrivate::SubscribePrivate(const std::shared_ptr<const CTransaction>& tx) : Subscribe(tx)
    {
        SetType(TxType::ACTION_SUBSCRIBE_PRIVATE);
    }

    shared_ptr <UniValue> SubscribePrivate::Serialize() const
    {
        auto result = Subscribe::Serialize();

        result->pushKV("private", true);

        return result;
    }
} // namespace PocketTx