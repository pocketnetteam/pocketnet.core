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

#include "pocketdb/migrations/main.h"

namespace PocketDb
{
    class SQLiteDatabase
    {
    private:
        std::string m_dir_path;
        std::string m_file_path;

        bool isGeneralConnect;
        bool isReadOnlyConnect;

        void Cleanup() noexcept;

        bool TryCreateDbIfNotExists();

        bool BulkExecute(std::string sql);

    public:

        sqlite3* m_db{nullptr};
        std::mutex m_connection_mutex;

        SQLiteDatabase(bool general, bool readOnly);

        void Init(const std::string& dir_path, const std::string& file_path);

        void CreateStructure();

        void DropIndexes();

        void Open();

        void Close();

        bool BeginTransaction();

        bool CommitTransaction();

        bool AbortTransaction();

    };

    typedef std::shared_ptr<SQLiteDatabase> SQLiteDatabaseRef;

} // namespace PocketDb

#endif // POCKETDB_SQLITEDATABASE_H
