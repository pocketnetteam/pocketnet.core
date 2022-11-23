// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETTX_MODERATOR_REGISTER_REQUEST_H
#define POCKETTX_MODERATOR_REGISTER_REQUEST_H

#include "pocketdb/models/dto/moderation/Moderator.h"

namespace PocketTx
{
    class ModeratorRegisterRequest : public Moderator
    {
    public:
        ModeratorRegisterRequest();
        ModeratorRegisterRequest(const CTransactionRef& tx);

        const optional<string>& GetRequestTxHash() const;
        void SetRequestTxHash(const string& value);

        string BuildHash() override;
    }; // class ModeratorRegisterRequest

} // namespace PocketTx

#endif // POCKETTX_MODERATOR_REGISTER_REQUEST_H
