// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include "pocketdb/models/dto/moderation/RequestCancel.h"

namespace PocketTx
{
    ModerationRequestCancel::ModerationRequestCancel() : ModerationRequest()
    {
        SetType(TxType::MODERATOR_REQUEST_CANCEL);
    }

    ModerationRequestCancel::ModerationRequestCancel(const CTransactionRef& tx) : ModerationRequest(tx)
    {
        SetType(TxType::MODERATOR_REQUEST_CANCEL);
    }
    
} // namespace PocketTx

