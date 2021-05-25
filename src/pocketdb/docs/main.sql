create table if not exists Transactions
(
    Type    int    not null,
    Hash    string not null primary key,
    Time    int    not null,

    BlockHash string null,
    Height int null,

    -- User.Id
    -- Post.Id
    -- Comment.Id
    Id int null,

    -- User.AddressHash
    -- Post.AddressHash
    -- Comment.AddressHash
    -- ScorePost.AddressHash
    -- ScoreComment.AddressHash
    -- Subscribe.AddressHash
    -- Blocking.AddressHash
    -- Complain.AddressHash
    String1 string null,

    -- User.ReferrerAddressHash
    -- Post.RootTxHash
    -- Comment.RootTxHash
    -- ScorePost.PostTxHash
    -- ScoreComment.CommentTxHash
    -- Subscribe.AddressToHash
    -- Blocking.AddressToHash
    -- Complain.PostTxHash
    String2 string null,

    -- Post.RelayTxId
    -- Comment.PostTxId
    String3 string null,

    -- Comment.ParentTxHash
    String4 string null,

    -- Comment.AnswerTxHash
    String5 string null,

    -- ScorePost.Value
    -- ScoreComment.Value
    -- Complain.Reason
    Int1    int    null
);

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

create index if not exists Transactions_Type_String1_index on Transactions (Type, String1);
create index if not exists Transactions_Type_String2_index on Transactions (Type, String2);

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
    TxHash string not null, -- Transactions.Hash
    Number int    not null, -- Number in tx.vout
    AddressHash string not null, -- Address
    Value  int    not null, -- Amount
    SpentHeight int null, -- Where spent
    SpentTxHash string null, -- Who spent
    primary key (TxHash, Number, AddressHash)
);

create index if not exists TxOutputs_AddressHash_SpentHeight_Value on TxOutputs (AddressHash, SpentHeight, Value);

--------------------------------------------
create table if not exists Ratings
(
    Type   int    not null,
    Height int    not null,
    Id     int    not null,
    Value  int    not null,
    primary key (Type, Height, Id, Value)
);

--------------------------------------------
--                 VIEWS                  --
--------------------------------------------

drop view if exists vUsers;
create view vUsers as
select Type,
       Hash,
       Time,
       BlockHash,
       Height,
       String1 as AddressHash,
       String2 as ReferrerAddressHash,
       Int1 as Registration
from Transactions t
where t.Type = 100;


drop view if exists vWebUsers;
create view vWebUsers as
select
       t.Hash,
       t.Time,
       t.BlockHash,
       t.Height,
       t.String1 as AddressHash,
       t.String2 as ReferrerAddressHash,
       t.Int1 as Registration,
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
     where t_.String1 = t.String1 and t_.Type = t.Type)
and t.Type = 100;

drop view if exists vScorePosts;
create view vScorePosts as
select String1 as AddressHash,
       String2 as PostTxHash,
       Int1 as Value,
       Time as Time,
       Height as Height
from Transactions
where Type in (300);

-- drop view if exists vTransactions;
-- create view vTransactions as
-- select T.Id,
--        T.Type,
--        T.Hash,
--        T.Time,
--        T.AddressId,
--        T.Int1,
--        T.Int2,
--        T.Int3,
--        T.Int4,
--        C.Height
-- from Transactions T
--          left join Chain C on T.Id = C.TxId;
--
-- drop view if exists vItem;
-- create view vItem as
-- select t.Id,
--        t.Type,
--        t.Hash,
--        t.Time,
--        t.Height,
--        t.AddressId,
--        t.Int1,
--        t.Int2,
--        t.Int3,
--        t.Int4,
--        a.Address
-- from vTransactions t
--          join Addresses a on a.Id = t.AddressId;
--
-- drop view if exists vUsers;
-- create view vUsers as
-- select Id,
--        Hash,
--        Time,
--        Height,
--        AddressId,
--        Address,
--        Int1 as Registration,
--        Int2 as ReferrerId
-- from vItem i
-- where i.Type in (100);
--
--
--
-- drop view if exists vWebItem;
-- create view vWebItem as
-- select I.Id,
--        I.Type,
--        I.Hash,
--        I.Time,
--        I.Height,
--        I.AddressId,
--        I.Int1,
--        I.Int2,
--        I.Int3,
--        I.Int4,
--        I.Address,
--        P.String1,
--        P.String2,
--        P.String3,
--        P.String4,
--        P.String5,
--        P.String6,
--        P.String7
-- from vItem I
--          join Payload P on I.Id = P.TxId
-- where I.Height is not null
--   and I.Height = (
--     select max(i_.Height)
--     from vItem i_
--     where i_.Int1 = I.Int1 -- RootTxId for all except for Scores
-- );
--
--
-- drop view if exists vWebUsers;
-- create view vWebUsers as
-- select WI.Id,
--        WI.Type,
--        WI.Hash,
--        WI.Time,
--        WI.Height,
--        WI.AddressId,
--        WI.Address,
--        WI.Int1    as Registration,
--        WI.Int2    as ReferrerId,
--        WI.String1 as Lang,
--        WI.String2 as Name,
--        WI.String3 as Avatar,
--        WI.String4 as About,
--        WI.String5 as Url,
--        WI.String6 as Pubkey,
--        WI.String7 as Donations
-- from vWebItem WI
-- where WI.Type in (100);
--
--
-- drop view if exists VideoServers;
-- create view VideoServers as
-- select t.Id,
--        t.Type,
--        t.Time,
--        t.Height,
--        t.AddressId,
--        t.Int1    as Registration,
--        t.String1 as Lang,
--        t.String2 as Name
-- from vWebItem t
-- where t.Type in (101);
--
--
-- drop view if exists MessageServers;
-- create view MessageServers as
-- select t.Id,
--        t.Type,
--        t.Time,
--        t.Height,
--        t.AddressId,
--        t.Int1    as Registration,
--        t.String1 as Lang,
--        t.String2 as Name
-- from vWebItem t
-- where t.Type in (102);
--
-- --------------------------------------------
drop view if exists vWebPosts;
create view vWebPosts as
select
       t.Hash,
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
     where t_.String2 = t.String2 and t_.Type =t.Type)
