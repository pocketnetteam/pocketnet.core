drop table if exists Transactions;
create table Transactions
(
    TxType  int    not null,
    TxId    string not null,
    TxTime  int    not null,
    Address string not null,

    -- User.Id
    -- Post.Id
    -- ScorePost.Value
    -- ScoreComment.Value
    -- Complain.Reason
    Int1    int null,

    -- User.Registration
    Int2    int null,

    -- Empty
    Int3    int null,

    -- Empty
    Int4    int null,

    -- Empty
    Int5    int null,

    -- User.Lang
    -- Post.Lang
    -- Comment.Lang
    -- Subscribe.AddressTo
    -- Blocking.AddressTo
    -- Complain.PostTxId
    -- ScorePost.PostTxId
    -- ScoreComment.CommentTxId
    String1 string null,

    -- User.Name
    -- Post.Root
    -- Comment.RootTxId
    String2 string null,

    -- User.Referrer
    -- Post.RelayTxId
    -- Comment.PostTxId
    String3 string null,

    -- Comment.ParentTxId
    String4 string null,

    -- Comment.AnswerTxId
    String5 string null,

    primary key (TxId)
);

--------------------------------------------
drop index if exists Transactions_TxType;
create index Transactions_TxType on Transactions (TxType);

drop index if exists Transactions_TxTime;
create index Transactions_TxTime on Transactions (TxTime desc);

drop index if exists Transactions_Address;
create index Transactions_Address on Transactions (Address);

drop index if exists Transactions_Int1;
create index Transactions_Int1 on Transactions (Int1);

drop index if exists Transactions_Int2;
create index Transactions_Int2 on Transactions (Int2);

drop index if exists Transactions_Int3;
create index Transactions_Int3 on Transactions (Int3);

drop index if exists Transactions_Int4;
create index Transactions_Int4 on Transactions (Int4);

drop index if exists Transactions_Int5;
create index Transactions_Int5 on Transactions (Int5);

drop index if exists Transactions_String1;
create index Transactions_String1 on Transactions (String1);

drop index if exists Transactions_String2;
create index Transactions_String2 on Transactions (String2);

drop index if exists Transactions_String3;
create index Transactions_String3 on Transactions (String3);

drop index if exists Transactions_String4;
create index Transactions_String4 on Transactions (String4);

drop index if exists Transactions_String5;
create index Transactions_String5 on Transactions (String5);


--------------------------------------------
--               EXT TABLES               --
--------------------------------------------

drop table if exists Chain;
create table Chain
(
    TxId  string not null,
    Block int    not null,

    primary key (TxId)
);

drop index if exists Chain_Block;
create index Chain_Block on Chain (Block);

--------------------------------------------
drop table if exists Payload;
create table Payload
(
    TxId string not null,
    Data blob   not null,

    primary key (TxId)
);

--------------------------------------------
drop table if exists Utxo;
create table Utxo
(
    TxId       string not null,
    TxOut      int    not null,
    TxTime     int    not null,
    Block      int    not null,
    BlockSpent int null,
    Address    string not null,
    Amount     int    not null,

    primary key (TxId, TxOut)
);

--------------------------------------------
drop table if exists Ratings;
create table Ratings
(
    RatingType int not null,
    Block      int not null,
    Key        int not null,
    Value      int not null,

    primary key (Block, RatingType, Key)
);




--------------------------------------------
--                VIEWS                   --
--------------------------------------------
drop view if exists Users;
create view Users as
select t.TxType,
       t.TxId,
       t.TxTime,
       c.Block,
       t.Address,
       t.Int1    as Id,
       t.Int2    as Registration,
       t.String1 as Lang,
       t.String2 as Name,
       t.String3 as Referrer
from Transactions t
         left join Chain c on c.TxId = t.TxId
where t.TxType in (100);

drop view if exists VideoServers;
create view VideoServers as
select t.TxType,
       t.TxId,
       t.TxTime,
       c.Block,
       t.Address,
       t.Int1    as Id,
       t.Int2    as Registration,
       t.String1 as Lang,
       t.String2 as Name
