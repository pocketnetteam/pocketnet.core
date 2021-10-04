#include "pocketdb/migrations/web.h"

namespace PocketDb
{
    PocketDbWebMigration::PocketDbWebMigration() : PocketDbMigration()
    {
        _tables.emplace_back(R"sql(
            create table if not exists Tags
            (
              Type  int not null,
              Id    int not null,
              Value text not null,
              primary key (Value, Id, Type)
            );
        )sql");

        _indexes = R"sql(
            create index if not exists Tags_Id on Tags (Id);
        )sql";
    }
}