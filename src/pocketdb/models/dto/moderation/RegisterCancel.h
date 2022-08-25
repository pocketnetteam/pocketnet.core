// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETTX_MODERATION_REGISTER_CANCEL_H
#define POCKETTX_MODERATION_REGISTER_CANCEL_H

#include "pocketdb/models/dto/moderation/Register.h"

namespace PocketTx
{
    class ModeratorRegisterCancel : public ModeratorRegister
    {
    public:
        ModeratorRegisterCancel();
        ModeratorRegisterCancel(const CTransactionRef& tx);

        string BuildHash() override;

        shared_ptr<string> GetDestionationAddress() const;
        void SetDestionationAddress(const string& value);

    }; // class ModeratorRegisterCancel

} // namespace PocketTx

#endif // POCKETTX_MODERATION_REGISTER_CANCEL_H
