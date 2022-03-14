// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include "pocketdb/models/dto/moderation/ModerationFlag.h"

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

    shared_ptr<UniValue> ModerationFlag::Serialize() const
    {
        auto result = SocialTransaction::Serialize();

        // TODO (brangr): tx data & payload

        // result->pushKV("data", (m_payload && m_payload->GetString1()) ? *m_payload->GetString1() : "");

        return result;
    }

    void ModerationFlag::Deserialize(const UniValue& src)
    {
        SocialTransaction::Deserialize(src);
        // TODO (brangr): tx data
    }
    
    void ModerationFlag::DeserializePayload(const UniValue& src)
    {
        SocialTransaction::DeserializePayload(src);
        // if (auto[ok, val] = TryGetStr(src, "data"); ok) m_payload->SetString1(val);
        // TODO (brangr): payload
    }

    void ModerationFlag::DeserializeRpc(const UniValue& src)
    {
        // TODO (brangr): rpc tx data & payload
        // GeneratePayload();
        // if (auto[ok, val] = TryGetStr(src, "d"); ok) m_payload->SetString1(val);
    }

    

    string ModerationFlag::BuildHash()
    {
        string data;

        // data += m_payload && m_payload->GetString1() ? *m_payload->GetString1() : "";
        // TODO (brangr): tx data & payload

        return SocialTransaction::GenerateHash(data);
    }

} // namespace PocketTx

