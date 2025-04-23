// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include "pocketdb/SQLiteDatabase.h"
#include "util/system.h"
#include "pocketdb/pocketnet.h"
#include "util/translation.h"
#include "validation.h"
#include <node/ui_interface.h>
#include "pocketdb/services/ChainPostProcessing.h"
#include "httpserver.h"
namespace PocketDb
{
    static int dbActualVersion = 3;

    static void ErrorLogCallback(void* arg, int code, const char* msg)
    {
        // From sqlite3_config() documentation for the SQLITE_CONFIG_LOG option:
        // "The void pointer that is the second argument to SQLITE_CONFIG_LOG is passed through as
        // the first parameter to the application-defined logger function whenever that function is
        // invoked."
        // Assert that this is the case:
        assert(arg == nullptr);
        LogPrint(BCLog::WARN, "%s, error: %d; Message: %s\n", sqlite3_errstr(code), code, msg);
    }

    static void InitializeSqlite()
    {
        LogPrintf("SQLite usage version: %d\n", (int)sqlite3_libversion_number());
        
        int ret = sqlite3_config(SQLITE_CONFIG_LOG, ErrorLogCallback, nullptr);
        if (ret != SQLITE_OK)
            throw std::runtime_error(
                strprintf("%s: %d Failed to setup error log: %s\n", __func__, ret, sqlite3_errstr(ret)));

        // Force serialized threading mode
        ret = sqlite3_config(SQLITE_CONFIG_SERIALIZED);
        if (ret != SQLITE_OK)
        {
            throw std::runtime_error(
                strprintf("%s: %d; Failed to configure serialized threading mode: %s\n",
                    __func__, ret, sqlite3_errstr(ret)));
        }

        // Initialize
        ret = sqlite3_initialize();
        if (ret != SQLITE_OK)
            throw std::runtime_error(
                strprintf("%s: %d; Failed to initialize SQLite: %s\n", __func__, ret, sqlite3_errstr(ret)));
    }

    void InitSQLite(fs::path path)
    {
        auto dbBasePath = path.string();

        InitializeSqlite();

        PocketDbMigrationRef mainDbMigration = std::make_shared<PocketDbMainMigration>();
        PocketDb::SQLiteDbInst.Init(dbBasePath, "main", mainDbMigration);
        SQLiteDbInst.CreateStructure();
        
        TransRepoInst.Init();
        ChainRepoInst.Init();
        RatingsRepoInst.Init();
        ConsensusRepoInst.Init();
        ExplorerRepoInst.Init();
        SystemRepoInst.Init();
        MigrationRepoInst.Init();

        auto dbVersion = SystemRepoInst.GetDbVersion();
        // Set db version to actual version if it is not set
        if (dbVersion == -1) {
            dbVersion = dbActualVersion;
            SystemRepoInst.SetDbVersion(dbVersion);
        }

        // Detect old version of database and migrate moderation juries
        int juryMigrationVersion = 2;
        if (dbVersion < juryMigrationVersion) {
            LOCK(cs_main);
            LogPrintf("Migrating moderation juries\n");
            PocketServices::ChainPostProcessing::Migrate_Jury();
            dbVersion = juryMigrationVersion;
            PocketDb::SystemRepoInst.SetDbVersion(dbVersion);
        }

        // Drop web database if reindex
        bool dropWebDb = gArgs.GetArg("-reindex", 0) == 5;

        // Open, create structure and close `web` db
        PocketDbMigrationRef webDbMigration = std::make_shared<PocketDbWebMigration>();
        SQLiteDatabase sqliteDbWebInst(false);
        sqliteDbWebInst.Init(dbBasePath, "web", webDbMigration, dropWebDb);
        sqliteDbWebInst.CreateStructure();
        sqliteDbWebInst.Close();

        // Attach `web` db to `main` db
        SQLiteDbInst.AttachDatabase("web");

        // Detect old version of database and migrate search index
        int searchMigrationVersion = 3;
        if (dbVersion < searchMigrationVersion) {
            LOCK(cs_main);
            LogPrintf("Migrating search index\n");
            PocketServices::MigrationRepoInst.Migrate_SearchIndex();
            dbVersion = searchMigrationVersion;
            PocketDb::SystemRepoInst.SetDbVersion(dbVersion);
        }

        LogPrintf("SQLite database version: %d\n", dbVersion);
    }

