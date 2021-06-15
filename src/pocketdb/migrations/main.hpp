#ifndef POCKETDB_MIGRATION_MAIN
#define POCKETDB_MIGRATION_MAIN

#include <string>

namespace PocketDb
{

    static std::string MainStructure = R"sql(

        create table if not exists Transactions
        (
            Type      int    not null,
            Hash      string not null primary key,
            Time      int    not null,

            BlockHash string null,
            Height    int    null,

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
            String1   string null,

            -- User.ReferrerAddressHash
            -- Post.RootTxHash
            -- Comment.RootTxHash
            -- ScorePost.PostTxHash
            -- ScoreComment.CommentTxHash
            -- Subscribe.AddressToHash
            -- Blocking.AddressToHash
            -- Complain.PostTxHash
            String2   string null,

            -- Post.RelayTxHash
            -- Comment.PostTxHash
            String3   string null,

            -- Comment.ParentTxHash
            String4   string null,

            -- Comment.AnswerTxHash
            String5   string null,

            -- ScorePost.Value
            -- ScoreComment.Value
            -- Complain.Reason
            Int1      int    null
        );


        --------------------------------------------
        --               EXT TABLES               --
        --------------------------------------------
        create table if not exists Payload
        (
            TxHash  string primary key, -- Transactions.Hash

            -- User.Lang
            -- Post.Lang
            -- Comment.Lang
            String1 string null,

            -- User.Name
            -- Post.Caption
            -- Comment.Message
            String2 string null,

            -- User.Avatar
            -- Post.Message
            String3 string null,

            -- User.About
            -- Post.Tags JSON
            String4 string null,

            -- User.Url
            -- Post.Images JSON
            String5 string null,

            -- User.Pubkey
            -- Post.Settings JSON
            String6 string null,

            -- User.Donations JSON
            -- Post.Url
            String7 string null
        );
        --------------------------------------------
        create table if not exists TxOutputs
        (
            TxHash      string not null, -- Transactions.Hash
            Number      int    not null, -- Number in tx.vout
            AddressHash string not null, -- Address
            Value       int    not null, -- Amount
            SpentHeight int    null,     -- Where spent
            SpentTxHash string null,     -- Who spent
            primary key (TxHash, Number, AddressHash)
        );


        --------------------------------------------
        create table if not exists Ratings
        (
            Type   int not null,
            Height int not null,
            Id     int not null,
            Value  int not null,
            primary key (Type, Height, Id, Value)
        );


        --------------------------------------------
        --                 VIEWS                  --
        --------------------------------------------
        drop view if exists vAccounts;
        create view if not exists vAccounts as
        select t.Type,
            t.Hash,
            t.Time,
            t.BlockHash,
            t.Height,
            t.Id,
            t.String1 as AddressHash,
            t.String2,
            t.String3,
            t.String4,
            t.String5,
            t.Int1
        from Transactions t
        where t.Height is not null
        and t.Type in (100, 101, 102);


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


        drop view if exists vUsers;
        create view if not exists vUsers as
        select a.Type,
            a.Hash,
            a.Time,
            a.BlockHash,
            a.Height,
            a.Id,
            a.AddressHash,
            a.String2 as ReferrerAddressHash,
            a.String3,
            a.String4,
            a.String5,
            a.Int1
        from vAccounts a
        where a.Type = 100;

        --------------------------------------------
        drop view if exists vContents;
        create view if not exists vContents as
        select t.Type,
            t.Hash,
            t.Time,
            t.BlockHash,
            t.Height,
            t.Id,
            t.String1 as AddressHash,
            t.String2 as RootTxHash,
            t.String3,
            t.String4,
            t.String5
        from Transactions t
        where t.Height is not null
        and t.Type in (200, 201, 202, 203, 204, 205, 206);


        drop view if exists vPosts;
        create view if not exists vPosts as
        select c.Type,
            c.Hash,
            c.Time,
            c.BlockHash,
            c.Height,
            c.Id,
            c.AddressHash,
            c.RootTxHash,
            c.String3 as RelayTxHash
        from vContents c
        where c.Type = 200;


