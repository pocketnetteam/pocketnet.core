#ifndef POCKETDB_MIGRATION_MAIN
#define POCKETDB_MIGRATION_MAIN

#include <string>
#include <vector>

namespace PocketDb
{
    class PocketDbMigration
    {

    public:

        PocketDbMigration();

        std::vector<std::string> Tables;
        std::vector<std::string> Views;
        std::string Indexes;

    };
}

#endif // POCKETDB_MIGRATION_MAIN