// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETTX_EMPTY_H
#define POCKETTX_EMPTY_H

#include "pocketdb/models/dto/Default.h"

namespace PocketTx
{

    class Empty : public Default
    {
    public:
        Empty();
        Empty(const shared_ptr<const CTransaction>& tx);
    };

} // namespace PocketTx

#endif // POCKETTX_EMPTY_H
