#include "pocketdb/migrations/main.h"

namespace PocketDb
{
    PocketDbMigration::PocketDbMigration()
    {
        Tables.emplace_back(R"sql(
            create table if not exists Transactions
            (
                Type      int    not null,
                Hash      text   not null primary key,
                Time      int    not null,

                BlockHash text   null,
                BlockNum  int    null,
                Height    int    null,

                -- AccountUser
                -- ContentPost
                -- ContentVideo
                -- ContentDelete
                -- Comment
                -- Blocking
                -- Subscribe
                Last      int    not null default 0,

                -- AccountUser.Id
                -- ContentPost.Id
                -- ContentVideo.Id
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
                String1   text   null,

                -- AccountUser.ReferrerAddressHash
                -- ContentPost.RootTxHash
                -- ContentVideo.RootTxHash
                -- ContentDelete.RootTxHash
                -- Comment.RootTxHash
                -- ScorePost.PostTxHash
                -- ScoreComment.CommentTxHash
                -- Subscribe.AddressToHash
                -- Blocking.AddressToHash
                -- Complain.PostTxHash
                String2   text   null,

                -- ContentPost.RelayTxHash
                -- ContentVideo.RelayTxHash
                -- Comment.PostTxHash
                String3   text   null,

                -- Comment.ParentTxHash
                String4   text   null,

                -- Comment.AnswerTxHash
                String5   text   null,

                -- ScoreContent.Value
                -- ScoreComment.Value
                -- Complain.Reason
                Int1      int    null
            );
        )sql");

        Tables.emplace_back(R"sql(
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

                -- Comment.Donate
                Int1    int    null
            );
        )sql");

