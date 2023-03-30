// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include "pocketdb/models/dto/moderation/Vote.h"

namespace PocketTx
{
    ModerationVote::ModerationVote() : SocialTransaction()
    {
        SetType(TxType::MODERATION_VOTE);
    }

    ModerationVote::ModerationVote(const CTransactionRef& tx) : SocialTransaction(tx)
    {
        SetType(TxType::MODERATION_VOTE);
    }
    
    string ModerationVote::BuildHash()
    {
        string data;

        data += GetJuryId() ? *GetJuryId() : "";
        data += GetVerdict() ? to_string(*GetVerdict()) : "";

        return SocialTransaction::GenerateHash(data);
    }

    
    const optional<string>& ModerationVote::GetJuryId() const { return m_string2; }
    void ModerationVote::SetJuryId(const string& value) { m_string2 = value; }
    
    const optional<int64_t>& ModerationVote::GetVerdict() const { return m_int1; }
    void ModerationVote::SetVerdict(int64_t value) { m_int1 = value; }

} // namespace PocketTx

