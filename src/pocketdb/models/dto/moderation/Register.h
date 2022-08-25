// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETTX_MODERATION_REGISTER_H
#define POCKETTX_MODERATION_REGISTER_H

#include "pocketdb/models/base/SocialTransaction.h"

namespace PocketTx
{
    class ModerationRegister : public SocialTransaction
    {
    public:
        ModerationRegister();
        ModerationRegister(const CTransactionRef& tx);

        shared_ptr<string> GetRequestId() const;
        void SetRequestId(const string& value);

        string BuildHash() override;

    }; // class ModerationRegister

} // namespace PocketTx

#endif // POCKETTX_MODERATION_REGISTER_H
