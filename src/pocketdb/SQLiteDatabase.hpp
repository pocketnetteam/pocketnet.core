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
            if (std::filesystem::exists(m_file_path)) {
                return true;
            }

            m_is_first_init = true;

            if (!std::filesystem::exists(m_dir_path)) {
                std::filesystem::create_directory(m_dir_path);
            }

            return true;
        }

        bool CreateStructure() const {
            std::string generate_sql =
                "drop table if exists Transactions;\n"
                "create table Transactions\n"
                "(\n"
                "    TxType  int    not null,\n"
                "    TxId    string not null,\n"
                "    Block   int    null,\n"
                "    TxOut   int    null,\n"
                "    TxTime  int    not null,\n"
                "    Address string not null,\n"
                "\n"
                "    -- User.Id\n"
                "    -- Post.Id\n"
                "    -- ScorePost.Value\n"
                "    -- ScoreComment.Value\n"
                "    -- Complain.Reason\n"
                "    Int1    int    null,\n"
                "\n"
                "    -- User.Registration\n"
                "    Int2    int    null,\n"
                "\n"
                "    -- Empty\n"
                "    Int3    int    null,\n"
                "\n"
                "    -- Empty\n"
                "    Int4    int    null,\n"
                "\n"
                "    -- Empty\n"
                "    Int5    int    null,\n"
                "\n"
                "    -- User.Lang\n"
                "    -- Post.Lang\n"
                "    -- Comment.Lang\n"
                "    -- Subscribe.AddressTo\n"
                "    -- Blocking.AddressTo\n"
                "    -- Complain.PostTxId\n"
                "    -- ScorePost.PostTxId\n"
                "    -- ScoreComment.CommentTxId\n"
                "    String1 string null,\n"
                "\n"
                "    -- User.Name\n"
                "    -- Post.Root\n"
                "    -- Comment.RootTxId\n"
                "    String2 string null,\n"
                "\n"
                "    -- User.Referrer\n"
                "    -- Post.RelayTxId\n"
                "    -- Comment.PostTxId\n"
                "    String3 string null,\n"
                "\n"
                "    -- Comment.ParentTxId\n"
                "    String4 string null,\n"
                "\n"
                "    -- Comment.AnswerTxId\n"
                "    String5 string null,\n"
                "\n"
                "    primary key (TxId)\n"
                ");\n"
                "\n"
                "--------------------------------------------\n"
                "drop index if exists Transactions_TxType;\n"
                "create index Transactions_TxType on Transactions (TxType);\n"
                "\n"
                "drop index if exists Transactions_Block;\n"
                "create index Transactions_Block on Transactions (Block);\n"
                "\n"
                "drop index if exists Transactions_TxOut;\n"
                "create index Transactions_TxOut on Transactions (TxOut);\n"
                "\n"
                "drop index if exists Transactions_TxTime;\n"
                "create index Transactions_TxTime on Transactions (TxTime desc);\n"
                "\n"
                "drop index if exists Transactions_Address;\n"
                "create index Transactions_Address on Transactions (Address);\n"
                "\n"
                "drop index if exists Transactions_Int1;\n"
                "create index Transactions_Int1 on Transactions (Int1);\n"
                "\n"
                "drop index if exists Transactions_Int2;\n"
                "create index Transactions_Int2 on Transactions (Int2);\n"
                "\n"
                "drop index if exists Transactions_Int3;\n"
                "create index Transactions_Int3 on Transactions (Int3);\n"
                "\n"
                "drop index if exists Transactions_Int4;\n"
                "create index Transactions_Int4 on Transactions (Int4);\n"
                "\n"
                "drop index if exists Transactions_Int5;\n"
                "create index Transactions_Int5 on Transactions (Int5);\n"
                "\n"
                "drop index if exists Transactions_String1;\n"
                "create index Transactions_String1 on Transactions (String1);\n"
                "\n"
                "drop index if exists Transactions_String2;\n"
                "create index Transactions_String2 on Transactions (String2);\n"
                "\n"
                "drop index if exists Transactions_String3;\n"
                "create index Transactions_String3 on Transactions (String3);\n"
                "\n"
                "drop index if exists Transactions_String4;\n"
                "create index Transactions_String4 on Transactions (String4);\n"
                "\n"
                "drop index if exists Transactions_String5;\n"
                "create index Transactions_String5 on Transactions (String5);\n"
                "\n"
                "\n"
                "--------------------------------------------\n"
                "--               EXT TABLES               --\n"
                "--------------------------------------------\n"
                "drop table if exists Payload;\n"
                "create table Payload\n"
                "(\n"
                "    TxId string not null,\n"
                "    Data string not null,\n"
                "\n"
                "    primary key (TxId)\n"
                ");\n"
                "\n"
                "--------------------------------------------\n"
                "drop table if exists Utxo;\n"
                "create table Utxo\n"
                "(\n"
                "    TxId       string not null,\n"
                "    TxOut      int    not null,\n"
                "    TxTime     int    not null,\n"
                "    Block      int    not null,\n"
                "    BlockSpent int    null,\n"
                "    Address    string not null,\n"
                "    Amount     int    not null,\n"
                "\n"
                "    primary key (TxId, TxOut)\n"
                ");\n"
                "\n"
                "drop index if exists Utxo_BlockSpent_Address_Amount_index;\n"
                "create index Utxo_BlockSpent_Address_Amount_index on Utxo (BlockSpent, Address, Amount);\n"
                "\n"
                "drop index if exists Utxo_Block_index;\n"
                "create index Utxo_Block_index on Utxo (Block);\n"
                "\n"
                "--------------------------------------------\n"
                "drop table if exists Ratings;\n"
                "create table Ratings\n"
                "(\n"
                "    RatingType int not null,\n"
                "    Block      int not null,\n"
                "    Key        int not null,\n"
                "    Value      int not null,\n"
                "\n"
                "    primary key (Block, RatingType, Key)\n"
                ");\n"
                "\n"
                "\n"
                "--------------------------------------------\n"
                "--                VIEWS                   --\n"
                "--------------------------------------------\n"
                "drop view if exists Users;\n"
                "create view Users as\n"
                "select t.TxType,\n"
                "       t.TxId,\n"
                "       t.TxTime,\n"
                "       t.Block,\n"
                "       t.TxOut,\n"
                "       t.Address,\n"
                "       t.Int1    as Registration,\n"
                "       t.String1 as Lang,\n"
                "       t.String2 as Name,\n"
                "       t.String3 as Referrer\n"
                "from Transactions t\n"
                "where t.TxType in (100);\n"
                "\n"
                "drop view if exists VideoServers;\n"
                "create view VideoServers as\n"
                "select t.TxType,\n"
                "       t.TxId,\n"
                "       t.TxTime,\n"
                "       t.Block,\n"
                "       t.TxOut,\n"
                "       t.Address,\n"
                "       t.Int1    as Registration,\n"
                "       t.String1 as Lang,\n"
                "       t.String2 as Name\n"
                "from Transactions t\n"
                "where t.TxType in (101);\n"
                "\n"
                "drop view if exists MessageServers;\n"
                "create view MessageServers as\n"
                "select t.TxType,\n"
                "       t.TxId,\n"
                "       t.TxTime,\n"
                "       t.Block,\n"
                "       t.TxOut,\n"
                "       t.Address,\n"
                "       t.Int1    as Registration,\n"
                "       t.String1 as Lang,\n"
                "       t.String2 as Name\n"
                "from Transactions t\n"
                "where t.TxType in (102);\n"
                "\n"
                "--------------------------------------------\n"
                "drop view if exists Posts;\n"
                "create view Posts as\n"
                "select t.TxType,\n"
                "       t.TxId,\n"
                "       t.TxTime,\n"
                "       t.Block,\n"
                "       t.TxOut,\n"
                "       t.Address,\n"
                "       t.String1 as Lang,\n"
                "       t.String2 as RootTxId,\n"
                "       t.String3 as RelayTxId\n"
                "from Transactions t\n"
                "where t.TxType in (200);\n"
                "\n"
                "drop view if exists Videos;\n"
                "create view Videos as\n"
                "select t.TxType,\n"
                "       t.TxId,\n"
                "       t.TxTime,\n"
                "       t.Block,\n"
                "       t.TxOut,\n"
                "       t.Address,\n"
                "       t.String1 as Lang,\n"
                "       t.String2 as RootTxId,\n"
                "       t.String3 as RelayTxId\n"
                "from Transactions t\n"
                "where t.TxType in (201);\n"
                "\n"
                "drop view if exists Translates;\n"
                "create view Translates as\n"
                "select t.TxType,\n"
                "       t.TxId,\n"
                "       t.TxTime,\n"
                "       t.Block,\n"
                "       t.TxOut,\n"
                "       t.Address,\n"
                "       t.String1 as Lang,\n"
                "       t.String2 as RootTxId,\n"
                "       t.String3 as RelayTxId\n"
                "from Transactions t\n"
                "where t.TxType in (202);\n"
                "\n"
                "drop view if exists ServerPings;\n"
                "create view ServerPings as\n"
                "select t.TxType,\n"
                "       t.TxId,\n"
                "       t.TxTime,\n"
                "       t.Block,\n"
                "       t.TxOut,\n"
                "       t.Address,\n"
                "       t.String1 as Lang,\n"
                "       t.String2 as RootTxId,\n"
                "       t.String3 as RelayTxId\n"
                "from Transactions t\n"
                "where t.TxType in (203);\n"
                "\n"
                "drop view if exists Comments;\n"
                "create view Comments as\n"
                "select t.TxType,\n"
                "       t.TxId,\n"
                "       t.TxTime,\n"
                "       t.Block,\n"
                "       t.TxOut,\n"
                "       t.Address,\n"
                "       t.String1 as Lang,\n"
                "       t.String2 as RootTxId,\n"
                "       t.String3 as PostTxId,\n"
                "       t.String4 as ParentTxId,\n"
                "       t.String5 as AnswerTxId\n"
                "from Transactions t\n"
                "where t.TxType in (204);\n"
                "\n"
                "--------------------------------------------\n"
                "drop view if exists ScorePosts;\n"
                "create view ScorePosts as\n"
                "select t.TxType,\n"
                "       t.TxId,\n"
                "       t.TxTime,\n"
                "       t.Block,\n"
                "       t.TxOut,\n"
                "       t.Address,\n"
                "       t.Int1    as Value,\n"
                "       t.String1 as PostTxId\n"
                "from Transactions t\n"
                "where t.TxType in (300);\n"
                "\n"
                "drop view if exists ScoreComments;\n"
                "create view ScoreComments as\n"
                "select t.TxType,\n"
                "       t.TxId,\n"
                "       t.TxTime,\n"
                "       t.Block,\n"
                "       t.TxOut,\n"
                "       t.Address,\n"
                "       t.Int1    as Value,\n"
                "       t.String1 as CommentTxId\n"
                "from Transactions t\n"
                "where t.TxType in (301);\n"
                "\n"
                "\n"
                "drop view if exists Subscribes;\n"
                "create view Subscribes as\n"
                "select t.TxType,\n"
                "       t.TxId,\n"
                "       t.TxTime,\n"
                "       t.Block,\n"
                "       t.TxOut,\n"
                "       t.Address,\n"
                "       t.String1 as AddressTo\n"
                "from Transactions t\n"
                "where t.TxType in (302, 303, 304);\n"
                "\n"
                "\n"
                "drop view if exists Blockings;\n"
                "create view Blockings as\n"
                "select t.TxType,\n"
                "       t.TxId,\n"
                "       t.TxTime,\n"
                "       t.Block,\n"
                "       t.TxOut,\n"
                "       t.Address,\n"
                "       t.String1 as AddressTo\n"
                "from Transactions t\n"
                "where t.TxType in (305, 306);\n"
                "\n"
                "\n"
                "drop view if exists Complains;\n"
                "create view Complains as\n"
                "select t.TxType,\n"
                "       t.TxId,\n"
                "       t.TxTime,\n"
                "       t.Block,\n"
                "       t.TxOut,\n"
                "       t.Address,\n"
                "       t.String1 as AddressTo,\n"
                "       t.Int1    as Reason\n"
                "from Transactions t\n"
                "where t.TxType in (307);\n";

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
                        //TODO (brangr): throw ex?
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
