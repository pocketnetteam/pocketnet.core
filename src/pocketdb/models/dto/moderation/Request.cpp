// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include "pocketdb/models/dto/moderation/Request.h"

namespace PocketTx
{
    ModerationRequest::ModerationRequest() : SocialTransaction()
    {
    }

    ModerationRequest::ModerationRequest(const CTransactionRef& tx) : SocialTransaction(tx)
    {
    }

    shared_ptr<string> ModerationRequest::GetDestionationAddress() const { return m_string2; }
    void ModerationRequest::SetDestionationAddress(const string& value) { m_string2 = make_shared<string>(value); }
    
    string ModerationRequest::BuildHash()
    {
        string data;

        data += GetDestionationAddress() ? *GetDestionationAddress() : "";

        return SocialTransaction::GenerateHash(data);
    }

} // namespace PocketTx
