// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include "pocketdb/SQLiteDatabase.h"
#include "util/system.h"
#include "pocketdb/pocketnet.h"
#include "util/translation.h"
#include "validation.h"
#include <node/ui_interface.h>

#include "pocketdb/services/Serializer.h"

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

    static void InitializeSqlite()
    {
        LogPrintf("SQLite usage version: %d\n", (int)sqlite3_libversion_number());

        int ret = sqlite3_config(SQLITE_CONFIG_LOG, ErrorLogCallback, nullptr);
        if (ret != SQLITE_OK)
            throw std::runtime_error(
                strprintf("%s: %sd Failed to setup error log: %s\n", __func__, ret, sqlite3_errstr(ret)));

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
        NotifierRepoInst.Init();
        SystemRepoInst.Init();

        // Open, create structure and close `web` db
        PocketDbMigrationRef webDbMigration = std::make_shared<PocketDbWebMigration>();
        SQLiteDatabase sqliteDbWebInst(false);
        sqliteDbWebInst.Init(dbBasePath, "web", webDbMigration, gArgs.GetArg("-reindex", 0) == 5);
        sqliteDbWebInst.CreateStructure();
        sqliteDbWebInst.Close();

        // Attach `web` db to `main` db
        SQLiteDbInst.AttachDatabase("web");
    }

    void InitSQLiteCheckpoints(fs::path path)
    {
        auto checkpointDbName = Params().NetworkIDString();
        int i = 0;

        // Look for checkpoints directory in current folder or up to two directories up.
        while (i < 3 && !fs::exists((path / "checkpoints" / (checkpointDbName + ".sqlite3")).string()))
        {
            path = path / "..";
            i++;
        }
        if (!fs::exists((path / "checkpoints" / (checkpointDbName + ".sqlite3")).string()))
        {
            LogPrintf("Checkpoint DB %s not found!\nDownload actual DB file from %s and place to %s directory.\n",
                (path / (checkpointDbName + ".sqlite3")).string(),
                "https://github.com/pocketnetteam/pocketnet.core/tree/master/checkpoints/" + checkpointDbName + ".sqlite3",
                path.string()
            );

            throw std::runtime_error(_("Unable to start server. Checkpoints DB not found. See debug log for details.").translated);
        }
        SQLiteDbCheckpointInst.Init((path / "checkpoints").string(), checkpointDbName);
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

/*
    void SQLiteDatabase::InitMigration(bool& cleanMempool)
    {

        /*** Update Pocket DB from 0 to version 1 ********************************************
         *
         * This migration is necessary to fill the database with TxOutputs
         * records with OP_RETURN records - these records were not saved initially in the database
         * 
         * It is also necessary to fill in the TxInputs table
         * 
         * Since the data of these tables become critically important for the operation of the node,
         * it is necessary to make sure that they are available and valid.
         * To do this, a full chainActive scan is performed, checking for the presence of all data.
         * 
         * Only after the end of the process, the DB version will be increased - in case of a process failure.
         * 
         ************************************************************************************/
/*        const string dbNameMain = "main";
        const int newDbVersion = 1;
        int dbVersion = SystemRepoInst.GetDbVersion(dbNameMain);

        if (dbVersion < newDbVersion)
        {
            // Clean mempool for resaving transactions data
            cleanMempool = true;

            // For optimize work drop all indexes
            uiInterface.InitMessage("Preparing the database for updating..");
            DropIndexes();

            // Full rescan blockchain for resaving all transactions data
            int height = 0;
            int percent = ChainActive().Height() / 100;
            int64_t startTime = GetTimeMicros();
            while (height <= ChainActive().Height() && !ShutdownRequested())
            {
                try
                {
                    // Read block from disk
                    CBlock block;
                    CBlockIndex* pblockindex = ChainActive()[height];
                    if (!pblockindex || !ReadBlockFromDisk(block, pblockindex, Params().GetConsensus()))
                    {
                        LogPrintf("Failed read block %s from disk. Stopping\n", pblockindex->GetBlockHash().GetHex());
                        StartShutdown();
                        return;
                    }

                    // The default logic is used to deserialize the necessary data
                    // We can use standard methods to write data to the database, because inside `WHERE NOT EXISTS` is used
                    // This will allow us to record only the missing data without changing the existing ones.
                    if (auto[ok, pocketBlock] = PocketServices::Serializer::DeserializeBlock(block); ok)
                    {
                        TransRepoInst.InsertTransactions(pocketBlock);
                        
                        for (size_t i = 0; i < block.vtx.size(); i++)
                        {
                            const auto& tx = block.vtx[i];

                            ChainRepoInst.UpdateTransactionHeight(
                                pblockindex->GetBlockHash().GetHex(),
                                i,
                                height,
                                tx->GetHash().GetHex()
                            );
                        }
                    }
                    else
                    {
                        LogPrintf("Failed deserialize block %s. Stopping\n", pblockindex->GetBlockHash().GetHex());
                        StartShutdown();
                        return;
                    }

                    // Message for logging
                    if (height % percent == 0)
                    {
                        int64_t time = GetTimeMicros();
                        uiInterface.InitMessage(tfm::format("Updating Pocket DB: %d%% (%.2fm)", (height / percent), (0.000001 * (time - startTime)) / 60.0));
                        LogPrintf("Updating Pocket DB: %d%% (%.2fm)\n", (height / percent), (0.000001 * (time - startTime)) / 60.0);
                    }

                    height += 1;
                }
                catch (std::exception& e)
                {
                    LogPrintf("Unknown error: %s\n", e.what());
                    StartShutdown();
                    return;
                }
            }

            // Restore db structure
            CreateStructure();

            // Increment db version
            SystemRepoInst.SetDbVersion(dbNameMain, newDbVersion);
        }
    }
*/

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

                if (true || isReadOnlyConnect)
                    ret = sqlite3_open_v2((dbPath / m_file_path).string().c_str(), &m_db, flags, nullptr);
                else
                    ret = sqlite3_open_v2(":memory:", &m_db, flags, nullptr);

                if (ret != SQLITE_OK)
                    throw std::runtime_error(strprintf("%s: %d; Failed to open database: %s\n",
                        __func__, ret, sqlite3_errstr(ret)));
            }

            if (!isReadOnlyConnect && sqlite3_db_readonly(m_db, dbName.c_str()) == 1)
                throw std::runtime_error("Database opened in readonly");

            if (!isReadOnlyConnect)
            {
                if (sqlite3_exec(m_db, "PRAGMA journal_mode = wal;", nullptr, nullptr, nullptr) != 0)
                    throw std::runtime_error("Failed apply journal_mode = wal");

                // if (sqlite3_exec(m_db, "PRAGMA temp_store = memory;", nullptr, nullptr, nullptr) != 0)
                //     throw std::runtime_error("Failed apply temp_store = memory");
            }

            // TODO (tawmaz): Not working for existed database
            // int cacheSize = gArgs.GetArg("-sqlcachesize", 5);
            // int pageCount = cacheSize * 1024 * 1024 / 4096;
            // string cmd = "PRAGMA cache_size = " + to_string(pageCount) + ";";
            // if (sqlite3_exec(m_db, cmd.c_str(), nullptr, nullptr, nullptr) != 0)
            //     throw std::runtime_error("Failed to apply cache size");
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
        assert(m_db && m_db_migration);

        try
        {
            uiInterface.InitMessage(tfm::format("Updating Pocket DB `%s` structure..", m_file_path));
            LogPrintf("Updating Pocket DB`%s` structure..\n", m_file_path);

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

            if (!BulkExecute(m_db_migration->Indexes()))
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

} // namespace PocketDb

