// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include "pocketdb/models/dto/moderation/RegisterCancel.h"

namespace PocketTx
{
    ModeratorRegisterCancel::ModeratorRegisterCancel() : Moderator()
    {
        SetType(TxType::MODERATOR_REGISTER_CANCEL);
    }

    ModeratorRegisterCancel::ModeratorRegisterCancel(const CTransactionRef& tx) : Moderator(tx)
    {
        SetType(TxType::MODERATOR_REGISTER_CANCEL);
    }
} // namespace PocketTx

