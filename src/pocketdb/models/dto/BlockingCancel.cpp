// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include <primitives/transaction.h>
#include "pocketdb/models/dto/BlockingCancel.h"

namespace PocketTx
{
    BlockingCancel::BlockingCancel(const string& hash, int64_t time) : Blocking(hash, time)
    {
        SetType(PocketTxType::ACTION_BLOCKING_CANCEL);
    }

    BlockingCancel::BlockingCancel(const std::shared_ptr<const CTransaction>& tx) : Blocking(tx)
    {
        SetType(PocketTxType::ACTION_BLOCKING_CANCEL);
    }

    shared_ptr <UniValue> BlockingCancel::Serialize() const
    {
        auto result = Blocking::Serialize();

        result->pushKV("unblocking", true);

        return result;
    }

} // namespace PocketTx