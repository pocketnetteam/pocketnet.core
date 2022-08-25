// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include "pocketdb/models/dto/moderation/RegisterCancel.h"

namespace PocketTx
{
    ModerationRegisterCancel::ModerationRegisterCancel() : ModerationRegister()
    {
        SetType(TxType::MODERATION_REGISTER_CANCEL);
    }

    ModerationRegisterCancel::ModerationRegisterCancel(const CTransactionRef& tx) : ModerationRegister(tx)
    {
        SetType(TxType::MODERATION_REGISTER_CANCEL);
    }
    
    shared_ptr<string> ModerationRegisterCancel::GetDestionationAddress() const { return m_string2; }
    void ModerationRegisterCancel::SetDestionationAddress(const string& value) { m_string2 = make_shared<string>(value); }
    
    string ModerationRegisterCancel::BuildHash()
    {
        string data;

        data += GetDestionationAddress() ? *GetDestionationAddress() : "";

        return SocialTransaction::GenerateHash(data);
    }

} // namespace PocketTx