    SQLiteDatabase::SQLiteDatabase(bool readOnly) : isReadOnlyConnect(readOnly)
    {
    }

    void SQLiteDatabase::Cleanup() noexcept
    {
        Close();

        int ret = sqlite3_shutdown();
        if (ret != SQLITE_OK)
        {
            LogPrintf("%s: %d; Failed to shutdown SQLite: %s\n", __func__, ret, sqlite3_errstr(ret));
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

                LogPrint(BCLog::MIGRATION, "Migration Sqlite database `%s` structure..\n---\n%s\n---\n", m_file_path, token);

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
            LogPrintf("%s: Failed to create init database structure\n", __func__);
            return false;
        }

        return true;
    }

    bool SQLiteDatabase::IsReadOnly() const { return isReadOnlyConnect; }

    void SQLiteDatabase::Init(const std::string& dbBasePath, const std::string& dbName, const PocketDbMigrationRef& migration, bool drop)
    {
        m_db_migration = migration;
        m_db_path = dbBasePath;
        fs::path dbPath(m_db_path);
        m_file_path = dbName + ".sqlite3";

        // Create directory structure
        try
        {
            if (!m_db_path.empty())
                fs::create_directories(m_db_path);
        }
        catch (const fs::filesystem_error&)
        {
            if (!fs::exists(m_db_path) || !fs::is_directory(m_db_path))
                throw;
        }

        if (drop)
        {
            try
            {
                if (fs::exists(dbPath / (dbName + ".sqlite3")))
                    fs::remove(dbPath / (dbName + ".sqlite3"));

                if (fs::exists(dbPath / (dbName + ".sqlite3-shm")))
                    fs::remove(dbPath / (dbName + ".sqlite3-shm"));

                if (fs::exists(dbPath / (dbName + ".sqlite3-wal")))
                    fs::remove(dbPath / (dbName + ".sqlite3-wal"));
            }
            catch (const fs::filesystem_error& e)
            {
                throw std::runtime_error(strprintf("Database remove file error: %s", e.what()));
            }
        }

        try
        {
            int flags = SQLITE_OPEN_READWRITE |
                        SQLITE_OPEN_CREATE;

            if (isReadOnlyConnect)
            {
                flags = SQLITE_OPEN_READONLY;
                if (gArgs.GetBoolArg("-sqlsharedcache", false))
                    flags |= SQLITE_OPEN_SHAREDCACHE;
            }

            if (m_db == nullptr)
            {
                int ret = SQLITE_OK;

                auto fullPath = dbPath / m_file_path;
                if (true || isReadOnlyConnect)
                    ret = sqlite3_open_v2(fullPath.string().c_str(), &m_db, flags, nullptr);
                else
                    ret = sqlite3_open_v2(":memory:", &m_db, flags, nullptr);

                if (ret != SQLITE_OK)
                    throw std::runtime_error(strprintf("%s: %d; Failed to open database: %s\n",
                        __func__, ret, sqlite3_errstr(ret)));

                // Setting up busy timeout per connection
                ret = sqlite3_busy_timeout(m_db, gArgs.GetArg("-sqltimeout", 10) * 1000);
                if (ret != SQLITE_OK)
                    throw std::runtime_error(strprintf("%s: %d; Failed to setup busy_timeout: %s\n",
                        __func__, ret, sqlite3_errstr(ret)));
            }

            if (!isReadOnlyConnect && sqlite3_db_readonly(m_db, dbName.c_str()) == 1)
                throw std::runtime_error("Database opened in readonly");

            if (!isReadOnlyConnect)
            {
                string mode = gArgs.GetArg("-sqlmode", "wal");
                if (sqlite3_exec(m_db, ("PRAGMA journal_mode = " + mode + ";").c_str(), nullptr, nullptr, nullptr) != 0)
                    throw std::runtime_error("Failed apply journal_mode = " + mode);

                string sync = gArgs.GetArg("-sqlsync", "full");
                if (sqlite3_exec(m_db, ("PRAGMA synchronous = " + sync + ";").c_str(), nullptr, nullptr, nullptr) != 0)
                    throw std::runtime_error("Failed apply synchronous = " + sync);
            }

            // TEMP store
            string tmpType = gArgs.GetArg("-sqltempstore", "memory");
            if (sqlite3_exec(m_db, ("PRAGMA temp_store = " + tmpType + ";").c_str(), nullptr, nullptr, nullptr) != 0)
                throw std::runtime_error("Failed apply temp_store = " + tmpType);
            
            if (tmpType == "file")
            {
                string tmpPath = dbPath.string();
                if (sqlite3_exec(m_db, ("PRAGMA temp_store_directory = '" + tmpPath + "';").c_str(), nullptr, nullptr, nullptr) != 0)
                    throw std::runtime_error("Failed apply temp_store_directory = " + tmpPath);
            }

            // Cache size
            int rpcMainThreads = std::max((long) gArgs.GetArg("-rpcthreads", DEFAULT_HTTP_THREADS), 1L);
            int rpcPostThreads = std::max((long) gArgs.GetArg("-rpcpostthreads", DEFAULT_HTTP_POST_THREADS), 1L);
            int rpcPublicThreads = std::max((long) gArgs.GetArg("-rpcpublicthreads", DEFAULT_HTTP_PUBLIC_THREADS), 1L);
            int rpcStaticThreads = std::max((long) gArgs.GetArg("-rpcstaticthreads", DEFAULT_HTTP_STATIC_THREADS), 1L);
            int rpcRestThreads = std::max((long) gArgs.GetArg("-rpcrestthreads", DEFAULT_HTTP_REST_THREADS), 1L);

            // We calculate the average value for the cache of each connection based on all possible connections
            // Negative value means that we use bytes, not pages (* -1)
            // (3 * 2) is the number of connections that we consider as main (3 write connections x 2 to increase cache)
            int cacheSize = (gArgs.GetArg("-sqlcachesize", 100) * 1024) / 
                ((3 * 2) + rpcMainThreads + rpcPostThreads + rpcPublicThreads + rpcStaticThreads + rpcRestThreads) * -1;

            if (!isReadOnlyConnect)
                cacheSize = cacheSize * 2;

            string cmd = "PRAGMA cache_size = " + to_string(cacheSize) + ";";
            if (sqlite3_exec(m_db, cmd.c_str(), nullptr, nullptr, nullptr) != 0)
                throw std::runtime_error("Failed to apply cache size");

            LogPrintf("SQLite cache_size (%s:%s): %d\n", dbName, isReadOnlyConnect ? "R" : "W", cacheSize);
        }
        catch (const std::runtime_error&)
        {
            // If open fails, cleanup this object and rethrow the exception
            Cleanup();
            throw;
        }
    }

