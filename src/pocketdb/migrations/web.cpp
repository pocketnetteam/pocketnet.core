#include "pocketdb/migrations/web.h"

namespace PocketDb
{
    PocketDbWebMigration::PocketDbWebMigration() : PocketDbMigration()
    {
        _tables.emplace_back(R"sql(
            create table if not exists Tags
            (
              Id    integer primary key,
              Value text not null
            );
        )sql");

        _tables.emplace_back(R"sql(
            create table if not exists TagsMap
            (
              ContentId   int not null,
              TagId       int not null,
              primary key (ContentId, TagId)
            );
        )sql");

        _tables.emplace_back(R"sql(
            create virtual table if not exists Content using fts5
            (
                ContentId UNINDEXED,
                FieldType UNINDEXED,
                Value
            );
        )sql");

        _indexes = R"sql(
            create unique index if not exists Tags_Value on Tags (Value);
            create index if not exists TagsMap_TagId_ContentId on TagsMap (TagId, ContentId);
        )sql";
    }
}