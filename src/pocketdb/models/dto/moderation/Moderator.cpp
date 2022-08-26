// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include "pocketdb/models/dto/moderation/Moderator.h"

namespace PocketTx
{
    Moderator::Moderator() : SocialTransaction() {}

    Moderator::Moderator(const CTransactionRef& tx) : SocialTransaction(tx) {}

    shared_ptr<string> Moderator::GetModeratorAddress() const { return m_string2; }
    void Moderator::SetModeratorAddress(const string& value) { m_string2 = make_shared<string>(value); }
    
    string Moderator::BuildHash()
    {
        string data;

        data += GetModeratorAddress() ? *GetModeratorAddress() : "";

        return SocialTransaction::GenerateHash(data);
    }

} // namespace PocketTx

