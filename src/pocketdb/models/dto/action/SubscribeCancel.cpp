// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include <primitives/transaction.h>
#include "pocketdb/models/dto/action/SubscribeCancel.h"

namespace PocketTx
{
    SubscribeCancel::SubscribeCancel() : Subscribe()
    {
        SetType(TxType::ACTION_SUBSCRIBE_CANCEL);
    }

    SubscribeCancel::SubscribeCancel(const std::shared_ptr<const CTransaction>& tx) : Subscribe(tx)
    {
        SetType(TxType::ACTION_SUBSCRIBE_CANCEL);
    }

    shared_ptr <UniValue> SubscribeCancel::Serialize() const
    {
        auto result = Subscribe::Serialize();
        result->pushKV("unsubscribe", true);
        return result;
    }
} // namespace PocketTx
