// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETTX_MODERATION_VOTE_H
#define POCKETTX_MODERATION_VOTE_H

#include "pocketdb/models/base/SocialTransaction.h"

namespace PocketTx
{
    class ModerationVote : public SocialTransaction
    {
    public:
        ModerationVote();
        ModerationVote(const CTransactionRef& tx);

        string BuildHash() override;

        const optional<string>& GetJuryId() const;
        void SetJuryId(const string& value);
        
        const optional<int64_t>& GetVerdict() const;
        void SetVerdict(int64_t value);

    }; // class ModerationVote

} // namespace PocketTx

#endif // POCKETTX_MODERATION_VOTE_H