        drop view if exists vComments;
        create view if not exists vComments as
        select c.Type,
            c.Hash,
            c.Time,
            c.BlockHash,
            c.Height,
            c.Id,
            c.AddressHash,
            c.RootTxHash,
            c.String3 as PostTxHash,
            c.String4 as ParentTxHash,
            c.String5 as AnswerTxHash
        from vContents c
        where c.Type in (204, 205, 206);

        --------------------------------------------
        drop view if exists vScores;
        create view if not exists vScores as
        select t.Type,
            t.Hash,
            t.Time,
            t.BlockHash,
            t.Height,
            t.String1 as AddressHash,
            t.String2 as ContentTxHash,
            t.Int1    as Value
        from Transactions t
        where t.Height is not null
        and t.Type in (300, 301);


        drop view if exists vScorePosts;
        create view if not exists vScorePosts as
        select s.Type,
            s.Hash,
            s.Time,
            s.BlockHash,
            s.Height,
            s.AddressHash,
            s.ContentTxHash as PostTxHash,
            s.Value         as Value
        from vScores s
        where s.Type in (300);


        drop view if exists vScoreComments;
        create view if not exists vScoreComments as
        select s.Type,
            s.Hash,
            s.Time,
            s.BlockHash,
            s.Height,
            s.AddressHash,
            s.ContentTxHash as CommentTxHash,
            s.Value         as Value
        from vScores s
        where s.Type in (301);


        drop view if exists vBlockings;
        create view if not exists vBlockings as
        select t.Type,
            t.Hash,
            t.Time,
            t.BlockHash,
            t.Height,
            t.String1 as AddressHash,
            t.String2 as AddressToHash
        from Transactions t
        where t.Height is not null
        and t.Type in (305, 306);

        --------------------------------------------
        --               WEB VIEWS                --
        --------------------------------------------

        drop view if exists vWebUsers;
        create view vWebUsers as
        select t.Hash,
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
        where t.Height = (
            select max(t_.Height)
            from Transactions t_
            where t_.String1 = t.String1
            and t_.Type = t.Type)
        and t.Type = 100;

        --------------------------------------------
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

        --
        -- drop view if exists vWebVideos;
        -- create view vWebVideos as
        -- select WI.Id,
        --        WI.Hash,
        --        WI.Time,
        --        WI.Height,
        --        WI.AddressId,
        --        WI.Int1    as RootTxId,
        --        WI.Int2    as RelayTxId,
        --        WI.Address,
        --        WI.String1 as Lang,
        --        WI.String2 as Caption,
        --        WI.String3 as Message,
        --        WI.String4 as Tags,
        --        WI.String5 as Images,
        --        WI.String6 as Settings,
        --        WI.String7 as Url
        -- from vWebItem WI
        -- where WI.Type in (201);
        --
        --
        -- drop view if exists vWebTranslates;
        -- create view vWebTranslates as
        -- select WI.Id,
        --        WI.Hash,
        --        WI.Time,
        --        WI.Height,
        --        WI.AddressId,
        --        WI.Int1    as RootTxId,
        --        WI.Int2    as RelayTxId,
        --        WI.Address,
        --        WI.String1 as Lang,
        --        WI.String2 as Caption,
        --        WI.String3 as Message,
        --        WI.String4 as Tags
        -- from vWebItem WI
        -- where WI.Type in (202);
        --
        -- drop view if exists vWebServerPings;
        -- create view vWebServerPings as
        -- select WI.Id,
        --        WI.Hash,
        --        WI.Time,
        --        WI.Height,
        --        WI.AddressId,
        --        WI.Int1    as RootTxId,
        --        WI.Int2    as RelayTxId,
        --        WI.Address,
        --        WI.String1 as Lang,
        --        WI.String2 as Caption,
        --        WI.String3 as Message,
        --        WI.String4 as Tags
        -- from vWebItem WI
        -- where WI.Type in (203);
        --
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

        --------------------------------------------
        drop view if exists vWebScorePosts;
        create view vWebScorePosts as
        select String1 as AddressHash,
            String2 as PostTxHash,
            Int1    as Value
        from Transactions
        where Type in (300);
        drop view if exists vWebScoreComments;
        create view vWebScoreComments as
        select String1 as AddressHash,
            String2 as CommentTxHash,
            Int1    as Value
        from Transactions
        where Type in (301);

