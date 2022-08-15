// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETTX_MODERATION_REGISTER_SUBS_H
#define POCKETTX_MODERATION_REGISTER_SUBS_H

#include "pocketdb/models/dto/moderation/Register.h"

namespace PocketTx
{
    class ModerationRegisterSubs : public ModerationRegister
    {
    public:
        ModerationRegisterSubs();
        ModerationRegisterSubs(const CTransactionRef& tx);
    }; // class ModerationRegisterSubs

} // namespace PocketTx

#endif // POCKETTX_MODERATION_REGISTER_SUBS_H
