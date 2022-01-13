// Copyright (c) 2018-2022 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETTX_VIDEO_H
#define POCKETTX_VIDEO_H

#include "pocketdb/models/dto/Content.h"

namespace PocketTx
{
    using namespace std;

    class Video : public Content
    {
    public:
        Video();
        Video(const CTransactionRef& tx);
        shared_ptr<UniValue> Serialize() const override;
    };

} // namespace PocketTx

#endif // POCKETTX_VIDEO_H