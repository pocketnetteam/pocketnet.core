// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 Bitcoin developers
// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0


#ifndef POCKETDB_SQLITEDATABASE_H
#define POCKETDB_SQLITEDATABASE_H

#include "logging.h"
#include "sqlite3.h"
#include "sync.h"
#include "tinyformat.h"

#include <iostream>
#include <filesystem>

namespace PocketDb
{
    namespace fs = std::filesystem;

    class SQLiteDatabase
    {
    private:
        std::string m_dir_path;
        std::string m_file_path;
        bool m_is_first_init;

        void Cleanup() noexcept
        {
            Close();

            //LOCK(g_sqlite_mutex);
            if (--g_sqlite_count == 0)
            {
                int ret = sqlite3_shutdown();
                if (ret != SQLITE_OK)
                {
                    LogPrintf("%s: %d; Failed to shutdown SQLite: %s\n", __func__, ret, sqlite3_errstr(ret));
                }
            }
        }

        Mutex g_sqlite_mutex;
        int g_sqlite_count = 0; //GUARDED_BY(g_sqlite_mutex)

        static void ErrorLogCallback(void *arg, int code, const char *msg)
        {
            // From sqlite3_config() documentation for the SQLITE_CONFIG_LOG option:
            // "The void pointer that is the second argument to SQLITE_CONFIG_LOG is passed through as
            // the first parameter to the application-defined logger function whenever that function is
            // invoked."
            // Assert that this is the case:
            assert(arg == nullptr);
            LogPrintf("%s: %d; Message: %s\n", __func__, code, msg);
        }

        bool TryCreateDbIfNotExists() {
            if (fs::exists(m_file_path)) {
                return true;
            }

            // TODO (joni): need check also db - maybe structure changed?
            m_is_first_init = true;

            if (!fs::exists(m_dir_path)) {
                fs::create_directory(m_dir_path);
            }

            return true;
        }

        bool CreateStructure() const {
            std::string generate_sql = R"sql(

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


            )sql";

            BeginTransaction();

            try {
                char* errMsg = nullptr;
                size_t pos;
                std::string token;

                while ((pos = generate_sql.find(';')) != std::string::npos) {
                    token = generate_sql.substr(0, pos);

                    if (sqlite3_exec(m_db, token.c_str(), nullptr, nullptr, &errMsg) != SQLITE_OK) {
                        throw std::runtime_error("Failed to create init database");
                    }

                    generate_sql.erase(0, pos + 1);
                }

                CommitTransaction();
            }
            catch (const std::exception&) {
                AbortTransaction();
                LogPrintf("FFailed to create init database structure", __func__);
                return false;
            }

            return true;
        }

    public:
        sqlite3 *m_db{nullptr};

        SQLiteDatabase() = default;

        void Init(const std::string &dir_path, const std::string &file_path)
        {
            m_dir_path = dir_path;
            m_file_path = file_path;

            if (++g_sqlite_count == 1)
            {
                // Setup logging
                int ret = sqlite3_config(SQLITE_CONFIG_LOG, ErrorLogCallback, nullptr);
                if (ret != SQLITE_OK)
                {
                    throw std::runtime_error(
                        strprintf("%s: %sd Failed to setup error log: %s\n", __func__, ret, sqlite3_errstr(ret)));
                }
                // Force serialized threading mode
                ret = sqlite3_config(SQLITE_CONFIG_SERIALIZED);
                if (ret != SQLITE_OK)
                {
                    throw std::runtime_error(
                        strprintf("%s: %d; Failed to configure serialized threading mode: %s\n",
                            __func__, ret, sqlite3_errstr(ret)));
                }

                TryCreateDbIfNotExists();
            }
            int ret = sqlite3_initialize(); // This is a no-op if sqlite3 is already initialized
            if (ret != SQLITE_OK)
            {
                throw std::runtime_error(
                    strprintf("%s: %d; Failed to initialize SQLite: %s\n", __func__, ret, sqlite3_errstr(ret)));
            }

            try
            {
                Open();
            } catch (const std::runtime_error &)
            {
                // If open fails, cleanup this object and rethrow the exception
                Cleanup();
                throw;
            }
        }

