// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include <primitives/transaction.h>
#include "pocketdb/models/dto/SubscribeCancel.h"

namespace PocketTx
{
    SubscribeCancel::SubscribeCancel(const string& hash, int64_t time) : Subscribe(hash, time)
    {
        SetType(PocketTxType::ACTION_SUBSCRIBE_CANCEL);
    }

    SubscribeCancel::SubscribeCancel(const std::shared_ptr<const CTransaction>& tx) : Subscribe(tx)
    {
        SetType(PocketTxType::ACTION_SUBSCRIBE_CANCEL);
    }

    shared_ptr <UniValue> SubscribeCancel::Serialize() const
    {
        auto result = Subscribe::Serialize();
        result->pushKV("unsubscribe", true);
        return result;
    }
} // namespace PocketTx
