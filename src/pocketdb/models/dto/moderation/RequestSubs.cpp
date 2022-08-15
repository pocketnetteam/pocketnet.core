// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include "pocketdb/models/dto/moderation/RequestSubs.h"

namespace PocketTx
{
    ModerationRequestSubs::ModerationRequestSubs() : SocialTransaction()
    {
        SetType(TxType::MODERATOR_REQUEST_SUBS);
    }

    ModerationRequestSubs::ModerationRequestSubs(const CTransactionRef& tx) : SocialTransaction(tx)
    {
        SetType(TxType::MODERATOR_REQUEST_SUBS);
    }
} // namespace PocketTx

