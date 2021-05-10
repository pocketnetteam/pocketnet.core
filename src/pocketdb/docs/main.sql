drop table if exists Transactions;
create table Transactions
(
    Id     int    not null primary key autoincrement,
    Type   int    not null,
    Hash   string not null,
    Height int    null,
    Number int    null,
    Time   int    not null,

    -- User.Id
    -- Post.Id
    -- Comment.RootTxId
    -- ScorePost.PostTxId
    -- ScoreComment.CommentTxId
    -- Complain.Reason
    -- Subscribe.AddressToId
    -- Blocking.AddressToId
    Int1   int    null,

    -- User.Registration
    -- Post.RootTxId
    -- Comment.PostTxId
    -- Complain.PostTxId
    -- ScorePost.Value
    -- ScoreComment.Value
    Int2   int    null,

    -- User.ReferrerId
    -- Post.RelayTxId
    -- Comment.ParentTxId
    Int3   int    null,

    -- Comment.AnswerTxId
    Int4   int    null
);

--------------------------------------------
drop index if exists Transactions_Type;
create index Transactions_Type on Transactions (Type);

drop index if exists Transactions_Hash;
create index Transactions_Hash on Transactions (Hash);

drop index if exists Transactions_Height;
create index Transactions_Height on Transactions (Height);

drop index if exists Transactions_Number;
create index Transactions_Number on Transactions (Number);

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
--               EXT TABLES               --
--------------------------------------------
drop table if exists Payload;
create table Payload
(
    TxId    string not null primary key,

    -- User.Name
    String1 string null,

    -- User.Lang
    -- Post.Lang
    -- Comment.Lang
    String2 string not null,
    String3 string not null
);

--------------------------------------------
create table TxOutput
(
    TxId      int not null, -- Transactions.Id
    Number    int not null, -- Number in tx.vout
    Value     int not null, -- Amount
    TxSpentId int null,     -- from next tx.vin
    primary key (TxId, Number)
);

drop index if exists TxOutput_TxSpentId;
create index TxOutput_TxSpentId on TxOutput (TxSpentId);


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