        void Open()
        {
            int flags = SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE; //SQLITE_OPEN_FULLMUTEX | SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE;

            if (m_db == nullptr)
            {
                int ret = sqlite3_open_v2(m_file_path.c_str(), &m_db, flags, nullptr);
                if (ret != SQLITE_OK)
                    throw std::runtime_error(strprintf("%s: %d; Failed to open database: %s\n",
                        __func__, ret, sqlite3_errstr(ret)));

                if (m_is_first_init) {
                    if (!CreateStructure()) {
                        throw std::runtime_error(strprintf("%s: Failed to create database\n",
                            __func__));
                    }

                    m_is_first_init = false;
                }
            }

            if (sqlite3_db_readonly(m_db, "main") != 0)
                throw std::runtime_error("Database opened in readonly");

            if (sqlite3_exec(m_db, "PRAGMA journal_mode = wal;", nullptr, nullptr, nullptr) != 0)
                throw std::runtime_error("Failed apply journal_mode = wal");

            //    // Acquire an exclusive lock on the database
            //    // First change the locking mode to exclusive
            //    int ret = sqlite3_exec(m_db, "PRAGMA locking_mode = exclusive", nullptr, nullptr, nullptr);
            //    if (ret != SQLITE_OK) {
            //        throw std::runtime_error(strprintf("SQLiteDatabase: Unable to change database locking mode to exclusive: %s\n", sqlite3_errstr(ret)));
            //    }
            //    // Now begin a transaction to acquire the exclusive lock. This lock won't be released until we close because of the exclusive locking mode.
            //    ret = sqlite3_exec(m_db, "BEGIN EXCLUSIVE TRANSACTION", nullptr, nullptr, nullptr);
            //    if (ret != SQLITE_OK) {
            //        throw std::runtime_error("SQLiteDatabase: Unable to obtain an exclusive lock on the database, is it being used by another bitcoind?\n");
            //    }
            //    ret = sqlite3_exec(m_db, "COMMIT", nullptr, nullptr, nullptr);
            //    if (ret != SQLITE_OK) {
            //        throw std::runtime_error(strprintf("SQLiteDatabase: Unable to end exclusive lock transaction: %s\n", sqlite3_errstr(ret)));
            //    }
            //
            //    // Enable fullfsync for the platforms that use it
            //    ret = sqlite3_exec(m_db, "PRAGMA fullfsync = true", nullptr, nullptr, nullptr);
            //    if (ret != SQLITE_OK) {
            //        throw std::runtime_error(strprintf("SQLiteDatabase: Failed to enable fullfsync: %s\n", sqlite3_errstr(ret)));
            //    }
            //
            //    // Make the table for our key-value pairs
            //    // First check that the main table exists
            //    sqlite3_stmt* check_main_stmt{nullptr};
            //    ret = sqlite3_prepare_v2(m_db, "SELECT name FROM sqlite_master WHERE type='table' AND name='main'", -1, &check_main_stmt, nullptr);
            //    if (ret != SQLITE_OK) {
            //        throw std::runtime_error(strprintf("SQLiteDatabase: Failed to prepare statement to check table existence: %s\n", sqlite3_errstr(ret)));
            //    }
            //    ret = sqlite3_step(check_main_stmt);
            //    if (sqlite3_finalize(check_main_stmt) != SQLITE_OK) {
            //        throw std::runtime_error(strprintf("SQLiteDatabase: Failed to finalize statement checking table existence: %s\n", sqlite3_errstr(ret)));
            //    }
            //    bool table_exists;
            //    if (ret == SQLITE_DONE) {
            //        table_exists = false;
            //    } else if (ret == SQLITE_ROW) {
            //        table_exists = true;
            //    } else {
            //        throw std::runtime_error(strprintf("SQLiteDatabase: Failed to execute statement to check table existence: %s\n", sqlite3_errstr(ret)));
            //    }
            //
            //    // Do the db setup things because the table doesn't exist only when we are creating a new wallet
            //    if (!table_exists) {
            //        ret = sqlite3_exec(m_db, "CREATE TABLE main(key BLOB PRIMARY KEY NOT NULL, value BLOB NOT NULL)", nullptr, nullptr, nullptr);
            //        if (ret != SQLITE_OK) {
            //            throw std::runtime_error(strprintf("SQLiteDatabase: Failed to create new database: %s\n", sqlite3_errstr(ret)));
            //        }

            // Set the application id
            //        uint32_t app_id = ReadBE32(Params().MessageStart());
            //        std::string set_app_id = strprintf("PRAGMA application_id = %d", static_cast<int32_t>(app_id));
            //        ret = sqlite3_exec(m_db, set_app_id.c_str(), nullptr, nullptr, nullptr);
            //        if (ret != SQLITE_OK) {
            //            throw std::runtime_error(strprintf("SQLiteDatabase: Failed to set the application id: %s\n", sqlite3_errstr(ret)));
            //        }

            // Set the user version
            //        std::string set_user_ver = strprintf("PRAGMA user_version = %d", WALLET_SCHEMA_VERSION);
            //        ret = sqlite3_exec(m_db, set_user_ver.c_str(), nullptr, nullptr, nullptr);
            //        if (ret != SQLITE_OK) {
            //            throw std::runtime_error(strprintf("SQLiteDatabase: Failed to set the wallet schema version: %s\n", sqlite3_errstr(ret)));
            //        }
            //    }
        }

        void Close()
        {
            int res = sqlite3_close(m_db);
            if (res != SQLITE_OK)
            {
                throw std::runtime_error(
                    strprintf("%s: %d; Failed to close database: %s\n", __func__, res, sqlite3_errstr(res)));
            }
            m_db = nullptr;
        }

        bool BeginTransaction() const
        {
            if (!m_db || sqlite3_get_autocommit(m_db) == 0) return false;
            int res = sqlite3_exec(m_db, "BEGIN TRANSACTION", nullptr, nullptr, nullptr);
            if (res != SQLITE_OK)
            {
                LogPrintf("%s: %d; Failed to begin the transaction: %s\n", __func__, res, sqlite3_errstr(res));
            }
            return res == SQLITE_OK;
        }

        bool CommitTransaction() const
        {
            if (!m_db || sqlite3_get_autocommit(m_db) != 0) return false;
            int res = sqlite3_exec(m_db, "COMMIT TRANSACTION", nullptr, nullptr, nullptr);
            if (res != SQLITE_OK)
            {
                LogPrintf("%s: %d; Failed to commit the transaction: %s\n", __func__, res, sqlite3_errstr(res));
            }
            return res == SQLITE_OK;
        }

        bool AbortTransaction() const
        {
            if (!m_db || sqlite3_get_autocommit(m_db) != 0) return false;
            int res = sqlite3_exec(m_db, "ROLLBACK TRANSACTION", nullptr, nullptr, nullptr);
            if (res != SQLITE_OK)
            {
                LogPrintf("%s: %d; Failed to abort the transaction: %s\n", __func__, res, sqlite3_errstr(res));
            }
            return res == SQLITE_OK;
        }

    };
} // namespace PocketDb

#endif // POCKETDB_SQLITEDATABASE_H
