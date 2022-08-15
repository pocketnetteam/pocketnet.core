// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETTX_MODERATION_REQUEST_SUBS_H
#define POCKETTX_MODERATION_REQUEST_SUBS_H

#include "pocketdb/models/dto/moderation/Request.h"

namespace PocketTx
{
    class ModerationRequestSubs : public ModerationRequest
    {
    public:
        ModerationRequestSubs();
        ModerationRequestSubs(const CTransactionRef& tx);
    }; // class ModerationRequestSubs

} // namespace PocketTx

#endif // POCKETTX_MODERATION_REQUEST_SUBS_H
