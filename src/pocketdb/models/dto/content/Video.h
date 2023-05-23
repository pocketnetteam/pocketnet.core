// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETTX_VIDEO_H
#define POCKETTX_VIDEO_H

#include "pocketdb/models/dto/content/Post.h"

namespace PocketTx
{
    using namespace std;

    class Video : public Post
    {
    public:
        Video();
        Video(const CTransactionRef& tx);
    };

} // namespace PocketTx

#endif // POCKETTX_VIDEO_H