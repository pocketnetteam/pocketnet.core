// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETTX_MODERATION_REQUEST_COIN_H
#define POCKETTX_MODERATION_REQUEST_COIN_H

#include "pocketdb/models/dto/moderation/Request.h"

namespace PocketTx
{
    class ModeratorRequestCoin : public ModeratorRequest
    {
    public:
        ModeratorRequestCoin();
        ModeratorRequestCoin(const CTransactionRef& tx);
    }; // class ModeratorRequestCoin

} // namespace PocketTx

#endif // POCKETTX_MODERATION_REQUEST_COIN_H
