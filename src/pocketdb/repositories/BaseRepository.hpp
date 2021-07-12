// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 Bitcoin developers
// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETDB_BASEREPOSITORY_HPP
#define POCKETDB_BASEREPOSITORY_HPP

#include <utility>
#include <util.h>
#include "shutdown.h"
#include "pocketdb/SQLiteDatabase.hpp"

namespace PocketDb
{
    using namespace std;
    using namespace PocketHelpers;

    class BaseRepository
    {
    protected:
        SQLiteDatabase& m_database;

        // General method for SQL operations
        // Locked with shutdownMutex
        template<typename T>
        void TryTransactionStep(T sql)
        {
            int64_t nTime1 = GetTimeMicros();

            LOCK(SqliteShutdownMutex);

            int64_t nTime2 = GetTimeMicros();
            LogPrint(BCLog::BENCH, "        - TryTransactionStep LOCK: %.2fms\n", 0.001 * (nTime2 - nTime1));

            if (!m_database.BeginTransaction())
                return;

            int64_t nTime3 = GetTimeMicros();
            LogPrint(BCLog::BENCH, "        - TryTransactionStep BEGIN: %.2fms\n", 0.001 * (nTime3 - nTime2));

            try
            {
                sql();

                if (!m_database.CommitTransaction())
                    throw std::runtime_error(strprintf("%s: can't commit transaction\n", __func__));

                int64_t nTime4 = GetTimeMicros();
                LogPrint(BCLog::BENCH, "        - TryTransactionStep COMMIT: %.2fms\n", 0.001 * (nTime4 - nTime3));

            }
            catch (std::exception& ex)
            {
                LogPrintf("Transaction error: %s\n", ex.what());
                m_database.AbortTransaction();
                throw ex;
            }
        }

        void TryTransactionStep(std::initializer_list<shared_ptr<sqlite3_stmt*>> stmts)
        {
            TryTransactionStep([&]()
            {
                for (auto stmt : stmts)
                {
                    int64_t nTime1 = GetTimeMicros();

                    TryStepStatement(stmt);

                    int64_t nTime2 = GetTimeMicros();
                    LogPrint(BCLog::BENCH, "        - TryTransactionStep STEP: %.2fms\n", 0.001 * (nTime2 - nTime1));
                }
            });
        }

        void TryStepStatement(shared_ptr<sqlite3_stmt*>& stmt)
        {
            int res = sqlite3_step(*stmt);
            FinalizeSqlStatement(*stmt);

            if (res != SQLITE_ROW && res != SQLITE_DONE)
                throw std::runtime_error(strprintf("%s: Failed execute SQL statement\n", __func__));
        }

        // --------------------------------
        // BINDS
        // --------------------------------
        void TryBindStatementText(shared_ptr<sqlite3_stmt*>& stmt, int index, const std::string& value)
        {
            int res = sqlite3_bind_text(*stmt, index, value.c_str(), (int) value.size(), SQLITE_STATIC);
            if (!CheckValidResult(stmt, res))
                throw std::runtime_error(strprintf("%s: Failed bind SQL statement - index:%d value:%s\n",
                    __func__, index, value));
        }

        void TryBindStatementInt(shared_ptr<sqlite3_stmt*>& stmt, int index, int value)
        {
            int res = sqlite3_bind_int(*stmt, index, value);
            if (!CheckValidResult(stmt, res))
                throw std::runtime_error(strprintf("%s: Failed bind SQL statement - index:%d value:%d\n",
                    __func__, index, value));
        }

        void TryBindStatementInt64(shared_ptr<sqlite3_stmt*>& stmt, int index, int64_t value)
        {
            int res = sqlite3_bind_int64(*stmt, index, value);
            if (!CheckValidResult(stmt, res))
                throw std::runtime_error(strprintf("%s: Failed bind SQL statement - index:%d value:%d\n",
                    __func__, index, value));
        }

        shared_ptr<sqlite3_stmt*> SetupSqlStatement(const std::string& sql) const
        {
            sqlite3_stmt* stmt;

            int res = sqlite3_prepare_v2(m_database.m_db, sql.c_str(), (int) sql.size(), &stmt, nullptr);
            if (res != SQLITE_OK)
                throw std::runtime_error(strprintf("SQLiteDatabase: Failed to setup SQL statements: %s\nSql: %s",
                    sqlite3_errstr(res), sql));

            return std::make_shared<sqlite3_stmt*>(stmt);
        }

        bool CheckValidResult(shared_ptr<sqlite3_stmt*> stmt, int result)
        {
            if (result != SQLITE_OK)
            {
                FinalizeSqlStatement(*stmt);
                return false;
            }

            return true;
        }

        int FinalizeSqlStatement(sqlite3_stmt* stmt)
        {
            return sqlite3_finalize(stmt);
        }

        tuple<bool, std::string> TryGetColumnString(sqlite3_stmt* stmt, int index)
        {
            return sqlite3_column_type(stmt, index) == SQLITE_NULL
                   ? make_tuple(false, "")
                   : make_tuple(true, std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, index))));
        }

        tuple<bool, int64_t> TryGetColumnInt64(sqlite3_stmt* stmt, int index)
        {
            return sqlite3_column_type(stmt, index) == SQLITE_NULL
                   ? make_tuple(false, (int64_t) 0)
                   : make_tuple(true, (int64_t) sqlite3_column_int64(stmt, index));
        }

        tuple<bool, int> TryGetColumnInt(sqlite3_stmt* stmt, int index)
        {
            return sqlite3_column_type(stmt, index) == SQLITE_NULL
                   ? make_tuple(false, 0)
                   : make_tuple(true, sqlite3_column_int(stmt, index));
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