    void SQLiteDatabase::CreateStructure(bool includeIndexes)
    {
        assert(m_db && m_db_migration);

        try
        {
            uiInterface.InitMessage(tfm::format("Updating Pocket DB `%s` structure..", m_file_path));
            LogPrintf("Updating Pocket DB `%s` structure..\n", m_file_path);

            if (sqlite3_get_autocommit(m_db) == 0)
                throw std::runtime_error(strprintf("%s: Database `%s` not opened?\n", __func__, m_file_path));

            std::string tables;
            for (const auto& tbl : m_db_migration->Tables())
                tables += tbl + "\n";
            if (!BulkExecute(tables))
                throw std::runtime_error(strprintf("%s: Failed to create database `%s` structure (Tables)\n", __func__, m_file_path));

            std::string views;
            for (const auto& vw : m_db_migration->Views())
                views += vw + "\n";
            if (!BulkExecute(views))
                throw std::runtime_error(strprintf("%s: Failed to create database `%s` structure (Views)\n", __func__, m_file_path));

            if (!BulkExecute(m_db_migration->PreProcessing()))
                throw std::runtime_error(strprintf("%s: Failed to create database `%s` structure (PreProcessing)\n", __func__, m_file_path));

            if (!BulkExecute(m_db_migration->RequiredIndexes()))
                throw std::runtime_error(strprintf("%s: Failed to create database `%s` structure (RequiredIndexes)\n", __func__, m_file_path));

            if (includeIndexes && !BulkExecute(m_db_migration->Indexes()))
                throw std::runtime_error(strprintf("%s: Failed to create database `%s` structure (Indexes)\n", __func__, m_file_path));

            if (!BulkExecute(m_db_migration->PostProcessing()))
                throw std::runtime_error(strprintf("%s: Failed to create database `%s` structure (PostProcessing)\n", __func__, m_file_path));
        }
        catch (const std::exception& ex)
        {
            throw std::runtime_error(ex.what());
        }
    }

