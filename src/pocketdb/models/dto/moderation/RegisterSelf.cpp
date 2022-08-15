// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include "pocketdb/models/dto/moderation/RegisterSelf.h"

namespace PocketTx
{
    ModerationRegisterSelf::ModerationRegisterSelf() : SocialTransaction()
    {
        SetType(TxType::MODERATION_REGISTER_SELF);
    }

    ModerationRegisterSelf::ModerationRegisterSelf(const CTransactionRef& tx) : SocialTransaction(tx)
    {
        SetType(TxType::MODERATION_REGISTER_SELF);
    }

} // namespace PocketTx

