// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include "pocketdb/migrations/main.h"

namespace PocketDb
{
    PocketDbMainMigration::PocketDbMainMigration() : PocketDbMigration()
    {
        _tables.emplace_back(R"sql(
            create table if not exists Chain
            (
                TxId     integer primary key, -- Transactions.Id
                BlockId  int     not null,
                BlockNum int     not null,
                Height   int     not null,

                -- AccountUser.Id
                -- ContentPost.Id
                -- ContentVideo.Id
                -- Comment.Id
                Uid       int     null
            );
        )sql");


        _tables.emplace_back(R"sql(
            create table if not exists Last
            (
                -- AccountUser
                -- ContentPost
                -- ContentVideo
                -- ContentDelete
                -- Comment
                -- Blocking
                -- Subscribe
                TxId integer primary key
            );
        )sql");

        // TODO (losty-db): create table First

        _tables.emplace_back(R"sql(
            create table if not exists Transactions
            (
                RowId     integer primary key,
                Type      int    not null,
                HashId    int    not null, -- Id of tx hash in Registry table
                Time      int    not null,

                -- AccountUser.AddressId
                -- ContentPost.AddressId
                -- ContentVideo.AddressId
                -- ContentDelete.AddressId
                -- Comment.AddressId
                -- ScorePost.AddressId
                -- ScoreComment.AddressId
                -- Subscribe.AddressId
                -- Blocking.AddressId
                -- Complain.AddressId
                -- Boost.AddressId
                RegId1   int   null,

                -- AccountUser.ReferrerAddressId
                -- ContentPost.RootTxId
                -- ContentVideo.RootTxId
                -- ContentDelete.RootTxId
                -- Comment.RootTxId
                -- ScorePost.ContentRootTxId
                -- ScoreComment.CommentRootTxId
                -- Subscribe.AddressToId
                -- Blocking.AddressToId
                -- Complain.ContentRootTxId
                -- Boost.ContentRootTxId
                -- ModerationFlag.ContentTxId
                RegId2   int   null,

                -- ContentPost.RelayRootTxId
                -- ContentVideo.RelayRootTxId
                -- Comment.ContentRootTxId
                RegId3   int   null,

                -- Comment.ParentRootTxId
                RegId4   int   null,

                -- Comment.AnswerRootTxId
                RegId5   int   null,

                -- ScoreContent.Value
                -- ScoreComment.Value
                -- Complain.Reason
                -- Boost.Amount
                -- ModerationFlag.Reason
                Int1      int    null
            );
        )sql");

        _tables.emplace_back(R"sql(
            -- Registry to handle all strings by id
            -- - BlockHashes
            -- - AddressHashes
            -- - TxHashes
            create table if not exists Registry
            (
                RowId integer primary key,
                String text not null unique
            );
        )sql");

        _tables.emplace_back(R"sql(
            create table if not exists Lists
            (
                TxId   int not null, -- TxId that List belongs to
                Order int not null, -- Allowes to use different lists for one tx
                RegId  int not null  -- Entry that list contains
            );
        )sql");

        _tables.emplace_back(R"sql(
            create table if not exists Payload
            (
                TxId  text   primary key, -- Transactions.TxId

                -- AccountUser.Lang
                -- ContentPost.Lang
                -- ContentVideo.Lang
                -- ContentDelete.Settings
                -- Comment.Message
                String1 text   null,

                -- AccountUser.Name
                -- ContentPost.Caption
                -- ContentVideo.Caption
                String2 text   null,

                -- AccountUser.Avatar
                -- ContentPost.Message
                -- ContentVideo.Message
                String3 text   null,

                -- AccountUser.About
                -- ContentPost.Tags JSON
                -- ContentVideo.Tags JSON
                String4 text   null,

                -- AccountUser.Url
                -- ContentPost.Images JSON
                -- ContentVideo.Images JSON
                String5 text   null,

                -- AccountUser.Pubkey
                -- ContentPost.Settings JSON
                -- ContentVideo.Settings JSON
                String6 text   null,

                -- AccountUser.Donations JSON
                -- ContentPost.Url
                -- ContentVideo.Url
                String7 text   null,

                Int1    int    null
            );
        )sql");

        _tables.emplace_back(R"sql(
            create table if not exists TxOutputs
            (
                TxId            int    not null, -- Transactions.TxId
                Number          int    not null, -- Number in tx.vout
                AddressId       int    null, -- Address
                Value           int    not null, -- Amount
                ScriptPubKey    text   not null, -- Original script
                primary key (TxId, Number, AddressId)
            );
        )sql");

        _tables.emplace_back(R"sql(
            create table if not exists TxInputs
            (
                SpentTxId int not null,
                TxId      int not null,
                Number    int not null
            );
        )sql");

        _tables.emplace_back(R"sql(
            create table if not exists Ratings
            (
                Type   int not null,
                Last   int not null,
                Height int not null,
                Uid     int not null,
                Value  int not null,
                primary key (Type, Height, Uid, Value)
            );
        )sql");

        _tables.emplace_back(R"sql(
            create table if not exists Balances
            (
                AddressId       int     not null,
                Last            int     not null,
                Height          int     not null,
                Value           int     not null,
                primary key (AddressId, Height)
            );
        )sql");

        _tables.emplace_back(R"sql(
            create table if not exists System
            (
                Db text not null,
                Version int not null,
                primary key (Db)
            );
        )sql");

        _tables.emplace_back(R"sql(
            create table if not exists BlockingLists
            (
                IdSource int not null,
                IdTarget int not null,
                primary key (IdSource, IdTarget)
            );
        )sql");

        
        _preProcessing = R"sql(
            insert or ignore into System (Db, Version) values ('main', 1);
            delete from Balances where AddressId = (select RowId from Registry where String = '');
        )sql";


        _indexes = R"sql(
            create index if not exists Chain_Uid on Chain (Uid);

            create unique index if not exists TxInputs_SpentTxId_TxId_Number on TxInputs (SpentTxId, TxId, Number);

            create index if not exists Ratings_Last_Uid_Height on Ratings (Last, Uid, Height);
            create index if not exists Ratings_Height_Last on Ratings (Height, Last);
            create index if not exists Ratings_Type_Uid_Value on Ratings (Type, Uid, Value);
            create index if not exists Ratings_Type_Uid_Last_Height on Ratings (Type, Uid, Last, Height);
            create index if not exists Ratings_Type_Uid_Last_Value on Ratings (Type, Uid, Last, Value);
            create index if not exists Ratings_Type_Uid_Height_Value on Ratings (Type, Uid, Height, Value);

            create index if not exists Payload_String2_nocase_TxId on Payload (String2 collate nocase, TxId);
            create index if not exists Payload_String7 on Payload (String7);
            create index if not exists Payload_String1_TxId on Payload (String1, TxId);

            create index if not exists Balances_Height on Balances (Height);
            create index if not exists Balances_AddressId_Last_Height on Balances (AddressId, Last, Height);
            create index if not exists Balances_Last_Value on Balances (Last, Value);
            create index if not exists Balances_AddressId_Last on Balances (AddressId, Last);
        )sql";

        _postProcessing = R"sql(

        )sql";
    }
}