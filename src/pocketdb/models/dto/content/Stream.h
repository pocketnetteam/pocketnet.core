// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETTX_STREAM_H
#define POCKETTX_STREAM_H

#include "pocketdb/models/dto/content/Post.h"

namespace PocketTx
{
    using namespace std;

    class Stream : public Post
    {
    public:
        Stream();
        Stream(const CTransactionRef& tx);
    };

} // namespace PocketTx

#endif // POCKETTX_STREAM_H