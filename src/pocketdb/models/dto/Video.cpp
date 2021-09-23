// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include <primitives/transaction.h>
#include "pocketdb/models/dto/Video.h"

namespace PocketTx
{
    Video::Video() : Post()
    {
        SetType(PocketTxType::CONTENT_VIDEO);
    }

    Video::Video(const std::shared_ptr<const CTransaction>& tx) : Post(tx)
    {
        SetType(PocketTxType::CONTENT_VIDEO);
    }

    shared_ptr <UniValue> Video::Serialize() const
    {
        auto result = Post::Serialize();

        result->pushKV("type", 1);

        return result;
    }

} // namespace PocketTx