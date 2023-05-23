// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include <primitives/transaction.h>
#include "pocketdb/models/dto/content/Article.h"

namespace PocketTx
{
    Article::Article() : Post()
    {
        SetType(TxType::CONTENT_ARTICLE);
    }

    Article::Article(const CTransactionRef& tx) : Post(tx)
    {
        SetType(TxType::CONTENT_ARTICLE);
    }

} // namespace PocketTx