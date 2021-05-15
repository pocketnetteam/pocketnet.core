// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 Bitcoin developers
// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETDB_BASEREPOSITORY_HPP
#define POCKETDB_BASEREPOSITORY_HPP

#include "shutdown.h"
#include "pocketdb/SQLiteDatabase.hpp"

namespace PocketDb
{
    using std::shared_ptr;
    using std::tuple;
    using std::make_tuple;
    using std::vector;
    using std::map;

    // TODO (brangr): mutext for insert/update/delete ?

    class BaseRepository
    {
    protected:
        SQLiteDatabase& m_database;

        // General method for SQL operations
        // Locked with shutdownMutex
        template<typename T>
        bool TryTransactionStep(T sql)
        {
            LOCK(SqliteShutdownMutex);
            if (ShutdownRequested())
                return false;

            if (!m_database.BeginTransaction())
                return false;

            try
            {
                sql();

                if (!m_database.CommitTransaction())
                    throw std::runtime_error(strprintf("%s: can't commit transaction\n", __func__));

            } catch (std::exception& ex)
            {
                LogPrintf("Transaction error: %s\n", ex.what());
                m_database.AbortTransaction();
                return false;
            }

            return true;
        }


        bool TryStepStatement(shared_ptr<sqlite3_stmt*> stmt)
        {
            int res = sqlite3_step(*stmt);
            return !(res != SQLITE_ROW && res != SQLITE_DONE);
        }

        bool TryBindStatementText(shared_ptr<sqlite3_stmt*> stmt, int index, shared_ptr<std::string> value)
        {
            if (!value) return true;

            int res = sqlite3_bind_text(*stmt, index, value->c_str(), (int) value->size(), SQLITE_STATIC);
            if (!CheckValidResult(stmt, res))
            {
                return false;
            }

            return true;
        }

        bool TryBindStatementInt(shared_ptr<sqlite3_stmt*> stmt, int index, const shared_ptr<int> value)
        {
            if (!value) return true;

            int res = sqlite3_bind_int(*stmt, index, *value);
            if (!CheckValidResult(stmt, res))
            {
                return false;
            }

            return true;
        }

        bool TryBindStatementInt64(shared_ptr<sqlite3_stmt*> stmt, int index, const shared_ptr<int64_t> value)
        {
            if (!value) return true;

            int res = sqlite3_bind_int64(*stmt, index, *value);
            if (!CheckValidResult(stmt, res))
            {
                return false;
            }

            return true;
        }


        [[nodiscard]]
        shared_ptr<sqlite3_stmt*> SetupSqlStatement(const std::string& sql) const
        {
            sqlite3_stmt* stmt;

            int res = sqlite3_prepare_v2(m_database.m_db, sql.c_str(), (int) sql.size(), &stmt, nullptr);
            if (res != SQLITE_OK)
                throw std::runtime_error(
                    strprintf("SQLiteDatabase: Failed to setup SQL statements: %s\n", sqlite3_errstr(res)));

            return std::make_shared<sqlite3_stmt*>(stmt);
        }

        bool CheckValidResult(shared_ptr<sqlite3_stmt*> stmt, int result)
        {
            if (result != SQLITE_OK)
            {
                //std::cout << strprintf("%s: Unable to bind statement: %s\n", __func__, sqlite3_errstr(result));
                sqlite3_clear_bindings(*stmt);
                sqlite3_reset(*stmt);
                return false;
            }

            return true;
        }

        int FinalizeSqlStatement(sqlite3_stmt* stmt)
        {
            return sqlite3_finalize(stmt);
        }

        std::string GetColumnString(sqlite3_stmt* stmt, int index)
        {
            return std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, index)));
        }

        int64_t GetColumnInt64(sqlite3_stmt* stmt, int index)
        {
            return sqlite3_column_int64(stmt, index);
        }

        int GetColumnInt(sqlite3_stmt* stmt, int index)
        {
            return sqlite3_column_int(stmt, index);
        }

    public:
        Mutex SqliteShutdownMutex;

        explicit BaseRepository(SQLiteDatabase& db) :
            m_database(db)
        {
        }

        virtual void Init() = 0;
        virtual void Destroy() = 0;
    };

}

#endif //POCKETDB_BASEREPOSITORY_HPP
