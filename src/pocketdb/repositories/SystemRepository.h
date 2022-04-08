// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef SRC_SYSTEM_REPOSITORY_H
#define SRC_SYSTEM_REPOSITORY_H

#include <util/system.h>
#include "pocketdb/repositories/BaseRepository.h"

namespace PocketDb
{
    class SystemRepository : public BaseRepository
    {
    public:
        explicit SystemRepository(SQLiteDatabase& db) : BaseRepository(db) {}

        void Init() override {}
        void Destroy() override {}

        int GetDbVersion(const string& db);
        void SetDbVersion(const string& db, int version);

    }; // namespace PocketDb
}
#endif //SRC_SYSTEM_REPOSITORY_H
