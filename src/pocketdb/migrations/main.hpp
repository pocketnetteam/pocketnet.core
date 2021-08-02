#ifndef POCKETDB_MIGRATION_MAIN
#define POCKETDB_MIGRATION_MAIN

#include <string>

namespace PocketDb
{
    using namespace std;

    class PocketDbMigration
    {

    public:

        vector<string> Tables;
        vector<string> Views;
        string Indexes;

        PocketDbMigration()
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
                Last      int    not null default 0,

                        -- User.Id
                            -- Post.Id
                    -- Comment.Id
                Id        int    null,

                -- User.AddressHash
                    -- Post.AddressHash
                    -- Comment.AddressHash
                    -- ScorePost.AddressHash
                    -- ScoreComment.AddressHash
                    -- Subscribe.AddressHash
                    -- Blocking.AddressHash
                    -- Complain.AddressHash
                String1   text   null,

                -- User.ReferrerAddressHash
                    -- Post.RootTxHash
                    -- Comment.RootTxHash
                    -- ScorePost.PostTxHash
                    -- ScoreComment.CommentTxHash
                    -- Subscribe.AddressToHash
                    -- Blocking.AddressToHash
                    -- Complain.PostTxHash
                String2   text   null,

                -- Post.RelayTxHash
                    -- Comment.PostTxHash
                String3   text   null,

                -- Comment.ParentTxHash
                String4   text   null,

                -- Comment.AnswerTxHash
                String5   text   null,

