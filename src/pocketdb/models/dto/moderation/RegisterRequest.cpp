// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include "pocketdb/models/dto/moderation/RegisterRequest.h"

namespace PocketTx
{
    ModeratorRegisterRequest::ModeratorRegisterRequest() : ModeratorRegister()
    {
        SetType(TxType::MODERATOR_REGISTER_REQUEST);
    }

    ModeratorRegisterRequest::ModeratorRegisterRequest(const CTransactionRef& tx) : ModeratorRegister(tx)
    {
        SetType(TxType::MODERATOR_REGISTER_REQUEST);
    }
    
    string ModeratorRegisterRequest::BuildHash()
    {
        string data;

        data += GetRequestId() ? *GetRequestId() : "";

        return SocialTransaction::GenerateHash(data);
    }

} // namespace PocketTx

