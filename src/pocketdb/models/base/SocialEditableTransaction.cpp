// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include "pocketdb/models/base/SocialEditableTransaction.h"

namespace PocketTx
{
    SocialEditableTransaction::SocialEditableTransaction() : SocialTransaction()
    {
    }

    SocialEditableTransaction::SocialEditableTransaction(const CTransactionRef& tx) : SocialTransaction(tx)
    {
    }

    const optional<string>& SocialEditableTransaction::GetRootTxHash() const { return m_string2; }
    void SocialEditableTransaction::SetRootTxHash(const string& value) { m_string2 = value; }

    bool SocialEditableTransaction::IsEdit() const { return m_string2 != m_hash; }

    void SocialEditableTransaction::Deserialize(const UniValue& src)
    {
        SocialTransaction::Deserialize(src);

        if (!GetRootTxHash())
            SetRootTxHash(*GetHash());
    }

    string SocialEditableTransaction::BuildHash()
    {
        string data;

        data += GetAddress() ? *GetAddress() : "";
        data += GetRootTxHash() && *GetRootTxHash() != *GetHash() ? *GetRootTxHash() : "";
        data += GetString3() ? *GetString3() : "";
        data += GetString4() ? *GetString4() : "";
        data += GetString5() ? *GetString5() : "";
        data += GetInt1() ? to_string(*GetInt1()) : "";

        if (GetPayload())
        {
            data += GetPayload()->GetString1() ? *GetPayload()->GetString1() : "";
            data += GetPayload()->GetString2() ? *GetPayload()->GetString2() : "";
            data += GetPayload()->GetString3() ? *GetPayload()->GetString3() : "";
            data += GetPayload()->GetString4() ? *GetPayload()->GetString4() : "";
            data += GetPayload()->GetString5() ? *GetPayload()->GetString5() : "";
            data += GetPayload()->GetString6() ? *GetPayload()->GetString6() : "";
            data += GetPayload()->GetString7() ? *GetPayload()->GetString7() : "";
            data += GetPayload()->GetInt1() ? to_string(*GetPayload()->GetInt1()) : "";
        }

        return SocialTransaction::GenerateHash(data);
    }

    size_t SocialEditableTransaction::PayloadSize() const
    {
        auto sz = SocialTransaction::PayloadSize();
        return sz - (GetString2() ? GetString2()->size() : 0);
    }

} // namespace PocketTx

