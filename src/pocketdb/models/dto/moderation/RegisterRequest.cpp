// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include "pocketdb/models/dto/moderation/RegisterRequest.h"

namespace PocketTx
{
    ModeratorRegisterRequest::ModeratorRegisterRequest() : Moderator()
    {
        SetType(TxType::MODERATOR_REGISTER_REQUEST);
    }

    ModeratorRegisterRequest::ModeratorRegisterRequest(const CTransactionRef& tx) : Moderator(tx)
    {
        SetType(TxType::MODERATOR_REGISTER_REQUEST);
    }

    const optional<string>& ModeratorRegisterRequest::GetRequestTxHash() const { return m_string3; }
    void ModeratorRegisterRequest::SetRequestTxHash(const string& value) { m_string3 = value; }
    
    string ModeratorRegisterRequest::BuildHash()
    {
        string data;
        data += *GetModeratorAddress();
        data += *GetRequestTxHash();
        return SocialTransaction::GenerateHash(data);
    }
} // namespace PocketTx

