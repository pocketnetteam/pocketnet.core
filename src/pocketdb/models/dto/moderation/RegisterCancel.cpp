// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include "pocketdb/models/dto/moderation/RegisterCancel.h"

namespace PocketTx
{
    ModerationRegisterCancel::ModerationRegisterCancel() : ModerationRegister()
    {
        SetType(TxType::MODERATION_REGISTER_CANCEL);
    }

    ModerationRegisterCancel::ModerationRegisterCancel(const CTransactionRef& tx) : ModerationRegister(tx)
    {
        SetType(TxType::MODERATION_REGISTER_CANCEL);
    }
    
    string ModerationRegisterCancel::BuildHash()
    {
        string data;

        // TODO (brangr): implement

        return SocialTransaction::GenerateHash(data);
    }

    // TODO (brangr): implement

} // namespace PocketTx

