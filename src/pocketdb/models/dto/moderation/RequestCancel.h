// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETTX_MODERATOR_REQUEST_CANCEL_H
#define POCKETTX_MODERATOR_REQUEST_CANCEL_H

#include "pocketdb/models/dto/moderation/Moderator.h"

namespace PocketTx
{
    class ModeratorRequestCancel : public Moderator
    {
    public:
        ModeratorRequestCancel();
        ModeratorRequestCancel(const CTransactionRef& tx);

        shared_ptr<string> GetRequestTxHash() const;
        void SetRequestTxHash(const string& value);

        string BuildHash() override;
    }; // class ModeratorRequestCancel

} // namespace PocketTx

#endif // POCKETTX_MODERATOR_REQUEST_CANCEL_H
