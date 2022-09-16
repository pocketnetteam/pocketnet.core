// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETDB_SHORTTXTYPE_H
#define POCKETDB_SHORTTXTYPE_H

namespace PocketDb
{
    enum class ShortTxType
    {
        NotSet,
        Money,
        Referal,
        Answer,
        Comment,
        Subscriber,
        CommentScore,
        ContentScore,
        PrivateContent,
        Boost,
        Repost,
        Blocking
    };
}

#endif // POCKETDB_SHORTTXTYPE_H