    void SQLiteDatabase::DropIndexes()
    {
        std::string indexesDropSql;

        // Get all indexes in DB
        try
        {
            std::string sql = "SELECT name FROM sqlite_master WHERE type == 'index' and name not like '%autoindex%'";

            BeginTransaction();

            sqlite3_stmt* stmt;
            int res = sqlite3_prepare_v2(m_db, sql.c_str(), (int) sql.size(), &stmt, nullptr);
            if (res != SQLITE_OK)
                throw std::runtime_error(strprintf("SQLiteDatabase: Failed to setup SQL statements: %s\nSql: %s\n",
                    sqlite3_errstr(res), sql));

            while (sqlite3_step(stmt) == SQLITE_ROW)
            {
                indexesDropSql += "DROP INDEX IF EXISTS ";
                indexesDropSql += std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0)));
                indexesDropSql += ";\n";
            }

            CommitTransaction();
        }
        catch (const std::exception& ex)
        {
            AbortTransaction();
            throw std::runtime_error(ex.what());
        }

        if (!BulkExecute(indexesDropSql))
            throw std::runtime_error(strprintf("%s: Failed drop indexes\n", __func__));
    }

    void SQLiteDatabase::Close()
    {
        int res = sqlite3_close(m_db);
        if (res != SQLITE_OK)
            LogPrintf("Error: %s: %d; Failed to close database %s: %s\n", __func__, res, m_file_path, sqlite3_errstr(res));

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

    void SQLiteDatabase::InterruptQuery()
    {
        if (m_db)
            sqlite3_interrupt(m_db);
    }

    void SQLiteDatabase::AttachDatabase(const string& dbName)
    {
        assert(m_db);

        fs::path dbPath(m_db_path);
        string cmnd = "attach database '" + (dbPath / (dbName + ".sqlite3")).string() + "' as " + dbName + ";";
        if (sqlite3_exec(m_db, cmnd.c_str(), nullptr, nullptr, nullptr) != 0)
            throw std::runtime_error("Failed attach database " + dbName);
    }

    void SQLiteDatabase::DetachDatabase(const string& dbName)
    {
        assert(m_db);

        fs::path dbPath(m_db_path);
        string cmnd = "detach " + dbName + ";";
        if (sqlite3_exec(m_db, cmnd.c_str(), nullptr, nullptr, nullptr) != 0)
            throw std::runtime_error("Failed detach database " + dbName);
    }

    void SQLiteDatabase::RebuildIndexes()
    {
        LogPrintf("Deleting database indexes..\n");
        DropIndexes();

        LogPrintf("Creating database indexes..\n");
        CreateStructure();
    }

    string SQLiteDatabase::GetPragmaValue(const string& pragmaName)
    {
        string value;
        sqlite3_stmt* stmt;
        if (sqlite3_prepare_v2(m_db, ("PRAGMA " + pragmaName + ";").c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
            if (sqlite3_step(stmt) == SQLITE_ROW) {
                // Получаем значение в зависимости от типа колонки
                switch (sqlite3_column_type(stmt, 0)) {
                    case SQLITE_INTEGER:
                        value = to_string(sqlite3_column_int(stmt, 0));
                        break;
                    case SQLITE_FLOAT:
                        value = to_string(sqlite3_column_double(stmt, 0));
                        break;
                    case SQLITE_TEXT:
                        value = string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0)));
                        break;
                    default:
                        value = "null";
                }
                LogPrintf("SQLite %s value: %s\n", pragmaName.c_str(), value.c_str());
            }
            sqlite3_finalize(stmt);
        }
        return value;
    }

} // namespace PocketDb

