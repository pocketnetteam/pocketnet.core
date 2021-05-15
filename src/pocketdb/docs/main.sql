drop table if exists Transactions;
create table Transactions
(
    Id        integer primary key,
    Type      int    not null,
    Hash      string not null,
    Time      int    not null,

    AddressId int    null,

    -- User.RegistrationTxId
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

drop index if exists Transactions_AddressId;
create index Transactions_AddressId on Transactions (AddressId);

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
    TxId   int not null, -- Transactions.Id
    Height int not null, -- Block height
    primary key (TxId, Height)
);

--------------------------------------------
drop table if exists Addresses;
create table Addresses
(
    Id      integer primary key,
    Address string not null
);
create unique index if not exists Addresses_Address on Addresses (Address);

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
    -- Post.Url
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

--------------------------------------------
drop table if exists TxOutputsDestinations;
create table TxOutputsDestinations
(
    TxId      int not null, -- TxOutput.TxId
    Number    int not null, -- TxOutput.Number
    AddressId int not null, -- Addresses.Id
    primary key (TxId, Number, AddressId)
);


--------------------------------------------
drop table if exists TxInputs;
create table TxInputs
(
    TxId          int not null, -- Transactions.Id
    InputTxId     int not null, -- TxOutput.TxId
    InputTxNumber int not null, -- TxOutput.Number
    primary key (TxId, InputTxId, InputTxNumber)
);


--------------------------------------------
drop table if exists Ratings;
create table Ratings
(
    Type   int not null,
    Height int not null,
    Id     int not null,
    Value  int not null,

    primary key (Type, Height, Id)
);


--------------------------------------------
--                 VIEWS                  --
--------------------------------------------
drop view if exists vTransactions;
create view vTransactions as
select T.Id,
       T.Type,
       T.Hash,
       T.Time,
       T.AddressId,
       T.Int1,
       T.Int2,
       T.Int3,
       T.Int4,
       C.Height
from Transactions T
         left join Chain C on T.Id = C.TxId;

drop view if exists vItem;
create view vItem as
select t.Id,
       t.Type,
       t.Hash,
       t.Time,
       t.Height,
       t.AddressId,
       t.Int1,
       t.Int2,
       t.Int3,
       t.Int4,
       a.Address
from vTransactions t
         join Addresses a on a.Id = t.AddressId;

drop view if exists vUsers;
create view vUsers as
select Id,
       Hash,
       Time,
       Height,
       AddressId,
       Address,
       Int1 as Registration,
       Int2 as ReferrerId
from vItem i
where i.Type in (100);



drop view if exists vWebItem;
create view vWebItem as
select I.Id,
       I.Type,
       I.Hash,
       I.Time,
       I.Height,
       I.AddressId,
       I.Int1,
       I.Int2,
       I.Int3,
       I.Int4,
       I.Address,
       P.String1,
       P.String2,
       P.String3,
       P.String4,
       P.String5,
       P.String6,
       P.String7
from vItem I
         join Payload P on I.Id = P.TxId
where I.Height is not null
  and I.Height = (
    select max(i_.Height)
    from vItem i_
    where i_.Int1 = I.Int1 -- RootTxId for all except for Scores
);


drop view if exists vWebUsers;
create view vWebUsers as
select WI.Id,
       WI.Type,
       WI.Hash,
       WI.Time,
       WI.Height,
       WI.AddressId,
       WI.Address,
       WI.Int1    as Registration,
       WI.Int2    as ReferrerId,
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
select t.Id,
       t.Type,
       t.Time,
       t.Height,
       t.AddressId,
       t.Int1    as Registration,
       t.String1 as Lang,
       t.String2 as Name
from vWebItem t
where t.Type in (101);


drop view if exists MessageServers;
create view MessageServers as
select t.Id,
       t.Type,
       t.Time,
       t.Height,
       t.AddressId,
       t.Int1    as Registration,
       t.String1 as Lang,
       t.String2 as Name
