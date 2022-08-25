// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include "pocketdb/models/dto/moderation/RegisterCancel.h"

namespace PocketTx
{
    ModeratorRegisterCancel::ModeratorRegisterCancel() : ModeratorRegister()
    {
        SetType(TxType::MODERATOR_REGISTER_CANCEL);
    }

    ModeratorRegisterCancel::ModeratorRegisterCancel(const CTransactionRef& tx) : ModeratorRegister(tx)
    {
        SetType(TxType::MODERATOR_REGISTER_CANCEL);
    }
    
    shared_ptr<string> ModeratorRegisterCancel::GetDestionationAddress() const { return m_string2; }
    void ModeratorRegisterCancel::SetDestionationAddress(const string& value) { m_string2 = make_shared<string>(value); }
    
    string ModeratorRegisterCancel::BuildHash()
    {
        string data;

        data += GetDestionationAddress() ? *GetDestionationAddress() : "";

        return SocialTransaction::GenerateHash(data);
    }

} // namespace PocketTx

