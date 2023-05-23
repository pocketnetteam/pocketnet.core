// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETTX_AUDIO_H
#define POCKETTX_AUDIO_H

#include "pocketdb/models/dto/content/Post.h"

namespace PocketTx
{
    using namespace std;

    class Audio : public Post
    {
    public:
        Audio();
        Audio(const CTransactionRef& tx);
        size_t PayloadSize() const override;
    };

} // namespace PocketTx

#endif // POCKETTX_AUDIO_H