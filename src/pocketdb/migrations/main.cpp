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
                -- AccountSetting.Id
                -- AccountDelete.Id
                -- ContentPost.Id
                -- ContentVideo.Id
                -- ContentArticle.Id
                -- ContentStream.Id
                -- ContentAudio.Id
                -- ContentDelete.Id
                -- Comment.Id
                Uid       int     null
            );
        )sql");


        _tables.emplace_back(R"sql(
            create table if not exists Last
            (
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
                TxId integer primary key
            );
        )sql");

        _tables.emplace_back(R"sql(
            create table if not exists First
            (
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
                TxId integer primary key
            );
        )sql");

        _tables.emplace_back(R"sql(
            create table if not exists Mempool
            (
                TxId integer primary key
            );
        )sql");

        _tables.emplace_back(R"sql(
            create table if not exists Transactions
            (
                RowId    integer primary key, -- Id of tx hash in Registry table
                Type      int    not null,
                Time      int    not null,

                -- AccountUser.ReferrerAddressId
                -- ContentPost.RootTxId
                -- ContentVideo.RootTxId
                -- ContentArticle.RootTxId
                -- ContentStream.RootTxId
                -- ContentAudio.RootTxId
                -- ContentDelete.RootTxId
                -- Comment.RootTxId
                -- CommentEdit.RootTxId
                -- CommentDelete.RootTxId
                -- ScorePost.ContentRootTxId
                -- ScoreComment.CommentRootTxId
                -- Subscribe.AddressToId
                -- Blocking.AddressToId
                -- Complain.ContentRootTxId
                -- Boost.ContentRootTxId
                -- ModerationFlag.ContentTxId
                -- ModerationVote.JuryId
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
                -- ModerationVote.Verdict
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
                String text not null
            );
        )sql");

        _tables.emplace_back(R"sql(
            create table if not exists Lists
            (
                TxId       int not null, -- TxId that List belongs to
                OrderIndex int not null, -- Allowes to use different lists for one tx
                RegId      int not null  -- Entry that list contains
            );
        )sql");

        _tables.emplace_back(R"sql(
            create table if not exists Payload
            (
                TxId integer primary key, -- Transactions.RowId

                -- AccountUser.Lang
                -- ContentPost.Lang
                -- ContentVideo.Lang
                -- ContentDelete.Settings
                -- Comment.Message
                -- Barteron.Offer.Lang
                String1 text   null,

                -- AccountUser.Name
                -- ContentPost.Caption
                -- ContentVideo.Caption
                -- Barteron.Offer.Caption
                String2 text   null,

                -- AccountUser.Avatar
                -- ContentPost.Message
                -- ContentVideo.Message
                -- Barteron.Offer.Message
                String3 text   null,

                -- AccountUser.About
                -- ContentPost.Tags JSON
                -- ContentVideo.Tags JSON
                -- Barteron.Account.Settings JSON
                -- Barteron.Offer.Settings JSON
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
                AddressId       int    not null, -- Address
                Value           int    not null, -- Amount
                ScriptPubKeyId  int    not null -- Original script
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
                Uid    int not null,
                Value  int not null
            );
        )sql");

        _tables.emplace_back(R"sql(
            create table if not exists Balances
            (
                AddressId   integer primary key,
                Value       int not null
            );
        )sql");

        _tables.emplace_back(R"sql(
            create table if not exists BlockingLists
            (
                IdSource    int not null,
                IdTarget    int not null,
                primary key (IdSource, IdTarget)
            );
        )sql");

        _tables.emplace_back(R"sql(
            create table if not exists Jury
            (
                -- Link to the Flag (via ROWID) that initiated this ury
                FlagRowId     int   not null,
                -- Transactions.Id
                AccountId     int   not null,
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
                -- Transactions.Id
                AccountId     int   not null,
                primary key (FlagRowId, AccountId)
            ) without rowid;
        )sql");

        _tables.emplace_back(R"sql(
            create table if not exists JuryBan
            (
                -- Link to the Vote (via ROWID) that initiated this ban
                VoteRowId     int   not null,
                -- Transactions.Id
                AccountId     int   not null,
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
                Cancel      int   not null,
                Height      int   not null,
                primary key (AccountId, Badge, Cancel, Height)
            ) without rowid;
        )sql");

        _views.emplace_back(R"sql(
            drop view if exists vBadges;

            create view if not exists vBadges as
            select
                Badge, Cancel, AccountId, Height
            from
                Badges b indexed by Badges_Badge_Cancel_AccountId_Height
            where
                b.Badge in (0,1,2,3) and
                b.Cancel = 0 and
                not exists (
                    select
                        1
                    from
                        Badges bb indexed by Badges_Badge_Cancel_AccountId_Height
                    where
                        bb.Badge = b.Badge and
                        bb.Cancel = 1 and
                        bb.AccountId = b.AccountId and
                        bb.Height > b.Height
                );
        )sql");

        // A helper table that consists of limited txs for a period of ConsensusLimit_depth
        _tables.emplace_back(R"sql(
            create table if not exists SocialRegistry
            (
                AddressId int not null,
                Type int not null,
                Height int not null,
                BlockNum int not null, -- TODO (losty): required only to not allow duplicates
                primary key (Type, Height, AddressId, BlockNum)
            ) without rowid;
        )sql");

        _views.emplace_back(R"sql(
            drop view if exists vTx;
            create view if not exists vTx as
            select
                t.RowId, t.Type, t.Time, t.RegId1, t.RegId2, t.RegId3, t.RegId4, t.RegId5, t.Int1,
                r.String as Hash
            from
                Registry r indexed by Registry_String,
                Transactions t
            where
                t.RowId = r.RowId;
        )sql");

        _views.emplace_back(R"sql(
            drop view if exists vTxStr;
            create view if not exists vTxStr as
            select
                t.RowId as RowId,
                (select r.String from Registry r where r.RowId = t.RowId) as Hash,
                (select r.String from Registry r where r.RowId = t.RegId1) as String1,
                (select r.String from Registry r where r.RowId = t.RegId2) as String2,
                (select r.String from Registry r where r.RowId = t.RegId3) as String3,
                (select r.String from Registry r where r.RowId = t.RegId4) as String4,
                (select r.String from Registry r where r.RowId = t.RegId5) as String5
            from Transactions t;
        )sql");

        _preProcessing = R"sql(
            PRAGMA user_version = 1;
        )sql";


        _requiredIndexes = R"sql(
            create unique index if not exists Registry_String on Registry (String);
        )sql";

        _indexes = R"sql(
            drop index if exists Chain_Height_BlockId;
            drop index if exists TxInputs_TxId_Number;
            drop index if exists TxInputs_SpentTxId_TxId_Number;
            drop index if exists TxOutputs_AddressId_TxId_Number;

            create index if not exists Chain_Uid_Height on Chain (Uid, Height);
            create index if not exists Chain_Height_Uid on Chain (Height, Uid);
            create index if not exists Chain_Height_BlockNum on Chain (Height desc, BlockNum desc);
            create index if not exists Chain_BlockId_Height on Chain (BlockId, Height);
            create index if not exists Chain_TxId_Height on Chain (TxId, Height);
            create index if not exists Chain_HeightByDay on Chain (Height / 1440 desc);
            create index if not exists Chain_HeightByHour on Chain (Height / 60 desc);


            create index if not exists Transactions_Type_RegId1_RegId2_RegId3 on Transactions (Type, RegId1, RegId2, RegId3);
            create index if not exists Transactions_Type_RegId1_RegId3 on Transactions (Type, RegId1, RegId3);
            create index if not exists Transactions_Type_RegId2_RegId1 on Transactions (Type, RegId2, RegId1);
            create index if not exists Transactions_Type_RegId3_RegId1 on Transactions (Type, RegId3, RegId1);
            create index if not exists Transactions_Type_RegId5_RegId1 on Transactions (Type, RegId5, RegId1);
            create index if not exists Transactions_Type_RegId4_RegId1 on Transactions (Type, RegId4, RegId1);
            create index if not exists Transactions_Type_RegId1_Int1_Time on Transactions (Type, RegId1, Int1, Time);
            create index if not exists Transactions_Type_RegId1_Time on Transactions (Type, RegId1, Time);
            create index if not exists Transactions_Type_RegId3_RegId4_RegId5 on Transactions(Type, RegId3, RegId4, RegId5);
            create index if not exists Transactions_RowId_desc_Type on Transactions(RowId desc, Type);

            create index if not exists TxInputs_SpentTxId_Number_TxId on TxInputs (SpentTxId, Number, TxId);
            create index if not exists TxInputs_TxId_Number_SpentTxId on TxInputs (TxId, Number, SpentTxId);

            create index if not exists TxOutputs_TxId_Number_AddressId on TxOutputs (TxId, Number, AddressId);
            create index if not exists TxOutputs_AddressId_TxIdDesc_Number on TxOutputs (AddressId, TxId desc, Number);

            create unique index if not exists Lists_TxId_OrderIndex_RegId on Lists (TxId, OrderIndex asc, RegId);

            create index if not exists BlockingLists_IdTarget_IdSource on BlockingLists (IdTarget, IdSource);

            ------------------------------

            create index if not exists Ratings_Last_Uid_Height on Ratings (Last, Uid, Height);
            create index if not exists Ratings_Height_Last on Ratings (Height, Last);
            create index if not exists Ratings_Type_Uid_Value on Ratings (Type, Uid, Value);
            create index if not exists Ratings_Type_Uid_Last_Height on Ratings (Type, Uid, Last, Height);
            create index if not exists Ratings_Type_Uid_Last_Value on Ratings (Type, Uid, Last, Value);
            create index if not exists Ratings_Type_Uid_Height_Value on Ratings (Type, Uid, Height, Value);

            create index if not exists Balances_Value on Balances (Value);

            create index if not exists Payload_String2_nocase on Payload (String2 collate nocase);
            create index if not exists Payload_String7 on Payload (String7);
            create index if not exists Payload_String1 on Payload (String1);

            create index if not exists Jury_AccountId_Reason on Jury (AccountId, Reason);
            create index if not exists JuryBan_AccountId_Ending on JuryBan (AccountId, Ending);
            create index if not exists JuryVerdict_VoteRowId_FlagRowId_Verdict on JuryVerdict (VoteRowId, FlagRowId, Verdict);
            create index if not exists JuryModerators_AccountId_FlagRowId on JuryModerators (AccountId, FlagRowId);

            create index if not exists Badges_Badge_Cancel_AccountId_Height on Badges (Badge, Cancel, AccountId, Height);

            create index if not exists SocialRegistry_Type_AddressId on SocialRegistry (Type, AddressId);
            create index if not exists SocialRegistry_Height on SocialRegistry (Height);


        )sql";

        _postProcessing = R"sql(

        )sql";
    }
}