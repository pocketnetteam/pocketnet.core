// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include "pocketdb/models/dto/moderation/RequestCancel.h"

namespace PocketTx
{
    ModeratorRequestCancel::ModeratorRequestCancel() : Moderator()
    {
        SetType(TxType::MODERATOR_REQUEST_CANCEL);
    }

    ModeratorRequestCancel::ModeratorRequestCancel(const CTransactionRef& tx) : Moderator(tx)
    {
        SetType(TxType::MODERATOR_REQUEST_CANCEL);
    }

    const optional<string>& ModeratorRequestCancel::GetRequestTxHash() const { return m_string3; }
    void ModeratorRequestCancel::SetRequestTxHash(const string& value) { m_string3 = value; }
    
    string ModeratorRequestCancel::BuildHash()
    {
        string data;
        data += *GetModeratorAddress();
        data += *GetRequestTxHash();
        return SocialTransaction::GenerateHash(data);
    }
    
} // namespace PocketTx

