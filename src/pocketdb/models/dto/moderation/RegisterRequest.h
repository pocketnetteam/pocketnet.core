// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETTX_MODERATION_REGISTER_REQUEST_H
#define POCKETTX_MODERATION_REGISTER_REQUEST_H

#include "pocketdb/models/dto/moderation/Register.h"

namespace PocketTx
{
    class ModeratorRegisterRequest : public ModeratorRegister
    {
    public:
        ModeratorRegisterRequest();
        ModeratorRegisterRequest(const CTransactionRef& tx);

        shared_ptr<string> GetRequestId() const;
        void SetRequestId(const string& value);

        string BuildHash() override;

    }; // class ModeratorRegisterRequest

} // namespace PocketTx

#endif // POCKETTX_MODERATION_REGISTER_REQUEST_H