        drop view if exists vWebScoresPosts;
        create view vWebScoresPosts as
        select count(*) as cnt, avg(Int1) as average, String2 as PostTxHash
        from Transactions
        where Type = 300
        group by String2;

        drop view if exists vWebScoresComments;
        create view vWebScoresComments as
        select count(*) as cnt, avg(Int1) as average, String2 as CommentTxHash
        from Transactions
        where Type = 301
        group by String2;

        -----------------------------------------------
        --               SUBSCRIPTIONS               --
        -----------------------------------------------

        
        -- My subscriptions
        -- WITH tmp AS (select t.String1,
        --                     t.String2,
        --                     t.Type,
        --                     ROW_NUMBER() OVER (PARTITION BY t.String1, t.String2
        --                         ORDER BY t.Height DESC) AS rank
        --             FROM Transactions t
        --             where Type in (302, 303, 304)
        --             and String1 in ('PDCNrwP1i8BJQWh2bctuJyAaXxozgMcRYT', 'PMaqHigwsZmsMn6mbGX2PrTB83cgnyYkZn')
        -- )
        -- SELECT String1 as AddressHash,
        --         String2 as AddressHashTo,
        --         Type
        -- FROM tmp
        -- WHERE rank = 1 and Type in (302, 303);

        -- My subscribers
        -- WITH tmp AS (select t.String1,
        --                     t.String2,
        --                     t.Type,
        --                     ROW_NUMBER() OVER (PARTITION BY t.String2, t.String1
        --                         ORDER BY t.Height DESC) AS rank
        --             FROM Transactions t
        --             where Type in (302, 303, 304)
        --             and String2 in ('PDCNrwP1i8BJQWh2bctuJyAaXxozgMcRYT', 'PMaqHigwsZmsMn6mbGX2PrTB83cgnyYkZn')
        -- )
        -- SELECT String1 as AddressHash,
        --         String2 as AddressHashTo,
        --         Type
        -- FROM tmp
        -- WHERE rank = 1 and Type in (302, 303);
        
        -----------------------------------------------
        --                BLOCKINGS                  --
        -----------------------------------------------
        
        -- My blockings
        -- WITH tmp AS (select t.String1,
        --                     t.String2,
        --                     t.Type,
        --                     ROW_NUMBER() OVER (PARTITION BY t.String1, t.String2
        --                         ORDER BY t.Height DESC) AS rank
        --             FROM Transactions t
        --             where Type in (305,306)
        --             and String1 in ('PDCNrwP1i8BJQWh2bctuJyAaXxozgMcRYT', 'PMaqHigwsZmsMn6mbGX2PrTB83cgnyYkZn')
        -- )
        -- SELECT String1 as AddressHash,
        --         String2 as AddressHashTo
        -- FROM tmp
        -- WHERE rank = 1 and Type=305;
       
        drop view if exists Complains;
        create view Complains as
        select String1 as AddressHash,
            String2 as PostTxHash,
            Int1    as Reason
        from Transactions
        where Type in (307);

        -- vacuum;
        ---------------------------------------------------------
        --               FULL TEXT SEARCH TABLES               --
        ---------------------------------------------------------
        
        -- DROP TABLE PayloadUsers_fts;
        -- CREATE VIRTUAL TABLE PayloadUsers_fts USING fts5(
        --     TxHash UNINDEXED,
        --     Name,
        --     About
        -- );

        -- insert into PayloadUsers_fts
        -- select
        --     t.Hash,
        --     p.String2 as Name, -- DECODE and UnEscape string
        --     p.String4 as About
        -- from Transactions t
        -- join Payload p on t.Hash = p.TxHash
        -- where t.Type = 100;
        -- select * from PayloadUsers_fts f, vWebUsers wu
        -- where f.Name match 'loki*' and f.TxHash=wu.Hash
        -- limit 60;
        

        
        -- DROP TABLE PayloadPosts_fts;
        -- CREATE VIRTUAL TABLE PayloadPosts_fts USING fts5(
        --     TxHash UNINDEXED,
        --     Caption,
        --     Message,
        --     Tags
        -- );
        
