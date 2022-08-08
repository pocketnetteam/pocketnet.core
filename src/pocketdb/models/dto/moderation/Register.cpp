// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include "pocketdb/models/dto/moderation/Register.h"

namespace PocketTx
{
    ModerationRegister::ModerationRegister() : SocialTransaction()
    {
        SetType(TxType::MODERATION_FLAG);
    }

    ModerationRegister::ModerationRegister(const CTransactionRef& tx) : SocialTransaction(tx)
    {
        SetType(TxType::MODERATION_FLAG);
    }
    
    string ModerationRegister::BuildHash()
    {
        string data;

        // TODO (brangr): implement

        return SocialTransaction::GenerateHash(data);
    }

    // TODO (brangr): implement

} // namespace PocketTx

