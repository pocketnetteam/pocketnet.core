drop table if exists Transactions;
create table Transactions
(
    Id   int    not null primary key,
    Type int    not null,
    Hash string not null,
    Time int    not null,

    AddressId int null,

    -- User.Registration
    -- Post.RootTxId
    -- Comment.RootTxId
    -- ScorePost.PostTxId
    -- ScoreComment.CommentTxId
    -- Subscribe.AddressToId
    -- Blocking.AddressToId
    -- Complain.PostTxId
    Int1 int    null,

    -- User.ReferrerId
    -- Post.RelayTxId
    -- Comment.PostTxId
    -- ScorePost.Value
    -- ScoreComment.Value
    -- Complain.Reason
    Int2 int    null,

    -- Comment.ParentTxId
    Int3 int    null,
    
    -- Comment.AnswerTxId
    Int4 int    null
);

--------------------------------------------
drop index if exists Transactions_Type;
create index Transactions_Type on Transactions (Type);

drop index if exists Transactions_Hash;
create index Transactions_Hash on Transactions (Hash);

drop index if exists Transactions_Time;
create index Transactions_Time on Transactions (Time);

drop index if exists Transactions_Int1;
create index Transactions_Int1 on Transactions (Int1);

drop index if exists Transactions_Int2;
create index Transactions_Int2 on Transactions (Int2);

drop index if exists Transactions_Int3;
create index Transactions_Int3 on Transactions (Int3);

drop index if exists Transactions_Int4;
create index Transactions_Int4 on Transactions (Int4);


--------------------------------------------
drop table if exists Chain;
create table Chain
(
    TxId   int not null,
    Height int not null,
    Number int not null,

    primary key (TxId, Height, Number)
);

--------------------------------------------
create table Addresses
(
    Id      int    not null primary key,
    Address string not null
);

--------------------------------------------
--               EXT TABLES               --
--------------------------------------------
drop table if exists Payload;
create table Payload
(
    TxId    string not null primary key,

    -- User.Birthday
    -- Complain.Reason
    Int1 int null,

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
    String7 string null
);


--------------------------------------------
drop table if exists TxOutput;
create table TxOutput
(
    TxId      int not null, -- Transactions.Id
    Number    int not null, -- Number in tx.vout
    Value     int not null, -- Amount
    primary key (TxId, Number)
);

drop index if exists TxOutput_TxSpentId;
create index TxOutput_TxSpentId on TxOutput (TxSpentId);


--------------------------------------------
/*
insert into TxOutput (TxId, Number, Value)
values (
    (select t.Id from Transactions t where t.Hash='sdfsdf'), 1, 2
);
*/

create table TxInput
(
    TxId                int not null,
    InputTxId           int not null,
    InputTxOutputNumber int not null
);

--------------------------------------------
create table TxOutputDestinations
(
    TxId      int not null, -- Transactions.Id
    Number    int not null, -- Number in tx.vout
    AddressId int not null,
    primary key (TxId, Number, AddressId)
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
--                 VIEWS                  --
--------------------------------------------
create view vTransactions as
select T.*, C.Height, C.Number
from Transactions T
         left join Chain C on T.Id = C.TxId
;


drop view if exists vUsers;
create view vUsers as
select t.Id,
       t.Hash,
       t.Time,
       t.Height,
       t.Number,
       a.Id   as AddressId,
       a.Address,
       t.Int2 as Registration,
       t.Int3 as Referrer
from vTransactions t
         join Addresses a on a.Id = t.Int1
where t.Type in (100);


drop view if exists vWebUsers;
create view vWebUsers as
select U.*,
       P.String1 as Title,
       P.String2 as About
from vUsers U
         join Payload P on U.Id = P.TxId
where U.Height is not null
  and U.Height = (select max(u_.Height) from vUsers u_ where u_.AddressId = U.AddressId)
;


drop view if exists VideoServers;
create view VideoServers as
select t.TxType,
       t.TxId,
       t.TxTime,
       t.Block,
       t.TxOut,
       t.Address,
       t.Int1    as Registration,
       t.String1 as Lang,
       t.String2 as Name
from Transactions t
where t.TxType in (101);

drop view if exists MessageServers;
create view MessageServers as
select t.TxType,
       t.TxId,
       t.TxTime,
       t.Block,
       t.TxOut,
       t.Address,
       t.Int1    as Registration,
       t.String1 as Lang,
       t.String2 as Name
from Transactions t
where t.TxType in (102);

--------------------------------------------
drop view if exists Posts;
create view Posts as
select t.TxType,
       t.TxId,
       t.TxTime,
       t.Block,
       t.TxOut,
       t.Address,
       t.String1 as Lang,
       t.String2 as RootTxId,
       t.String3 as RelayTxId
from Transactions t
where t.TxType in (200);

drop view if exists Videos;
create view Videos as
select t.TxType,
       t.TxId,
       t.TxTime,
       t.Block,
       t.TxOut,
       t.Address,
       t.String1 as Lang,
       t.String2 as RootTxId,
       t.String3 as RelayTxId
from Transactions t
where t.TxType in (201);

drop view if exists Translates;
create view Translates as
select t.TxType,
       t.TxId,
       t.TxTime,
       t.Block,
       t.TxOut,
       t.Address,
       t.String1 as Lang,
       t.String2 as RootTxId,
       t.String3 as RelayTxId
from Transactions t
where t.TxType in (202);

drop view if exists ServerPings;
create view ServerPings as
select t.TxType,
       t.TxId,
       t.TxTime,
       t.Block,
       t.TxOut,
       t.Address,
       t.String1 as Lang,
       t.String2 as RootTxId,
       t.String3 as RelayTxId
from Transactions t
where t.TxType in (203);

drop view if exists Comments;
create view Comments as
select t.TxType,
       t.TxId,
       t.TxTime,
       t.Block,
       t.TxOut,
       t.Address,
       t.String1 as Lang,
       t.String2 as RootTxId,
       t.String3 as PostTxId,
       t.String4 as ParentTxId,
       t.String5 as AnswerTxId
from Transactions t
where t.TxType in (204);

--------------------------------------------
drop view if exists ScorePosts;
create view ScorePosts as
select t.TxType,
       t.TxId,
       t.TxTime,
       t.Block,
       t.TxOut,
       t.Address,
       t.Int1    as Value,
       t.String1 as PostTxId
from Transactions t
where t.TxType in (300);

drop view if exists ScoreComments;
create view ScoreComments as
select t.TxType,
       t.TxId,
       t.TxTime,
       t.Block,
       t.TxOut,
       t.Address,
       t.Int1    as Value,
       t.String1 as CommentTxId
from Transactions t
where t.TxType in (301);


drop view if exists Subscribes;
create view Subscribes as
select t.TxType,
       t.TxId,
       t.TxTime,
       t.Block,
       t.TxOut,
       t.Address,
       t.String1 as AddressTo
from Transactions t
where t.TxType in (302, 303, 304);


drop view if exists Blockings;
create view Blockings as
select t.TxType,
       t.TxId,
       t.TxTime,
       t.Block,
       t.TxOut,
       t.Address,
       t.String1 as AddressTo
from Transactions t
where t.TxType in (305, 306);


drop view if exists Complains;
create view Complains as
select t.TxType,
       t.TxId,
       t.TxTime,
       t.Block,
       t.TxOut,
       t.Address,
       t.String1 as AddressTo,
       t.Int1    as Reason
from Transactions t
where t.TxType in (307);






vacuum;