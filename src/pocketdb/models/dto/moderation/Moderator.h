// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETTX_MODERATION_MODERATOR_H
#define POCKETTX_MODERATION_MODERATOR_H

#include "pocketdb/models/base/SocialTransaction.h"

namespace PocketTx
{
    class Moderator : public SocialTransaction
    {
    public:
        Moderator();
        Moderator(const CTransactionRef& tx);

        string BuildHash() override;

        shared_ptr<string> GetModeratorAddress() const;
        void SetModeratorAddress(const string& value);

    }; // class Moderator

} // namespace PocketTx

#endif // POCKETTX_MODERATION_MODERATOR_H
