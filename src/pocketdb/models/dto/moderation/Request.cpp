// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include "pocketdb/models/dto/moderation/Request.h"

namespace PocketTx
{
    ModerationRequest::ModerationRequest() : SocialTransaction()
    {
        SetType(TxType::MODERATOR_REQUEST);
    }

    ModerationRequest::ModerationRequest(const CTransactionRef& tx) : SocialTransaction(tx)
    {
        SetType(TxType::MODERATOR_REQUEST);
    }
    
    string ModerationRequest::BuildHash()
    {
        string data;

        data += GetDestionationAddress() ? *GetDestionationAddress() : "";

        return SocialTransaction::GenerateHash(data);
    }

    shared_ptr<string> SocialTransaction::GetDestionationAddress() const { return m_string2; }
    void SocialTransaction::SetDestionationAddress(const string& value) { m_string2 = make_shared<string>(value); }

} // namespace PocketTx