                -- ScorePost.Value
                    -- ScoreComment.Value
                    -- Complain.Reason
                Int1      int    null
                );
            )sql");
            Tables.emplace_back(R"sql(
                create table if not exists Payload
                (
                    TxHash  text   primary key, -- Transactions.Hash

                    -- User.Lang
                    -- Post.Lang
                    -- Comment.Lang
                    String1 text   null,

                    -- User.Name
                    -- Post.Caption
                    -- Comment.Message
                    String2 text   null,

                    -- User.Avatar
                    -- Post.Message
                    String3 text   null,

                    -- User.About
                    -- Post.Tags JSON
                    String4 text   null,

                    -- User.Url
                    -- Post.Images JSON
                    String5 text   null,

                    -- User.Pubkey
                    -- Post.Settings JSON
                    String6 text   null,

                    -- User.Donations JSON
                    -- Post.Url
                    String7 text   null
                );
            )sql");
            Tables.emplace_back(R"sql(
                create table if not exists TxOutputs
                (
                    TxHash      text   not null, -- Transactions.Hash
                    TxHeight    int    null,     -- Transactions.Height
                    Number      int    not null, -- Number in tx.vout
                    AddressHash text   not null, -- Address
                    Value       int    not null, -- Amount
                    SpentHeight int    null,     -- Where spent
                    SpentTxHash text   null,     -- Who spent
                    primary key (TxHash, Number, AddressHash)
                );
            )sql");
            Tables.emplace_back(R"sql(
                create table if not exists Ratings
                (
                    Type   int not null,
                    Height int not null,
                    Id     int not null,
                    Value  int not null,
                    primary key (Type, Id, Height, Value)
                );
            )sql");

            Views.emplace_back(R"sql(
                drop view if exists vAccounts;
                create view{} if not exists vAccounts as
                select t.Type,
                       t.Hash,
                       t.Time,
                       t.BlockHash,
                       t.Height,
                       t.Last,
                       t.Id,
                       t.String1 as AddressHash,
                       t.String2,
                       t.String3,
                       t.String4,
                       t.String5,
                       t.Int1
                from Transactions t
                where t.Type in (100, 101, 102);
            )sql");
            Views.emplace_back(R"sql(
                drop view if exists vUsersPayload;
                create view if not exists vUsersPayload as
                select p.TxHash,
                       p.String1 as Lang,
                       p.String2 as Name,
                       p.String3 as Avatar,
                       p.String4 as About,
                       p.String5 as Url,
                       p.String6 as Pubkey,
                       p.String7 as Donations
                from Payload p;
            )sql");
            Views.emplace_back(R"sql(
                drop view if exists vUsers;
                create view if not exists vUsers as
                select a.Type,
                       a.Hash,
                       a.Time,
                       a.BlockHash,
                       a.Height,
                       a.Last,
                       a.Id,
                       a.AddressHash,
                       a.String2 as ReferrerAddressHash,
                       a.String3,
                       a.String4,
                       a.String5,
                       a.Int1
                from vAccounts a
                where a.Type in (100);
            )sql");
            Views.emplace_back(R"sql(
                drop view if exists vContents;
                create view if not exists vContents as
                select t.Type,
                       t.Hash,
                       t.Time,
                       t.BlockHash,
                       t.Height,
                       t.Last,
                       t.Id,
                       t.String1 as AddressHash,
                       t.String2 as RootTxHash,
                       t.String3,
                       t.String4,
                       t.String5
                from Transactions t
                where t.Type in (200, 201, 202, 203, 204, 205, 206);
            )sql");
            Views.emplace_back(R"sql(
                drop view if exists vPosts;
                create view if not exists vPosts as
                select c.Type,
                       c.Hash,
                       c.Time,
                       c.BlockHash,
                       c.Height,
                       c.Last,
                       c.Id,
                       c.AddressHash,
                       c.RootTxHash,
                       c.String3 as RelayTxHash
                from vContents c
                where c.Type in (200);
            )sql");
            Views.emplace_back(R"sql(
                drop view if exists vVideos;
                create view if not exists vVideos as
                select c.Type,
                       c.Hash,
                       c.Time,
                       c.BlockHash,
                       c.Height,
                       c.Last,
                       c.Id,
                       c.AddressHash,
                       c.RootTxHash,
                       c.String3 as RelayTxHash
                from vContents c
                where c.Type in (201);
            )sql");
            Views.emplace_back(R"sql(
                drop view if exists vComments;
                create view if not exists vComments as
                select c.Type,
                       c.Hash,
                       c.Time,
                       c.BlockHash,
                       c.Height,
                       c.Last,
                       c.Id,
                       c.AddressHash,
                       c.RootTxHash,
                       c.String3 as PostTxHash,
                       c.String4 as ParentTxHash,
                       c.String5 as AnswerTxHash
                from vContents c
                where c.Type in (204, 205, 206);
            )sql");
            Views.emplace_back(R"sql(
                drop view if exists vScores;
                create view if not exists vScores as
                select t.Type,
                       t.Hash,
                       t.Time,
                       t.BlockHash,
                       t.Height,
                       t.Last,
                       t.String1 as AddressHash,
                       t.String2 as ContentTxHash,
                       t.Int1    as Value
                from Transactions t
                where t.Type in (300, 301);
            )sql");
            Views.emplace_back(R"sql(
                drop view if exists vScoreContents;
                create view if not exists vScoreContents as
                select s.Type,
                       s.Hash,
                       s.Time,
                       s.BlockHash,
                       s.Height,
                       s.Last,
                       s.AddressHash,
                       s.ContentTxHash as PostTxHash,
                       s.Value         as Value
                from vScores s
                where s.Type in (300);
            )sql");
            Views.emplace_back(R"sql(
                drop view if exists vScoreComments;
                create view if not exists vScoreComments as
                select s.Type,
                       s.Hash,
                       s.Time,
                       s.BlockHash,
                       s.Height,
                       s.Last,
                       s.AddressHash,
                       s.ContentTxHash as CommentTxHash,
                       s.Value         as Value
                from vScores s
                where s.Type in (301);
            )sql");
            Views.emplace_back(R"sql(
                drop view if exists vBlockings;
                create view if not exists vBlockings as
                select t.Type,
                       t.Hash,
                       t.Time,
                       t.BlockHash,
                       t.Height,
                       t.Last,
                       t.String1 as AddressHash,
                       t.String2 as AddressToHash
                from Transactions t
                where t.Type in (305, 306);
            )sql");
            Views.emplace_back(R"sql(
                drop view if exists vSubscribes;
                create view if not exists vSubscribes as
                select t.Type,
                       t.Hash,
                       t.Time,
                       t.BlockHash,
                       t.Height,
                       t.Last,
                       t.String1 as AddressHash,
                       t.String2 as AddressToHash,
                       t.Int1    as Private
                from Transactions t
                where t.Type in (302, 303, 304);
            )sql");
            Views.emplace_back(R"sql(
                drop view if exists vComplains;
                create view vComplains as
                select t.Type,
                       t.Hash,
                       t.Time,
                       t.BlockHash,
                       t.Height,
                       t.Last,
                       t.String1 as AddressHash,
                       t.String2 as PostTxHash,
                       t.Int1    as Reason
                from Transactions t
                where Type in (307);
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
                where t.Height = (
                    select max(t_.Height)
                    from Transactions t_
                    where t_.String2 = t.String2
                    and t_.Type = t.Type)
                and t.Type in (200, 201);
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
                create index if not exists Transactions_Type on Transactions (Type);
                create index if not exists Transactions_Hash on Transactions (Hash);
                create index if not exists Transactions_BlockHash on Transactions (BlockHash);
                create index if not exists Transactions_Height on Transactions (Height);
                create index if not exists Transactions_Hash_Height on Transactions (Hash, Height);
                create index if not exists Transactions_Height_Type on Transactions (Height, Type);
                create index if not exists Transactions_Time_Type on Transactions (Time, Type);
                create index if not exists Transactions_Type_Time on Transactions (Type, Time);
                create index if not exists Transactions_Type_String1_Height on Transactions (Type, String1, Height, Hash);
                create index if not exists Transactions_Type_String2_Height on Transactions (Type, String2, Height, Hash);
                create index if not exists Transactions_Type_Height_Id on Transactions (Type, Height, Id);
                create index if not exists Transactions_LastAccount on Transactions (Type, Last, String1, Height);
                create index if not exists Transactions_LastContent on Transactions (Type, Last, String2, Height);
                create index if not exists Transactions_LastAction on Transactions (Type, Last, String1, String2, Height);
                create index if not exists Transactions_ExistsScore on Transactions (Type, String1, String2, Height);
                create index if not exists Transactions_CountChain on Transactions (Type, Last, String1, Height, Time);
                create index if not exists Transactions_Type_Id on Transactions (Type, Id);
                create index if not exists TxOutputs_SpentHeight on TxOutputs (SpentHeight);
                create index if not exists TxOutputs_TxHash_Number on TxOutputs (TxHash, Number);
                create index if not exists TxOutputs_AddressHash_SpentHeight_TxHeight on TxOutputs (AddressHash, SpentHeight, TxHeight);
                create index if not exists TxOutputs_SpentTxHash on TxOutputs (SpentTxHash);
                create index if not exists Ratings_Height on Ratings (Height);
                create index if not exists Payload_ExistsAnotherByName on Payload (String2, TxHash);
                create index if not exists Transactions_GetScoreContentCount on Transactions (Type, String1, Height, Time, Int1);
                create index if not exists Transactions_GetScoreContentCount_2 on Transactions (Hash, String1, Type, Height);
            )sql";
        }
    };
}

#endif // POCKETDB_MIGRATION_MAIN