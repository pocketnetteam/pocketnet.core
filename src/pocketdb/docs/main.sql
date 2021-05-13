drop table if exists Transactions;
create table Transactions
(
  Id        integer primary key,
  Type      int    not null,
  Hash      string not null,
  Time      int    not null,

  AddressId int    null,

  -- User.Registration
  -- Post.RootTxId
  -- Comment.RootTxId
  -- ScorePost.PostTxId
  -- ScoreComment.CommentTxId
  -- Subscribe.AddressToId
  -- Blocking.AddressToId
  -- Complain.PostTxId
  Int1      int    null,

  -- User.ReferrerId
  -- Post.RelayTxId
  -- Comment.PostTxId
  -- ScorePost.Value
  -- ScoreComment.Value
  -- Complain.Reason
  Int2      int    null,

  -- Comment.ParentTxId
  Int3      int    null,

  -- Comment.AnswerTxId
  Int4      int    null
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

drop index if exists Transactions_AddressId;
create index Transactions_AddressId on Transactions (AddressId);


--------------------------------------------
drop table if exists Chain;
create table Chain
(
  TxId   int not null, -- Transactions.Id
  Height int not null, -- Block height
  Number int not null,
  primary key (TxId, Height, Number)
);

--------------------------------------------
create table Addresses
(
  Id      integer primary key,
  Address string not null
);

--------------------------------------------
--               EXT TABLES               --
--------------------------------------------
drop table if exists Payload;
create table Payload
(
  TxId    int primary key, -- Transactions.Id

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
drop table if exists TxOutputs;
create table TxOutputs
(
  TxId   int not null, -- Transactions.Id
  Number int not null, -- Number in tx.vout
  Value  int not null, -- Amount
  primary key (TxId, Number)
);

drop index if exists TxOutput_TxSpentId;
create index TxOutput_TxSpentId on TxOutput (TxSpentId);


--------------------------------------------
drop table if exists TxOutputsDestinations;
create table TxOutputsDestinations
(
  TxId      int not null, -- TxOutput.TxId
  Number    int not null, -- TxOutput.Number
  AddressId int not null, -- Addresses.Id
  primary key (TxOutputId, AddressId)
);


--------------------------------------------
create table TxInputs
(
  TxId          int not null, -- Transactions.Id
  InputTxId     int not null, -- TxOutput.TxId
  InputTxNumber int not null  -- TxOutput.Number
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
left join Chain C on T.Id = C.TxId;

drop view if exists vItem;
create view vItem as
select t.Id,
       t.Type,
       t.Hash,
       t.Time,
       t.Height,
       t.Number,
       a.Id   as AddressId,
       a.Address,
       t.Int1,
       t.Int2,
       t.Int3,
       t.Int4
from vTransactions t
join Addresses a on a.Id = t.AddressId;

drop view if exists vUsers;
create view vUsers as
select Id,
       Hash,
       Time,
       Height,
       Number,
       AddressId,
       Address,
       Int1 as Registration,
       Int2 as ReferrerId
from vItem i
where i.Type in (100);



create view vWebItem as
select I.*, P.String1, P.String2, P.String3, P.String4, P.String5, P.String6, P.String7
from vItem I
join Payload P on I.Id = P.TxId
where I.Height is not null
  and I.Height = (
  select max( i_.Height )
  from vItem i_
  where i_.RootTxId = I.RootTxId
);



drop view if exists vWebUsers;
create view vWebUsers as
select WI.*,
       WI.Int1 as Registration,
       WI.Int2 as ReferrerId,
       WI.String1 as Lang,
       WI.String2 as Name,
       WI.String3 as Avatar,
       WI.String4 as About,
       WI.String5 as Url,
       WI.String6 as Pubkey,
       WI.String7 as Donations
from vWebItem WI
where WI.Type in (100);


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

/*
drop view if exists VideoServers;
create view VideoServers as
select t.Id,
       t.Hash,
       t.Time,
       t.Height,
       t.Number,
       a.Id   as AddressId,
       a.Address,
       t.Int1 as Registration,
       t.Int2 as Referrer
from vTransactions t
join Addresses a on a.Id = t.AddressId
where t.Type in (101);
*/

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
drop view if exists vWebPosts;
create view vWebPosts as
select 
 *
  String1 as Lang,
  String2 as Caption,

  -- Post.Message
  String3 string null,

  -- Post.Tags JSON
  String4 string null,

  -- Post.Images JSON
  String5 string null,

  -- Post.Settings JSON
  String6 string null
from vWebItem WI
where WI.Type in (200);


drop view if exists vWebVideos;
create view vWebVideos as
select 
 *
  String1 as Lang,
  String2 as Caption,

  -- Post.Message
  String3 string null,

  -- Post.Tags JSON
  String4 string null,

  -- Post.Images JSON
  String5 string null,

  -- Post.Settings JSON
  String6 string null
from vWebItem WI
where WI.Type in (201);


drop view if exists vWebTranslates;
create view vWebTranslates as
select  *
  String1 as Lang,
  String2 as Caption,

  -- Post.Message
  String3 string null,

  -- Post.Tags JSON
  String4 string null,

  -- Post.Images JSON
  String5 string null,

  -- Post.Settings JSON
  String6 string null
from vWebItem WI
where WI.Type in (202);

drop view if exists vWebServerPings;
create view vWebServerPings as
 select *
  String1 as Lang,
  String2 as Caption,
  String3 as Message,
  String4 as Tags,

  -- Post.Images JSON
  String5 string null,

  -- Post.Settings JSON
  String6 string null
from vWebItem WI
where WI.Type in (203);


drop view if exists vWebComments;
create view vWebComments as
 select *
  String1 as Lang,
  String2 as Message,
  Int1 as RootTxId,
  Int2 as PostTxId,
  Int3 as ParentTxId,
  Int4 as AnswerTxId
from vWebItem WI
where WI.Type in (204);


--------------------------------------------
drop view if exists vWebScorePosts;
create view vWebScorePosts as
 select 
  Int1 as PostTxId,
  Int2 as Value
from vWebItem WI
where WI.Type in (300);

drop view if exists vWebScoreComments;
create view vWebScoreComments as
 select 
  Int1 as CommentTxId,
  Int2 as Value
from vWebItem WI
where WI.Type in (301);


drop view if exists vWebSubscribes;
create view vWebSubscribes as
 select *
  Type,
  Int1 as AddressToId
  a.Address as AddressTo
from vWebItem WI
join Addresses a ON a.Id=WI.Int1
where WI.Type in (302, 303, 304);



drop view if exists vWebBlockings;
create view vWebBlockings as
 select *
  Type,
  Int1 as AddressToId
  a.Address as AddressTo
from vWebItem WI
join Addresses a ON a.Id=WI.Int1
where WI.Type in (305, 306);



drop view if exists Complains;
create view Complains as
 select *
  Type,
  Int1 as AddressToId
  a.Address as AddressTo,
  WI.Int2    as Reason
from vWebItem WI
join Addresses a ON a.Id=WI.Int1
where WI.Type in (307);


vacuum;
