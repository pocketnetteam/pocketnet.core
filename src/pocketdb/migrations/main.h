#ifndef POCKETDB_MIGRATION_MAIN
#define POCKETDB_MIGRATION_MAIN

#include <string>
#include <vector>

#include "pocketdb/migrations/base.h"

namespace PocketDb
{
    class PocketDbMainMigration : public PocketDbMigration
    {
    public:
        PocketDbMainMigration();
    };
}

#endif // POCKETDB_MIGRATION_MAIN