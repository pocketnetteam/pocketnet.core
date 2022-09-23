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

        string BuildHash() override;

        const optional<string>& GetContentTxHash() const;
        void SetContentTxHash(const string& value);
        
        const optional<string>& GetContentAddressHash() const;
        void SetContentAddressHash(const string& value);

        const optional<int64_t>& GetReason() const;
        void SetReason(int64_t value);

    }; // class ModerationFlag

} // namespace PocketTx

#endif // POCKETTX_MODERATION_FLAG_H
