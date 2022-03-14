// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETTX_MODERATION_FLAG_H
#define POCKETTX_MODERATION_FLAG_H

#include "pocketdb/models/base/SocialTransaction.h"

namespace PocketTx
{
    class ModerationFlag : public SocialTransaction
    {
    public:
        ModerationFlag();
        ModerationFlag(const CTransactionRef& tx);

        shared_ptr<UniValue> Serialize() const override;
        void Deserialize(const UniValue& src) override;
        void DeserializeRpc(const UniValue& src) override;
        void DeserializePayload(const UniValue& src) override;

        string BuildHash() override;

    }; // class ModerationFlag

} // namespace PocketTx

#endif // POCKETTX_MODERATION_FLAG_H
