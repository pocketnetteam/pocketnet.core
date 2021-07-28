// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 Bitcoin developers
// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0


#ifndef POCKETDB_SQLITEDATABASE_H
#define POCKETDB_SQLITEDATABASE_H

#include "logging.h"
#include "sync.h"
#include "tinyformat.h"
#include "fs.h"

#include <sqlite3.h>
#include <iostream>

#include "pocketdb/migrations/main.hpp"

namespace PocketDb
{
    class SQLiteDatabase
    {
    private:
        std::string m_dir_path;
        std::string m_file_path;

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

        static void ErrorLogCallback(void* arg, int code, const char* msg)
        {
            // From sqlite3_config() documentation for the SQLITE_CONFIG_LOG option:
            // "The void pointer that is the second argument to SQLITE_CONFIG_LOG is passed through as
            // the first parameter to the application-defined logger function whenever that function is
            // invoked."
            // Assert that this is the case:
            assert(arg == nullptr);
            LogPrintf("%s: %d; Message: %s\n", __func__, code, msg);
        }

        bool TryCreateDbIfNotExists()
        {
            try
            {
                return fs::create_directories(m_dir_path);
            }
            catch (const fs::filesystem_error&)
            {
                if (!fs::exists(m_dir_path) || !fs::is_directory(m_dir_path))
                    throw;

                return false;
            }
        }

        bool BulkExecute(std::string sql)
        {
            try
            {
                char* errMsg = nullptr;
                size_t pos;
                std::string token;

                while ((pos = sql.find(';')) != std::string::npos)
                {
                    token = sql.substr(0, pos);

                    BeginTransaction();

                    if (sqlite3_exec(m_db, token.c_str(), nullptr, nullptr, &errMsg) != SQLITE_OK)
                        throw std::runtime_error("Failed to create init database");

                    CommitTransaction();

                    sql.erase(0, pos + 1);
                }
            }
            catch (const std::exception&)
            {
                AbortTransaction();
                LogPrintf("%s: Failed to create init database structure", __func__);
                return false;
            }

            return true;
        }

    public:
        sqlite3* m_db{nullptr};
        std::mutex m_connection_mutex;

        SQLiteDatabase() = default;

        void Init(const std::string& dir_path, const std::string& file_path)
        {
            m_dir_path = dir_path;
            m_file_path = file_path;

            LogPrintf("SQLite usage version: %d\n", (int)sqlite3_libversion_number());

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
            } catch (const std::runtime_error&)
            {
                // If open fails, cleanup this object and rethrow the exception
                Cleanup();
                throw;
            }
        }

        void CreateStructure()
        {
            LogPrintf("Creating Sqlite database structure..\n");

            if (!m_db || sqlite3_get_autocommit(m_db) == 0)
                throw std::runtime_error(strprintf("%s: Database not opened?\n", __func__));

            if (!BulkExecute(MainStructure))
                throw std::runtime_error(strprintf("%s: Failed to create database structure\n", __func__));
        }

        void DropIndexes()
        {
            LogPrintf("Drop Sqlite database indexes..\n");

            if (!m_db || sqlite3_get_autocommit(m_db) == 0)
                throw std::runtime_error(strprintf("%s: Database not opened?\n", __func__));

            if (!BulkExecute(MainDropIndexes))
                throw std::runtime_error(strprintf("%s: Failed to drop indexes\n", __func__));
        }

        void CreateIndexes()
        {
            LogPrintf("Creating Sqlite database indexes..\n");

            if (!m_db || sqlite3_get_autocommit(m_db) == 0)
                throw std::runtime_error(strprintf("%s: Database not opened?\n", __func__));

            if (!BulkExecute(MainCreateIndexes))
                throw std::runtime_error(strprintf("%s: Failed to create indexes\n", __func__));
        }

        void Open()
        {
            int flags = SQLITE_OPEN_READWRITE |
                        SQLITE_OPEN_CREATE; //SQLITE_OPEN_FULLMUTEX | SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE;

            if (m_db == nullptr)
            {
                int ret = sqlite3_open_v2(m_file_path.c_str(), &m_db, flags, nullptr);
                if (ret != SQLITE_OK)
                    throw std::runtime_error(strprintf("%s: %d; Failed to open database: %s\n",
                        __func__, ret, sqlite3_errstr(ret)));

                CreateStructure();
                CreateIndexes();
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
                LogPrintf("Error: %s: %d; Failed to close database: %s\n", __func__, res, sqlite3_errstr(res));

            m_db = nullptr;
        }

        bool BeginTransaction()
        {
            m_connection_mutex.lock();

            if (!m_db || sqlite3_get_autocommit(m_db) == 0) return false;
            int res = sqlite3_exec(m_db, "BEGIN TRANSACTION", nullptr, nullptr, nullptr);
            if (res != SQLITE_OK)
            {
                LogPrintf("%s: %d; Failed to begin the transaction: %s\n", __func__, res, sqlite3_errstr(res));
            }
            return res == SQLITE_OK;
        }

        bool CommitTransaction()
        {
            if (!m_db || sqlite3_get_autocommit(m_db) != 0) return false;
            int res = sqlite3_exec(m_db, "COMMIT TRANSACTION", nullptr, nullptr, nullptr);
            if (res != SQLITE_OK)
            {
                LogPrintf("%s: %d; Failed to commit the transaction: %s\n", __func__, res, sqlite3_errstr(res));
            }

            m_connection_mutex.unlock();

            return res == SQLITE_OK;
        }

        bool AbortTransaction()
        {
            if (!m_db || sqlite3_get_autocommit(m_db) != 0) return false;
            int res = sqlite3_exec(m_db, "ROLLBACK TRANSACTION", nullptr, nullptr, nullptr);
            if (res != SQLITE_OK)
            {
                LogPrintf("%s: %d; Failed to abort the transaction: %s\n", __func__, res, sqlite3_errstr(res));
            }

            m_connection_mutex.unlock();

            return res == SQLITE_OK;
        }

    };
} // namespace PocketDb

#endif // POCKETDB_SQLITEDATABASE_H
