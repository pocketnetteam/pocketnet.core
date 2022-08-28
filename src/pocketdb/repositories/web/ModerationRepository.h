// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef SRC_RPC_MODERATION_REPOSITORY_H
#define SRC_RPC_MODERATION_REPOSITORY_H

#include "pocketdb/repositories/BaseRepository.h"

namespace PocketDb
{
    class RpcModerationRepository : public BaseRepository
    {
    public:
        explicit RpcModerationRepository(SQLiteDatabase& db) : BaseRepository(db) {}

        void Init() override;
        void Destroy() override;

    };
}

#endif //SRC_RPC_MODERATION_REPOSITORY_H
