// Copyright (c) 2018-2022 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include <primitives/transaction.h>
#include "pocketdb/models/dto/Post.h"

namespace PocketTx
{
    Post::Post() : Content()
    {
        SetType(TxType::CONTENT_POST);
    }

    Post::Post(const CTransactionRef& tx) : Content(tx)
    {
        SetType(TxType::CONTENT_POST);
    }

    shared_ptr<UniValue> Post::Serialize() const
    {
        auto result = Content::Serialize();

        result->pushKV("type", 0);

        return result;
    }

} // namespace PocketTx