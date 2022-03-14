// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include "pocketdb/models/dto/moderation/ModeratorRegister.h"

namespace PocketTx
{
    ModeratorRegister::ModeratorRegister() : SocialTransaction()
    {
        SetType(TxType::MODERATOR_REGISTER);
    }

    ModeratorRegister::ModeratorRegister(const CTransactionRef& tx) : SocialTransaction(tx)
    {
        SetType(TxType::MODERATOR_REGISTER);
    }

    shared_ptr<UniValue> ModeratorRegister::Serialize() const
    {
        auto result = SocialTransaction::Serialize();

        // TODO (brangr): tx data & payload

        // result->pushKV("data", (m_payload && m_payload->GetString1()) ? *m_payload->GetString1() : "");

        return result;
    }

    void ModeratorRegister::Deserialize(const UniValue& src)
    {
        SocialTransaction::Deserialize(src);
        // TODO (brangr): tx data
    }
    
    void ModeratorRegister::DeserializePayload(const UniValue& src)
    {
        SocialTransaction::DeserializePayload(src);
        // if (auto[ok, val] = TryGetStr(src, "data"); ok) m_payload->SetString1(val);
        // TODO (brangr): payload
    }

    void ModeratorRegister::DeserializeRpc(const UniValue& src)
    {
        // TODO (brangr): rpc tx data & payload
        // GeneratePayload();
        // if (auto[ok, val] = TryGetStr(src, "d"); ok) m_payload->SetString1(val);
    }

    

    string ModeratorRegister::BuildHash()
    {
        string data;

        // data += m_payload && m_payload->GetString1() ? *m_payload->GetString1() : "";
        // TODO (brangr): tx data & payload

        return SocialTransaction::GenerateHash(data);
    }

} // namespace PocketTx

