// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETTX_MODERATOR_REQUEST_SUBS_H
#define POCKETTX_MODERATOR_REQUEST_SUBS_H

#include "pocketdb/models/dto/moderation/Moderator.h"

namespace PocketTx
{
    class ModeratorRequestSubs : public Moderator
    {
    public:
        ModeratorRequestSubs();
        ModeratorRequestSubs(const CTransactionRef& tx);
    }; // class ModeratorRequestSubs

} // namespace PocketTx

#endif // POCKETTX_MODERATOR_REQUEST_SUBS_H
