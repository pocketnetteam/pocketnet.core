// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETDB_MIGRATION_OLD_MINIMAL_H
#define POCKETDB_MIGRATION_OLD_MINIMAL_H

#include "pocketdb/migrations/base.h"

namespace PocketDb
{
    // Specify minimal required table schema for 0.21 db that will be used for 0.22 migration.
    // Useful for migration from 0.21 snapshot
    class PocketDbOldMinimalMigration : public PocketDbMigration
    {
    public:
        PocketDbOldMinimalMigration();
    };
}

#endif // POCKETDB_MIGRATION_OLD_MINIMAL_H
