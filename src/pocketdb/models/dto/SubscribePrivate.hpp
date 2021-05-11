// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 Bitcoin developers
// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETTX_SUBSCRIBE_PRIVATE_HPP
#define POCKETTX_SUBSCRIBE_PRIVATE_HPP

#include "pocketdb/models/dto/Subscribe.hpp"

namespace PocketTx
{

    class SubscribePrivate : public Subscribe
    {
    public:

        SubscribePrivate() : Subscribe()
        {
            SetType(PocketTxType::ACTION_SUBSCRIBE_PRIVATE);
        }
    };

} // namespace PocketTx

#endif //POCKETTX_SUBSCRIBE_PRIVATE_HPP
