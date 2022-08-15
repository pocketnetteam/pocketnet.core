// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETTX_MODERATION_REQUEST_COIN_H
#define POCKETTX_MODERATION_REQUEST_COIN_H

#include "pocketdb/models/dto/moderation/Request.h"

namespace PocketTx
{
    class ModerationRequestCoin : public ModerationRequest
    {
    public:
        ModerationRequestCoin();
        ModerationRequestCoin(const CTransactionRef& tx);
    }; // class ModerationRequestCoin

} // namespace PocketTx

#endif // POCKETTX_MODERATION_REQUEST_COIN_H
