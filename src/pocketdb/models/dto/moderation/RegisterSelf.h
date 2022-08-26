// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETTX_MODERATION_REGISTER_SELF_H
#define POCKETTX_MODERATION_REGISTER_SELF_H

#include "pocketdb/models/dto/moderation/Register.h"

namespace PocketTx
{
    class ModeratorRegister : public SocialTransaction
    {
    public:
        ModeratorRegister();
        ModeratorRegister(const CTransactionRef& tx);
    }; // class ModeratorRegister

} // namespace PocketTx

#endif // POCKETTX_MODERATION_REGISTER_SELF_H