        Tables.emplace_back(R"sql(
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

        Tables.emplace_back(R"sql(
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

        Tables.emplace_back(R"sql(
            create table if not exists Balances
            (
                AddressHash     text    not null,
                Last            int     not null,
                Height          int     not null,
                Value           int     not null,
                primary key (AddressHash, Height)
            );
        )sql");


        Views.emplace_back(R"sql(
            drop view if exists vWebUsers;
            create view vWebUsers as
            select t.Hash,
                t.Id,
                t.Time,
                t.BlockHash,
                t.Height,
                t.String1 as AddressHash,
                t.String2 as ReferrerAddressHash,
                t.Int1    as Registration,
                p.String1 as Lang,
                p.String2 as Name,
                p.String3 as Avatar,
                p.String4 as About,
                p.String5 as Url,
                p.String6 as Pubkey,
                p.String7 as Donations
            from Transactions t
                    join Payload p on t.Hash = p.TxHash
            where t.Last = 1
            and t.Type = 100;
        )sql");

        Views.emplace_back(R"sql(
            drop view if exists vWebContents;
            create view vWebContents as
            select t.Hash,
                t.Time,
                t.BlockHash,
                t.Height,
                t.String1 as AddressHash,
                t.String2 as RootTxHash,
                t.String3 as RelayTxHash,
                t.Type,
                p.String1 as Lang,
                p.String2 as Caption,
                p.String3 as Message,
                p.String4 as Tags,
                p.String5 as Images,
                p.String6 as Settings,
                p.String7 as Url
            from Transactions t
                    join Payload p on t.Hash = p.TxHash
            where t.Last = 1;
        )sql");

        Views.emplace_back(R"sql(
            drop view if exists vWebPosts;
            create view vWebPosts as
            select t.Hash,
                t.Time,
                t.BlockHash,
                t.Height,
                t.String1 as AddressHash,
                t.String2 as RootTxHash,
                p.String1 as Lang,
                p.String2 as Caption,
                p.String3 as Message,
                p.String4 as Tags,
                p.String5 as Images,
                p.String6 as Settings,
                p.String7 as Url
            from Transactions t
                    join Payload p on t.Hash = p.TxHash
            where t.Height = (
                select max(t_.Height)
                from Transactions t_
                where t_.String2 = t.String2
                and t_.Type = t.Type)
            and t.Type = 200;
        )sql");

        Views.emplace_back(R"sql(
            drop view if exists vWebComments;
            create view vWebComments as
            select t.Hash,
                t.Time,
                t.BlockHash,
                t.Height,
                t.String1 as AddressHash,
                t.String2 as RootTxHash,
                t.String3 as PostTxId,
                t.String4 as ParentTxHash,
                t.String5 as AnswerTxHash,
                p.String1 as Lang,
                p.String2 as Message
            from Transactions t
                    join Payload p on t.Hash = p.TxHash
            where t.Height = (
                select max(t_.Height)
                from Transactions t_
                where t_.String2 = t.String2
                and t_.Type = t.Type)
            and t.Type = 204;
        )sql");

        Views.emplace_back(R"sql(
            drop view if exists vWebScorePosts;
            create view vWebScorePosts as
            select String1 as AddressHash,
                String2 as PostTxHash,
                Int1    as Value
            from Transactions
            where Type in (300);
        )sql");

        Views.emplace_back(R"sql(
            drop view if exists vWebScoreComments;
            create view vWebScoreComments as
            select String1 as AddressHash,
                String2 as CommentTxHash,
                Int1    as Value
            from Transactions
            where Type in (301);
        )sql");

        Views.emplace_back(R"sql(
            drop view if exists vWebScoresPosts;
            create view vWebScoresPosts as
            select count(*) as cnt, avg(Int1) as average, String2 as PostTxHash
            from Transactions
            where Type = 300
            group by String2;
        )sql");

        Views.emplace_back(R"sql(
            drop view if exists vWebScoresComments;
            create view vWebScoresComments as
            select count(*) as cnt, avg(Int1) as average, String2 as CommentTxHash
            from Transactions
            where Type = 301
            group by String2;
        )sql");


        Indexes = R"sql(
            create index if not exists Transactions_Id on Transactions (Id);
            create index if not exists Transactions_Id_Last on Transactions (Id, Last);
            create index if not exists Transactions_Hash_Height on Transactions (Hash, Height);
            create index if not exists Transactions_Height_Type on Transactions (Height, Type);
            create index if not exists Transactions_Type_Last_String1_Height on Transactions (Type, Last, String1, Height);
            create index if not exists Transactions_Type_Last_String2_Height on Transactions (Type, Last, String2, Height);
            create index if not exists Transactions_Type_Last_String1_String2_Height on Transactions (Type, Last, String1, String2, Height);
            create index if not exists Transactions_Type_Last_Height_String3 on Transactions (Type, Last, Height, String3);
            create index if not exists Transactions_Type_String1_String2_Height on Transactions (Type, String1, String2, Height);
            create index if not exists Transactions_Type_String1_Height_Time_Int1 on Transactions (Type, String1, Height, Time, Int1);
            create index if not exists Transactions_String1_Last_Height on Transactions (String1, Last, Height);
            create index if not exists Transactions_Last_Id_Height on Transactions (Last, Id, Height);
            create index if not exists Transactions_Time_Type_Height on Transactions (Time, Type, Height);
            create index if not exists Transactions_Type_Time_Height on Transactions (Type, Time, Height);
            create index if not exists Transactions_BlockHash on Transactions (BlockHash);

            create index if not exists TxOutputs_TxHeight on TxOutputs (TxHeight);
            create index if not exists TxOutputs_SpentHeight on TxOutputs (SpentHeight);
            create index if not exists TxOutputs_SpentTxHash on TxOutputs (SpentTxHash);
            create index if not exists TxOutputs_Value on TxOutputs (Value);
            create index if not exists TxOutputs_AddressHash_SpentHeight_TxHeight on TxOutputs (AddressHash, SpentHeight, TxHeight);
            create index if not exists TxOutputs_TxHeight_AddressHash on TxOutputs (TxHeight, AddressHash);

            create index if not exists Ratings_Height on Ratings (Height);
            create index if not exists Ratings_Type_Id_Last on Ratings (Type, Id, Last);
            create index if not exists Ratings_Last_Id_Height on Ratings (Last, Id, Height);

            create index if not exists Payload_String2 on Payload (String2);

            create index if not exists Balances_Height on Balances (Height);
            create index if not exists Balances_AddressHash_Height_Last on Balances (AddressHash, Height, Last);
            create index if not exists Balances_Value on Balances (Value);
            create index if not exists Balances_AddressHash_Last on Balances (AddressHash, Last);

            -----------------------------------WEB-----------------------------------
            create index if not exists Transactions_Height_Time on Transactions (Height, Time);
        )sql";
    }
}