        -- insert into PayloadPosts_fts
        -- select
        --     t.Hash,
        --     p.String2 as Caption,
        --     p.String3 as Message,
        --     p.String4 as Tags
        -- from Transactions t
        -- join Payload p on t.Hash = p.TxHash
        -- where t.Type = 200;
        -- select wu.* from PayloadPosts_fts f, vWebPosts wu
        -- where f.message match 'Trump' and f.TxHash=wu.Hash
        -- limit 60;

        
        -- DROP TABLE PayloadComments_fts;
        -- CREATE VIRTUAL TABLE PayloadComments_fts USING fts5(
        --     TxHash UNINDEXED,
        --     Message
        -- );

        -- insert into PayloadComments_fts
        -- select
        --     t.Hash,
        --     p.String2 as Message
        -- from Transactions t
        -- join Payload p on t.Hash = p.TxHash
        -- where t.Type = 204;
        -- select wc.* from PayloadComments_fts f, vWebComments wc
        -- where f.message match 'nothing' and f.TxHash=wc.Hash
        -- limit 60;

    )sql";

    static std::string MainDropIndexes = R"sql(

        drop index if exists Transactions_Type;
        drop index if exists Transactions_Hash;
        drop index if exists Transactions_Time;
        drop index if exists Transactions_BlockHash;
        drop index if exists Transactions_Height;
        drop index if exists Transactions_String1;
        drop index if exists Transactions_String2;
        drop index if exists Transactions_String3;
        drop index if exists Transactions_String4;
        drop index if exists Transactions_String5;
        drop index if exists Transactions_Int1;
        drop index if exists Transactions_Type_String1_Height;
        drop index if exists Transactions_Type_String2_Height;
        drop index if exists Transactions_Type_Height_Id;
        drop index if exists Transactions_Type_Id;
        drop index if exists TxOutput_SpentHeight;
        drop index if exists TxOutput_TxHash_Number;
        drop index if exists TxOutputs_AddressHash_SpentHeight_Value;
        drop index if exists Ratings_Height;
        drop index if exists Ratings_Type_Id_Value;
        drop index if exists Ratings_Type_Id_Height;
        drop index if exists Payload_String2_TxHash;

    )sql";

    static std::string MainCreateIndexes = R"sql(

        create index if not exists Transactions_Type on Transactions (Type);
        create index if not exists Transactions_Hash on Transactions (Hash);
        create index if not exists Transactions_Time on Transactions (Time);
        create index if not exists Transactions_BlockHash on Transactions (BlockHash);
        create index if not exists Transactions_Height on Transactions (Height);
        create index if not exists Transactions_String1 on Transactions (String1);
        create index if not exists Transactions_String2 on Transactions (String2);
        create index if not exists Transactions_String3 on Transactions (String3);
        create index if not exists Transactions_String4 on Transactions (String4);
        create index if not exists Transactions_String5 on Transactions (String5);
        create index if not exists Transactions_Int1 on Transactions (Int1);
        create index if not exists Transactions_Type_String1_Height on Transactions (Type, String1, Height, Hash);
        create index if not exists Transactions_Type_String2_Height on Transactions (Type, String2, Height, Hash);
        create index if not exists Transactions_Type_Height_Id on Transactions (Type, Height, Id);

        -- ChainRepository::SetContentId
        create index if not exists Transactions_Type_Id on Transactions (Type, Id);

        create index if not exists TxOutput_SpentHeight on TxOutputs (SpentHeight);
        create index if not exists TxOutput_TxHash_Number on TxOutputs (TxHash, Number);
        create index if not exists TxOutputs_AddressHash_SpentHeight_Value on TxOutputs (AddressHash, SpentHeight, Value);
        create index if not exists Ratings_Height on Ratings (Height);
        create index if not exists Ratings_Type_Id_Value on Ratings (Type, Id, Value);
        create index if not exists Ratings_Type_Id_Height on Ratings (Type, Id, Height desc);

        -- ConsensusRepository::ExistsAnotherByName
        create index if not exists Payload_String2_TxHash on Payload (String2, TxHash);

    )sql";

    
}

#endif // POCKETDB_MIGRATION_MAIN