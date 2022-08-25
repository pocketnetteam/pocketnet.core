// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include "pocketdb/models/dto/moderation/RequestCoin.h"

namespace PocketTx
{
    ModeratorRequestCoin::ModeratorRequestCoin() : ModeratorRequest()
    {
        SetType(TxType::MODERATOR_REQUEST_COIN);
    }

    ModeratorRequestCoin::ModeratorRequestCoin(const CTransactionRef& tx) : ModeratorRequest(tx)
    {
        SetType(TxType::MODERATOR_REQUEST_COIN);
    }
} // namespace PocketTx

