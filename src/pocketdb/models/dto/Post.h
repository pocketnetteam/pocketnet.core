// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETTX_POST_H
#define POCKETTX_POST_H

#include "pocketdb/models/dto/Content.h"

namespace PocketTx
{
    using namespace std;

    class Post : public Content
    {
    public:
        Post();
        Post(const CTransactionRef& tx);
        shared_ptr<UniValue> Serialize() const override;
    };

} // namespace PocketTx

#endif // POCKETTX_POST_H