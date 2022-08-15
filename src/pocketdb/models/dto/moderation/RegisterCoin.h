// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETTX_MODERATION_REGISTER_COIN_H
#define POCKETTX_MODERATION_REGISTER_COIN_H

#include "pocketdb/models/dto/moderation/Register.h"

namespace PocketTx
{
    class ModerationRegisterCoin : public ModerationRegister
    {
    public:
        ModerationRegisterCoin();
        ModerationRegisterCoin(const CTransactionRef& tx);
    }; // class ModerationRegisterCoin

} // namespace PocketTx

#endif // POCKETTX_MODERATION_REGISTER_COIN_H
