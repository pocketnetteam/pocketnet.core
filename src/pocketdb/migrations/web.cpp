// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include "pocketdb/migrations/web.h"

namespace PocketDb
{
    PocketDbWebMigration::PocketDbWebMigration() : PocketDbMigration()
    {
        _tables.emplace_back(R"sql(
            create table if not exists System
            (
                Key text primary key,
                Value int not null default 0
            );
        )sql");

        _tables.emplace_back(R"sql(
            create table if not exists Tags
            (
              Id    integer primary key,
              Lang  text not null,
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
            create table if not exists ContentMap
            (
                ContentId int not null,
                FieldType int not null,
                primary key (ContentId, FieldType)
            );
        )sql");

        _tables.emplace_back(R"sql(
            create virtual table if not exists Content using fts5
            (
                Value
            );
        )sql");

        _tables.emplace_back(R"sql(
            drop table if exists Badges;
        )sql");

        //
        // BARTERON
        //
        
        _tables.emplace_back(R"sql(
            create table if not exists BarteronAccountTags
            (
                AccountId  int not null,
                Tag        int not null
            );
        )sql");

        _tables.emplace_back(R"sql(
            create table if not exists BarteronOffers
            (
                AccountId  int not null,
                OfferId    int not null,
                Tag        int not null
            );
        )sql");

        //
        // INDEXES
        //
        _indexes = R"sql(
            create unique index if not exists Tags_Lang_Value on Tags (Lang, Value);
            create index if not exists Tags_Lang_Id on Tags (Lang, Id);
            create index if not exists Tags_Lang_Value_Id on Tags (Lang, Value, Id);
            create index if not exists TagsMap_TagId_ContentId on TagsMap (TagId, ContentId);

            create index if not exists BarteronAccountTags_Tag_AccountId on BarteronAccountTags (Tag, AccountId);
            create index if not exists BarteronOffers_Tag_OfferId_AccountId on BarteronOffers (Tag, OfferId, AccountId);
        )sql";
    }
}