from Transactions t
         left join Chain c on c.TxId = t.TxId
where t.TxType in (101);

drop view if exists MessageServers;
create view MessageServers as
select t.TxType,
       t.TxId,
       t.TxTime,
       c.Block,
       t.Address,
       t.Int1    as Id,
       t.Int2    as Registration,
       t.String1 as Lang,
       t.String2 as Name
from Transactions t
         left join Chain c on c.TxId = t.TxId
where t.TxType in (102);

--------------------------------------------
drop view if exists Posts;
create view Posts as
select t.TxType,
       t.TxId,
       t.TxTime,
       c.Block,
       t.Address,
       t.Int1 as Id,
       t.String1 as Lang,
       t.String2 as RootTxId,
       t.String3 as RelayTxId
from Transactions t
         left join Chain c on c.TxId = t.TxId
where t.TxType in (200);

drop view if exists Videos;
create view Videos as
select t.TxType,
       t.TxId,
       t.TxTime,
       c.Block,
       t.Address,
       t.String1 as Lang,
       t.String2 as RootTxId,
       t.String3 as RelayTxId
from Transactions t
         left join Chain c on c.TxId = t.TxId
where t.TxType in (201);

drop view if exists Translates;
create view Translates as
select t.TxType,
       t.TxId,
       t.TxTime,
       c.Block,
       t.Address,
       t.String1 as Lang,
       t.String2 as RootTxId,
       t.String3 as RelayTxId
from Transactions t
         left join Chain c on c.TxId = t.TxId
where t.TxType in (202);

drop view if exists ServerPings;
create view ServerPings as
select t.TxType,
       t.TxId,
       t.TxTime,
       c.Block,
       t.Address,
       t.String1 as Lang,
       t.String2 as RootTxId,
       t.String3 as RelayTxId
from Transactions t
         left join Chain c on c.TxId = t.TxId
where t.TxType in (203);

drop view if exists Comments;
create view Comments as
select t.TxType,
       t.TxId,
       t.TxTime,
       c.Block,
       t.Address,
       t.String1 as Lang,
       t.String2 as RootTxId,
       t.String3 as PostTxId,
       t.String4 as ParentTxId,
       t.String5 as AnswerTxId
from Transactions t
         left join Chain c on c.TxId = t.TxId
where t.TxType in (204);

--------------------------------------------
drop view if exists ScorePosts;
create view ScorePosts as
select t.TxType,
       t.TxId,
       t.TxTime,
       c.Block,
       t.Address,
       t.Int1 as Value,
       t.String1 as PostTxId
from Transactions t
         left join Chain c on c.TxId = t.TxId
where t.TxType in (300);

drop view if exists ScoreComments;
create view ScoreComment ass
select t.TxType,
       t.TxId,
       t.TxTime,
       c.Block,
       t.Address,
       t.Int1 as Value,
       t.String1 as CommentTxId
from Transactions t
         left join Chain c on c.TxId = t.TxId
where t.TxType in (301);


drop view if exists Subscribes;
create view Subscribes as
select t.TxType,
       t.TxId,
       t.TxTime,
       c.Block,
       t.Address,
       t.String1 as AddressTo
from Transactions t
         left join Chain c on c.TxId = t.TxId
where t.TxType in (302, 303, 304);


drop view if exists Blockings;
create view Blockings as
select t.TxType,
       t.TxId,
       t.TxTime,
       c.Block,
       t.Address,
       t.String1 as AddressTo
from Transactions t
         left join Chain c on c.TxId = t.TxId
where t.TxType in (305, 306);


drop view if exists Complains;
create view Complains as
select t.TxType,
       t.TxId,
       t.TxTime,
       c.Block,
       t.Address,
       t.String1 as AddressTo,
       t.Int1 as Reason
from Transactions t
         left join Chain c on c.TxId = t.TxId
where t.TxType in (307);
