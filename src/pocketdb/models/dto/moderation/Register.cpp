// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include "pocketdb/models/dto/moderation/Register.h"

namespace PocketTx
{
    ModeratorRegister::ModeratorRegister() : SocialTransaction()
    {
        SetType(TxType::MODERATOR_REGISTER);
    }

    ModeratorRegister::ModeratorRegister(const CTransactionRef& tx) : SocialTransaction(tx)
    {
        SetType(TxType::MODERATOR_REGISTER);
    }
    
    string ModeratorRegister::BuildHash()
    {
        string data;

        data += GetRequestId() ? *GetRequestId() : "";

        return SocialTransaction::GenerateHash(data);
    }

} // namespace PocketTx