and t.Type = 200;

-- drop view if exists vWebPosts;
-- create view vWebPosts as
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
-- where WI.Type in (200);
--
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
select
       t.Hash,
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
     where t_.String2 = t.String2 and t_.Type =t.Type)
and t.Type =204;
--
-- drop view if exists vWebComments;
-- create view vWebComments as
-- select WI.Id,
--        WI.Hash,
--        WI.Time,
--        WI.Height,
--        WI.AddressId,
--        WI.Address,
--        WI.String1 as Lang,
--        WI.String2 as Message,
--        WI.Int1    as RootTxId,
--        WI.Int2    as PostTxId,
--        WI.Int3    as ParentTxId,
--        WI.Int4    as AnswerTxId
-- from vWebItem WI
-- where WI.Type in (204);
--
--
-- --------------------------------------------
drop view if exists vWebScorePosts;
create view vWebScorePosts as
select String1 as AddressHash,
       String2 as PostTxHash,
       Int1 as Value
from Transactions
where Type in (300);


drop view if exists vWebScoreComments;
create view vWebScoreComments as
select String1 as AddressHash,
       String2 as CommentTxHash,
       Int1 as Value
from Transactions
where Type in (301);

drop view if exists vWebScoresPosts;
create view vWebScoresPosts as
    select count(*) as cnt, avg(Int1) as average,String2 as PostTxHash from Transactions  where Type=300 group by String2;

drop view if exists vWebScoresComments;
create view vWebScoresComments as
    select count(*) as cnt, avg(Int1) as average,String2 as CommentTxHash from Transactions  where Type=301 group by String2;


-- drop view if exists vWebScorePosts;
-- create view vWebScorePosts as
-- select Int1 as PostTxId,
--        Int2 as Value
-- from vItem I
-- where I.Type in (300);
--
-- drop view if exists vWebScoreComments;
-- create view vWebScoreComments as
-- select Int1 as CommentTxId,
--        Int2 as Value
-- from vItem I
-- where I.Type in (301);
--
--

-- drop view if exists vWebSubscribes;
-- create view vWebSubscribes as
-- select WI.Type,
--        WI.AddressId,
--        WI.Address,
--        WI.Int1   as AddressToId,
--        A.Address as AddressTo
-- from vWebItem WI
--          join Addresses A ON A.Id = WI.Int1
-- where WI.Type in (302, 303, 304);
--
--
--
-- drop view if exists vWebBlockings;
-- create view vWebBlockings as
-- select WI.Type,
--        WI.AddressId,
--        WI.Address,
--        WI.Int1   as AddressToId,
--        A.Address as AddressTo
-- from vWebItem WI
--          join Addresses A ON A.Id = WI.Int1
-- where WI.Type in (305, 306);
--
--
--
drop view if exists Complains;
create view Complains as
select String1 as AddressHash,
       String2 as PostTxHash,
       Int1   as Reason
from Transactions
where Type in (307);

-- drop view if exists Complains;
-- create view Complains as
-- select WI.Type,
--        WI.AddressId,
--        WI.Address,
--        WI.Int1   as AddressToId,
--        A.Address as AddressTo,
--        WI.Int2   as Reason
-- from vWebItem WI
--          join Addresses A ON A.Id = WI.Int1
-- where WI.Type in (307);
--
--
-- vacuum;


---------------------------------------------------------
--               FULL TEXT SEARCH TABLES               --
---------------------------------------------------------
/*
DROP TABLE PayloadUsers_fts;
CREATE VIRTUAL TABLE PayloadUsers_fts USING fts5(
    TxHash UNINDEXED,
    Name,
    About
);

insert into PayloadUsers_fts
select
       t.Hash,
       p.String2 as Name, -- DECODE and UnEscape string
       p.String4 as About
from Transactions t
join Payload p on t.Hash = p.TxHash
where t.Type = 100;


select * from PayloadUsers_fts f, vWebUsers wu
where f.Name match 'loki*' and f.TxHash=wu.Hash
limit 60;
*/

/*
DROP TABLE PayloadPosts_fts;
CREATE VIRTUAL TABLE PayloadPosts_fts USING fts5(
    TxHash UNINDEXED,
    Caption,
    Message,
    Tags
);

insert into PayloadPosts_fts
select
       t.Hash,
       p.String2 as Caption,
       p.String3 as Message,
       p.String4 as Tags
from Transactions t
join Payload p on t.Hash = p.TxHash
where t.Type = 200;


select wu.* from PayloadPosts_fts f, vWebPosts wu
where f.message match 'Trump' and f.TxHash=wu.Hash
limit 60;*/

/*
DROP TABLE PayloadComments_fts;
CREATE VIRTUAL TABLE PayloadComments_fts USING fts5(
    TxHash UNINDEXED,
    Message
);

insert into PayloadComments_fts
select
       t.Hash,
       p.String2 as Message
from Transactions t
join Payload p on t.Hash = p.TxHash
where t.Type = 204;


select wc.* from PayloadComments_fts f, vWebComments wc
where f.message match 'nothing' and f.TxHash=wc.Hash
limit 60;*/
