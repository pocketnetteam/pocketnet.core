// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETTX_COINSTAKE_H
#define POCKETTX_COINSTAKE_H

#include "pocketdb/models/dto/money/Default.h"

namespace PocketTx
{
    class Coinstake : public Default
    {
    public:
        Coinstake();
        Coinstake(const std::shared_ptr<const CTransaction>& tx);
    };
} // namespace PocketTx

#endif // POCKETTX_COINSTAKE_H
