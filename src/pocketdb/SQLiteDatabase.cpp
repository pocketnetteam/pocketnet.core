// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include "pocketdb/SQLiteDatabase.h"

namespace PocketDb
{
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

    SQLiteDatabase::SQLiteDatabase(bool general, bool readOnly) : generalConnect(general), readOnlyConnect(readOnly)
    {

    }

    void SQLiteDatabase::Cleanup() noexcept
    {
        Close();

        if (generalConnect)
        {
            int ret = sqlite3_shutdown();
            if (ret != SQLITE_OK)
            {
                LogPrintf("%s: %d; Failed to shutdown SQLite: %s\n", __func__, ret, sqlite3_errstr(ret));
            }
        }
    }

    bool SQLiteDatabase::TryCreateDbIfNotExists()
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

    bool SQLiteDatabase::BulkExecute(std::string sql)
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

    void SQLiteDatabase::Init(const std::string& dir_path, const std::string& file_path)
    {
        m_dir_path = dir_path;
        m_file_path = file_path;

        if (generalConnect)
        {
            LogPrintf("SQLite usage version: %d\n", (int) sqlite3_libversion_number());

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

            ret = sqlite3_initialize();
            if (ret != SQLITE_OK)
            {
                throw std::runtime_error(
                    strprintf("%s: %d; Failed to initialize SQLite: %s\n", __func__, ret, sqlite3_errstr(ret)));
            }
        }

        try
        {
            Open();
        }
        catch (const std::runtime_error&)
        {
            // If open fails, cleanup this object and rethrow the exception
            Cleanup();
            throw;
        }
    }

    void SQLiteDatabase::CreateStructure()
    {
        LogPrintf("Creating Sqlite database structure..\n");

        if (!m_db || sqlite3_get_autocommit(m_db) == 0)
            throw std::runtime_error(strprintf("%s: Database not opened?\n", __func__));

        if (!BulkExecute(MainStructure))
            throw std::runtime_error(strprintf("%s: Failed to create database structure\n", __func__));
    }

    void SQLiteDatabase::DropIndexes()
    {
        LogPrintf("Drop Sqlite database indexes..\n");

        if (!m_db || sqlite3_get_autocommit(m_db) == 0)
            throw std::runtime_error(strprintf("%s: Database not opened?\n", __func__));

        if (!BulkExecute(MainDropIndexes))
            throw std::runtime_error(strprintf("%s: Failed to drop indexes\n", __func__));
    }

    void SQLiteDatabase::CreateIndexes()
    {
        LogPrintf("Creating Sqlite database indexes..\n");

        if (!m_db || sqlite3_get_autocommit(m_db) == 0)
            throw std::runtime_error(strprintf("%s: Database not opened?\n", __func__));

        if (!BulkExecute(MainCreateIndexes))
            throw std::runtime_error(strprintf("%s: Failed to create indexes\n", __func__));
    }

    void SQLiteDatabase::Open()
    {
        int flags = SQLITE_OPEN_READWRITE |
                    SQLITE_OPEN_CREATE;

        if (readOnlyConnect)
            flags = SQLITE_OPEN_READONLY;

        if (m_db == nullptr)
        {
            int ret = sqlite3_open_v2(m_file_path.c_str(), &m_db, flags, nullptr);
            if (ret != SQLITE_OK)
                throw std::runtime_error(strprintf("%s: %d; Failed to open database: %s\n",
                    __func__, ret, sqlite3_errstr(ret)));

            if (generalConnect)
            {
                CreateStructure();
                CreateIndexes();
            }
        }

        if (!readOnlyConnect && sqlite3_db_readonly(m_db, "main") != 0)
            throw std::runtime_error("Database opened in readonly");

        if (sqlite3_exec(m_db, "PRAGMA journal_mode = wal;", nullptr, nullptr, nullptr) != 0)
            throw std::runtime_error("Failed apply journal_mode = wal");

        //    // Acquire an exclusive lock on the database
        //    // First change the locking mode to exclusive
        //    int ret = sqlite3_exec(m_db, "PRAGMA locking_mode = exclusive", nullptr, nullptr, nullptr);
        //    if (ret != SQLITE_OK) {
        //        throw std::runtime_error(strprintf("SQLiteDatabase: Unable to change database locking mode to exclusive: %s\n", sqlite3_errstr(ret)));
        //    }

        //    // Enable fullfsync for the platforms that use it
        //    ret = sqlite3_exec(m_db, "PRAGMA fullfsync = true", nullptr, nullptr, nullptr);
        //    if (ret != SQLITE_OK) {
        //        throw std::runtime_error(strprintf("SQLiteDatabase: Failed to enable fullfsync: %s\n", sqlite3_errstr(ret)));
        //    }

        // Set the user version
        //        std::string set_user_ver = strprintf("PRAGMA user_version = %d", WALLET_SCHEMA_VERSION);
        //        ret = sqlite3_exec(m_db, set_user_ver.c_str(), nullptr, nullptr, nullptr);
        //        if (ret != SQLITE_OK) {
        //            throw std::runtime_error(strprintf("SQLiteDatabase: Failed to set the wallet schema version: %s\n", sqlite3_errstr(ret)));
        //        }
        //    }
    }

    void SQLiteDatabase::Close()
    {
        int res = sqlite3_close(m_db);
        if (res != SQLITE_OK)
            LogPrintf("Error: %s: %d; Failed to close database: %s\n", __func__, res, sqlite3_errstr(res));

        m_db = nullptr;
    }

    bool SQLiteDatabase::BeginTransaction()
    {
        m_connection_mutex.lock();

        if (!m_db || sqlite3_get_autocommit(m_db) == 0) return false;
        int res = sqlite3_exec(m_db, "BEGIN TRANSACTION", nullptr, nullptr, nullptr);
        if (res != SQLITE_OK)
            LogPrintf("%s: %d; Failed to begin the transaction: %s\n", __func__, res, sqlite3_errstr(res));

        return res == SQLITE_OK;
    }

    bool SQLiteDatabase::CommitTransaction()
    {
        if (!m_db || sqlite3_get_autocommit(m_db) != 0) return false;
        int res = sqlite3_exec(m_db, "COMMIT TRANSACTION", nullptr, nullptr, nullptr);
        if (res != SQLITE_OK)
            LogPrintf("%s: %d; Failed to commit the transaction: %s\n", __func__, res, sqlite3_errstr(res));

        m_connection_mutex.unlock();

        return res == SQLITE_OK;
    }

    bool SQLiteDatabase::AbortTransaction()
    {
        if (!m_db || sqlite3_get_autocommit(m_db) != 0) return false;
        int res = sqlite3_exec(m_db, "ROLLBACK TRANSACTION", nullptr, nullptr, nullptr);
        if (res != SQLITE_OK)
            LogPrintf("%s: %d; Failed to abort the transaction: %s\n", __func__, res, sqlite3_errstr(res));

        m_connection_mutex.unlock();

        return res == SQLITE_OK;
    }

} // namespace PocketDb

