// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETTX_MODERATOR_REQUEST_COIN_H
#define POCKETTX_MODERATOR_REQUEST_COIN_H

#include "pocketdb/models/dto/moderation/Moderator.h"

namespace PocketTx
{
    class ModeratorRequestCoin : public Moderator
    {
    public:
        ModeratorRequestCoin();
        ModeratorRequestCoin(const CTransactionRef& tx);
    }; // class ModeratorRequestCoin

} // namespace PocketTx

#endif // POCKETTX_MODERATOR_REQUEST_COIN_H
