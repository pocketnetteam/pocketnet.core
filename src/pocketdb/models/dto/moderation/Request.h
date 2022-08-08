// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETTX_MODERATION_REQUEST_H
#define POCKETTX_MODERATION_REQUEST_H

#include "pocketdb/models/base/SocialTransaction.h"

namespace PocketTx
{
    class ModerationRequest : public SocialTransaction
    {
    public:
        ModerationRequest();
        ModerationRequest(const CTransactionRef& tx);

        string BuildHash() override;

        shared_ptr<string> GetDestionationAddress() const;
        void SetDestionationAddress(const string& value);

    }; // class ModerationRequest

} // namespace PocketTx

#endif // POCKETTX_MODERATION_REQUEST_H