from vWebItem t
where t.Type in (102);

--------------------------------------------
drop view if exists vWebPosts;
create view vWebPosts as
select WI.Id,
       WI.Hash,
       WI.Time,
       WI.Height,
       WI.AddressId,
       WI.Int1    as RootTxId,
       WI.Int2    as RelayTxId,
       WI.Address,
       WI.String1 as Lang,
       WI.String2 as Caption,
       WI.String3 as Message,
       WI.String4 as Tags,
       WI.String5 as Images,
       WI.String6 as Settings,
       WI.String7 as Url
from vWebItem WI
where WI.Type in (200);


drop view if exists vWebVideos;
create view vWebVideos as
select WI.Id,
       WI.Hash,
       WI.Time,
       WI.Height,
       WI.AddressId,
       WI.Int1    as RootTxId,
       WI.Int2    as RelayTxId,
       WI.Address,
       WI.String1 as Lang,
       WI.String2 as Caption,
       WI.String3 as Message,
       WI.String4 as Tags,
       WI.String5 as Images,
       WI.String6 as Settings,
       WI.String7 as Url
from vWebItem WI
where WI.Type in (201);


drop view if exists vWebTranslates;
create view vWebTranslates as
select WI.Id,
       WI.Hash,
       WI.Time,
       WI.Height,
       WI.AddressId,
       WI.Int1    as RootTxId,
       WI.Int2    as RelayTxId,
       WI.Address,
       WI.String1 as Lang,
       WI.String2 as Caption,
       WI.String3 as Message,
       WI.String4 as Tags
from vWebItem WI
where WI.Type in (202);

drop view if exists vWebServerPings;
create view vWebServerPings as
select WI.Id,
       WI.Hash,
       WI.Time,
       WI.Height,
       WI.AddressId,
       WI.Int1    as RootTxId,
       WI.Int2    as RelayTxId,
       WI.Address,
       WI.String1 as Lang,
       WI.String2 as Caption,
       WI.String3 as Message,
       WI.String4 as Tags
from vWebItem WI
where WI.Type in (203);


drop view if exists vWebComments;
create view vWebComments as
select WI.Id,
       WI.Hash,
       WI.Time,
       WI.Height,
       WI.AddressId,
       WI.Address,
       WI.String1 as Lang,
       WI.String2 as Message,
       WI.Int1    as RootTxId,
       WI.Int2    as PostTxId,
       WI.Int3    as ParentTxId,
       WI.Int4    as AnswerTxId
from vWebItem WI
where WI.Type in (204);


--------------------------------------------
drop view if exists vWebScorePosts;
create view vWebScorePosts as
select Int1 as PostTxId,
       Int2 as Value
from vItem I
where I.Type in (300);

drop view if exists vWebScoreComments;
create view vWebScoreComments as
select Int1 as CommentTxId,
       Int2 as Value
from vItem I
where I.Type in (301);


drop view if exists vWebSubscribes;
create view vWebSubscribes as
select WI.Type,
       WI.AddressId,
       WI.Address,
       WI.Int1   as AddressToId,
       A.Address as AddressTo
from vWebItem WI
         join Addresses A ON A.Id = WI.Int1
where WI.Type in (302, 303, 304);



drop view if exists vWebBlockings;
create view vWebBlockings as
select WI.Type,
       WI.AddressId,
       WI.Address,
       WI.Int1   as AddressToId,
       A.Address as AddressTo
from vWebItem WI
         join Addresses A ON A.Id = WI.Int1
where WI.Type in (305, 306);



drop view if exists Complains;
create view Complains as
select WI.Type,
       WI.AddressId,
       WI.Address,
       WI.Int1   as AddressToId,
       A.Address as AddressTo,
       WI.Int2   as Reason
from vWebItem WI
         join Addresses A ON A.Id = WI.Int1
where WI.Type in (307);


vacuum;
