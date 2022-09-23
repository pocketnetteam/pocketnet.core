// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include "pocketdb/models/dto/moderation/Flag.h"

namespace PocketTx
{
    ModerationFlag::ModerationFlag() : SocialTransaction()
    {
        SetType(TxType::MODERATION_FLAG);
    }

    ModerationFlag::ModerationFlag(const CTransactionRef& tx) : SocialTransaction(tx)
    {
        SetType(TxType::MODERATION_FLAG);
    }
    
    string ModerationFlag::BuildHash()
    {
        string data;

        data += GetContentTxHash() ? *GetContentTxHash() : "";
        data += GetContentAddressHash() ? *GetContentAddressHash() : "";
        data += GetReason() ? to_string(*GetReason()) : "";

        return SocialTransaction::GenerateHash(data);
    }

    
    const optional<string>& ModerationFlag::GetContentTxHash() const { return m_string2; }
    void ModerationFlag::SetContentTxHash(const string& value) { m_string2 = value; }
    
    const optional<string>& ModerationFlag::GetContentAddressHash() const { return m_string3; }
    void ModerationFlag::SetContentAddressHash(const string& value) { m_string3 = value; }
    
    const optional<int64_t>& ModerationFlag::GetReason() const { return m_int1; }
    void ModerationFlag::SetReason(int64_t value) { m_int1 = value; }


} // namespace PocketTx

