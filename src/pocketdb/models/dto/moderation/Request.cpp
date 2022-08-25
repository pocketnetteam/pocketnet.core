// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include "pocketdb/models/dto/moderation/Request.h"

namespace PocketTx
{
    ModeratorRequest::ModeratorRequest() : SocialTransaction() {}

    ModeratorRequest::ModeratorRequest(const CTransactionRef& tx) : SocialTransaction(tx) {}

    shared_ptr<string> ModeratorRequest::GetDestionationAddress() const { return m_string2; }
    void ModeratorRequest::SetDestionationAddress(const string& value) { m_string2 = make_shared<string>(value); }
    
    string ModeratorRequest::BuildHash()
    {
        string data;

        data += GetDestionationAddress() ? *GetDestionationAddress() : "";

        return SocialTransaction::GenerateHash(data);
    }

} // namespace PocketTx

