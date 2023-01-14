// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include "pocketdb/migrations/main.h"

namespace PocketDb
{
    PocketDbMainMigration::PocketDbMainMigration() : PocketDbMigration()
    {
        _tables.emplace_back(R"sql(
            create table if not exists Transactions
            (
                Type      int    not null,
                Hash      text   not null primary key,
                Time      int    not null,

                BlockHash text   null,
                BlockNum  int    null,
                Height    int    null,

                -- AccountUser
                -- AccountSetting
                -- AccountDelete
                -- ContentPost
                -- ContentVideo
                -- ContentArticle
                -- ContentDelete
                -- ContentStream
                -- ContentAudio
                -- Comment
                -- Blocking
                -- Subscribe
                Last      int    not null default 0,
                First     int    not null default 0,

                -- AccountUser.Id
                -- AccountSetting.Id
                -- AccountDelete.Id
                -- ContentPost.Id
                -- ContentVideo.Id
                -- ContentArticle.Id
                -- ContentStream.Id
                -- ContentAudio.Id
                -- ContentDelete.Id
                -- Comment.Id
                Id        int    null,

                -- AccountUser.AddressHash
                -- ContentPost.AddressHash
                -- ContentVideo.AddressHash
                -- ContentDelete.AddressHash
                -- Comment.AddressHash
                -- ScorePost.AddressHash
                -- ScoreComment.AddressHash
                -- Subscribe.AddressHash
                -- Blocking.AddressHash
                -- Complain.AddressHash
                -- Boost.AddressHash
                String1   text   null,

                -- AccountUser.ReferrerAddressHash
                -- ContentPost.RootTxHash
                -- ContentVideo.RootTxHash
                -- ContentArticle.RootTxHash
                -- ContentStream.RootTxHash
                -- ContentAudio.RootTxHash
                -- ContentDelete.RootTxHash
                -- Comment.RootTxHash
                -- CommentEdit.RootTxHash
                -- CommentDelete.RootTxHash
                -- ScorePost.ContentRootTxHash
                -- ScoreComment.CommentRootTxHash
                -- Subscribe.AddressToHash
                -- Blocking.AddressToHash
                -- Complain.ContentRootTxHash
                -- Boost.ContentRootTxHash
                -- ModerationFlag.ContentTxHash
                -- ModerationVote.JuryId
                String2   text   null,

                -- ContentPost.RelayRootTxHash
                -- ContentVideo.RelayRootTxHash
                -- Comment.ContentRootTxHash
                String3   text   null,

                -- Comment.ParentRootTxHash
                String4   text   null,

                -- Comment.AnswerRootTxHash
                String5   text   null,

                -- ScoreContent.Value
                -- ScoreComment.Value
                -- Complain.Reason
                -- Boost.Amount
                -- ModerationFlag.Reason
                -- ModerationVote.Verdict
                Int1      int    null
            );
        )sql");

        _tables.emplace_back(R"sql(
            create table if not exists Payload
            (
                TxHash  text   primary key, -- Transactions.Hash

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
                TxHash          text   not null, -- Transactions.Hash
                TxHeight        int    null,     -- Transactions.Height
                Number          int    not null, -- Number in tx.vout
                AddressHash     text   not null, -- Address
                Value           int    not null, -- Amount
                ScriptPubKey    text   not null, -- Original script
                SpentHeight     int    null,     -- Where spent
                SpentTxHash     text   null,     -- Who spent
                primary key (TxHash, Number, AddressHash)
            );
        )sql");

        _tables.emplace_back(R"sql(
            create table if not exists TxInputs
            (
                SpentTxHash text not null,
                TxHash text not null,
                Number int not null
            );
        )sql");

        _tables.emplace_back(R"sql(
            create table if not exists Ratings
            (
                Type   int not null,
                Last   int not null,
                Height int not null,
                Id     int not null,
                Value  int not null,
                primary key (Type, Height, Id, Value)
            );
        )sql");

        _tables.emplace_back(R"sql(
            create table if not exists Balances
            (
                AddressHash     text    not null,
                Last            int     not null,
                Height          int     not null,
                Value           int     not null,
                primary key (AddressHash, Height)
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

        _tables.emplace_back(R"sql(
            create table if not exists Jury
            (
                -- Link to the Flag (via ROWID) that initiated this ury
                FlagRowId     int   not null,
                AddressHash   text  not null,
                Reason        int   not null,
                primary key (FlagRowId)
            ) without rowid;
        )sql");

        _tables.emplace_back(R"sql(
            create table if not exists JuryVerdict
            (
                -- Link to the Flag (via ROWID) that initiated this ury
                FlagRowId     int   not null,
                -- Link to the Vote (via ROWID) that initiated this ury
                VoteRowId     int   not null,
                -- 1 or 0
                Verdict       int   null,
                primary key (FlagRowId)
            ) without rowid;
        )sql");

        _tables.emplace_back(R"sql(
            create table if not exists JuryModerators
            (
                -- Link to the Flag (via ROWID) that initiated this ury
                FlagRowId     int   not null,
                -- Link to uniq id of address
                AccountId     int   not null,
                primary key (FlagRowId, AccountId)
            ) without rowid;
        )sql");

        _tables.emplace_back(R"sql(
            create table if not exists JuryBan
            (
                -- Link to the Vote (via ROWID) that initiated this ban
                VoteRowId     int   not null,
                AddressHash   text  not null,
                -- The height to which the penalty continues to apply
                Ending        int   not null,
                primary key (VoteRowId)
            ) without rowid;
        )sql");

        _tables.emplace_back(R"sql(
            create table if not exists Badges
            (
                -- Transactions.Id
                AccountId   int   not null,
                -- Developer = 0
                -- Shark = 1
                -- Whale = 2
                -- Moderator = 3
                Badge       int   not null,
                primary key (AccountId, Badge)
            ) without rowid;
        )sql");
        
        _preProcessing = R"sql(
            insert or ignore into System (Db, Version) values ('main', 0);
        )sql";


        _indexes = R"sql(

            drop index if exists Payload_String2;
            drop index if exists Payload_String2_TxHash;
            drop index if exists Transactions_Height_Time;
            drop index if exists Transactions_Time_Type_Height;
            drop index if exists Transactions_Type_Time_Height;

            create index if not exists Transactions_Id on Transactions (Id);
            create index if not exists Transactions_Id_Last on Transactions (Id, Last);
            create index if not exists Transactions_Id_First on Transactions (Id, First);
            create index if not exists Transactions_Hash_Height on Transactions (Hash, Height);
            create index if not exists Transactions_Height_Type on Transactions (Height, Type);
            create index if not exists Transactions_Hash_Type_Height on Transactions (Hash, Type, Height);
            create index if not exists TxOutputs_AddressHash_TxHeight_TxHash on TxOutputs (AddressHash, TxHeight, TxHash);
            create index if not exists Transactions_Type_Last_String1_Height_Id on Transactions (Type, Last, String1, Height, Id);
            create index if not exists Transactions_Type_Last_String2_Height on Transactions (Type, Last, String2, Height);
            create index if not exists Transactions_Type_Last_String3_Height on Transactions (Type, Last, String3, Height);
            create index if not exists Transactions_Type_Last_String4_Height on Transactions (Type, Last, String4, Height);
            create index if not exists Transactions_Type_Last_String5_Height on Transactions (Type, Last, String5, Height);
            create index if not exists Transactions_Type_Last_String1_String2_Height on Transactions (Type, Last, String1, String2, Height);
            create index if not exists Transactions_Type_Last_String2_String1_Height on Transactions (Type, Last, String2, String1, Height);
            create index if not exists Transactions_Type_Last_Height_String5_String1 on Transactions (Type, Last, Height, String5, String1);
            create index if not exists Transactions_Type_Last_Height_Id on Transactions (Type, Last, Height, Id);
            create index if not exists Transactions_Type_String1_String2_Height on Transactions (Type, String1, String2, Height);
            create index if not exists Transactions_Type_String1_String3_Height on Transactions (Type, String1, String3, Height);
            create index if not exists Transactions_Type_String1_Height_Time_Int1 on Transactions (Type, String1, Height, Time, Int1);
            create index if not exists Transactions_String1_Last_Height on Transactions (String1, Last, Height);
            create index if not exists Transactions_Last_Id_Height on Transactions (Last, Id, Height);
            create index if not exists Transactions_BlockHash on Transactions (BlockHash);
            create index if not exists Transactions_Height_Id on Transactions (Height, Id);
            create index if not exists Transactions_Type_HeightByDay on Transactions (Type, (Height / 1440));
            create index if not exists Transactions_Type_HeightByHour on Transactions (Type, (Height / 60));

            create index if not exists TxOutputs_SpentHeight_AddressHash on TxOutputs (SpentHeight, AddressHash);
            create index if not exists TxOutputs_TxHeight_AddressHash on TxOutputs (TxHeight, AddressHash);
            create index if not exists TxOutputs_SpentTxHash on TxOutputs (SpentTxHash);
            create index if not exists TxOutputs_TxHash_AddressHash_Value on TxOutputs (TxHash, AddressHash, Value);
            create index if not exists TxOutputs_AddressHash_TxHeight_SpentHeight on TxOutputs (AddressHash, TxHeight, SpentHeight);

            create unique index if not exists TxInputs_SpentTxHash_TxHash_Number on TxInputs (SpentTxHash, TxHash, Number);

            create index if not exists Ratings_Last_Id_Height on Ratings (Last, Id, Height);
            create index if not exists Ratings_Height_Last on Ratings (Height, Last);
            create index if not exists Ratings_Type_Id_Value on Ratings (Type, Id, Value);
            create index if not exists Ratings_Type_Id_Last_Height on Ratings (Type, Id, Last, Height);
            create index if not exists Ratings_Type_Id_Last_Value on Ratings (Type, Id, Last, Value);
            create index if not exists Ratings_Type_Id_Height_Value on Ratings (Type, Id, Height, Value);

            create index if not exists Payload_String2_nocase_TxHash on Payload (String2 collate nocase, TxHash);
            create index if not exists Payload_String7 on Payload (String7);
            create index if not exists Payload_String1_TxHash on Payload (String1, TxHash);

            create index if not exists Balances_Height on Balances (Height);
            create index if not exists Balances_AddressHash_Last_Height on Balances (AddressHash, Last, Height);
            create index if not exists Balances_Last_Value on Balances (Last, Value);
            create index if not exists Balances_AddressHash_Last on Balances (AddressHash, Last);

            create index if not exists Jury_AddressHash_Reason on Jury (AddressHash, Reason);
            create index if not exists JuryBan_AddressHash_Ending on JuryBan (AddressHash, Ending);
            create index if not exists JuryVerdict_VoteRowId_FlagRowId_Verdict on JuryVerdict (VoteRowId, FlagRowId, Verdict);
            create index if not exists JuryModerators_AccountId_FlagRowId on JuryModerators (AccountId, FlagRowId);

            create index if not exists Badges_Badge_AccountId on Badges (Badge, AccountId);

        )sql";

        _postProcessing = R"sql(

        )sql";
    }
}