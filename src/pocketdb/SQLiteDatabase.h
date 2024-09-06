// Copyright (c) 2018-2022 The Pocketnet developers
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

#include "pocketdb/migrations/base.h"
#include "pocketdb/migrations/main.h"
#include "pocketdb/migrations/web.h"

namespace PocketDb
{
    using namespace std;

    void InitSQLite(fs::path path);

    void MaybeMigrate0_22(const fs::path& pocketPath);

    class SQLiteDatabase
    {
    private:
        PocketDbMigrationRef m_db_migration;
        string m_file_path;
        string m_db_path;
        bool isReadOnlyConnect;

        bool BulkExecute(string sql);

    public:
        sqlite3* m_db{nullptr};
        mutex m_connection_mutex;

        explicit SQLiteDatabase(bool readOnly);

        bool IsReadOnly() const;

        void Init(const std::string& dbBasePath, const string& dbName, const PocketDbMigrationRef& migration = nullptr, bool drop = false);

        void CreateStructure(bool includeIndexes = true);

        void DropIndexes();

        void Cleanup() noexcept;
        
        /* void InitMigration(bool& cleanMempool); */

        void Close();

        bool BeginTransaction();

        bool CommitTransaction();

        bool AbortTransaction();

        void InterruptQuery();

        void DetachDatabase(const string& dbName);
        void AttachDatabase(const string& dbName);
        bool IsDatabaseAttached(const string& dbName);

        void RebuildIndexes();
    };

    typedef shared_ptr<SQLiteDatabase> SQLiteDatabaseRef;

} // namespace PocketDb

#endif // POCKETDB_SQLITEDATABASE_H
