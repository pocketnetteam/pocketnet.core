// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 Bitcoin developers
// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETTX_BLOCKING_CANCEL_HPP
#define POCKETTX_BLOCKING_CANCEL_HPP

#include "pocketdb/models/dto/Blocking.hpp"

namespace PocketTx
{

    class BlockingCancel : public Blocking
    {
    public:

        BlockingCancel(string& hash, int64_t time) : Blocking(hash, time)
        {
            SetType(PocketTxType::ACTION_BLOCKING_CANCEL);
        }
    };

} // namespace PocketTx

#endif // POCKETTX_BLOCKING_CANCEL_HPP
