// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETTX_APP_H
#define POCKETTX_APP_H

#include "pocketdb/models/dto/content/Post.h"

namespace PocketTx
{
    using namespace std;

    class App : public Post
    {
    public:
        App();
        App(const CTransactionRef& tx);
    };

} // namespace PocketTx

#endif // POCKETTX_APP_H