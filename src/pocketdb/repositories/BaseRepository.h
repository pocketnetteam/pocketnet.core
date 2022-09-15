// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETDB_BASEREPOSITORY_H
#define POCKETDB_BASEREPOSITORY_H

#include <utility>
#include <util.h>
#include "shutdown.h"
#include "pocketdb/SQLiteDatabase.h"
#include "pocketdb/helpers/TransactionHelper.h"

#include <boost/algorithm/string/replace.hpp>

namespace PocketDb
{
    using namespace std;
    using namespace PocketHelpers;

    class RowAccessor
    {
    public:
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
    };

    class BaseRepository : protected RowAccessor
    {
    private:
    protected:
        SQLiteDatabase& m_database;

        string EscapeValue(string value)
        {
            boost::replace_all(value, "%", "\\%");
            boost::replace_all(value, "_", "\\_");

            return value;
        }

        // General method for SQL operations
        // Locked with shutdownMutex
        // Timeouted for ReadOnly connections
        template<typename T>
        void TryTransactionStep(const string& func, T sql)
        {
            try
            {
                int64_t nTime1 = GetTimeMicros();

                if (!m_database.BeginTransaction())
                    throw std::runtime_error(strprintf("%s: can't begin transaction\n", func));

                // We are running SQL logic with timeout only for read-only connections
                if (m_database.IsReadOnly())
                {
                    auto timeoutValue = chrono::seconds(gArgs.GetArg("-sqltimeout", 10));
                    run_with_timeout(
                        [&]()
                        {
                            sql();
                        },
                        timeoutValue,
                        [&]()
                        {
                            m_database.InterruptQuery();
                            LogPrintf("Function `%s` failed with execute timeout\n", func);
                        }
                    );
                }
                else
                {
                    sql();
                }
                
                if (!m_database.CommitTransaction())
                    throw std::runtime_error(strprintf("%s: can't commit transaction\n", func));

                int64_t nTime2 = GetTimeMicros();

                LogPrint(BCLog::SQLBENCH, "SQL Bench `%s`: %.2fms\n", func, 0.001 * (nTime2 - nTime1));
            }
            catch (const std::exception& ex)
            {
                m_database.AbortTransaction();
                throw std::runtime_error(func + ": " + ex.what());
            }
        }

        void TryStepStatement(shared_ptr<sqlite3_stmt*>& stmt)
        {
            int res = sqlite3_step(*stmt);
            FinalizeSqlStatement(*stmt);

            if (res != SQLITE_ROW && res != SQLITE_DONE)
                throw std::runtime_error(strprintf("%s: Failed execute SQL statement\n", __func__));
        }

        void TryTransactionBulk(const string& func, const vector<shared_ptr<sqlite3_stmt*>>& stmts)
    {
            if (!m_database.BeginTransaction())
                throw std::runtime_error(strprintf("%s: can't begin transaction\n", func));
                
            for (auto stmt : stmts)
                TryStepStatement(stmt);

            if (!m_database.CommitTransaction())
                throw std::runtime_error(strprintf("%s: can't commit transaction\n", func));
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

        // --------------------------------
        // BINDS
        // --------------------------------

        bool TryBindStatementText(shared_ptr<sqlite3_stmt*>& stmt, int index, shared_ptr<std::string> value)
        {
            if (!value) return true;

            int res = sqlite3_bind_text(*stmt, index, value->c_str(), (int) value->size(), SQLITE_STATIC);
            if (!CheckValidResult(stmt, res))
                return false;

            return true;
        }

        void TryBindStatementText(shared_ptr<sqlite3_stmt*>& stmt, int index, const std::string& value)
        {
            int res = sqlite3_bind_text(*stmt, index, value.c_str(), (int) value.size(), SQLITE_STATIC);
            if (!CheckValidResult(stmt, res))
                throw std::runtime_error(strprintf("%s: Failed bind SQL statement - index:%d value:%s\n",
                    __func__, index, value));
        }

        bool TryBindStatementInt(shared_ptr<sqlite3_stmt*>& stmt, int index, const shared_ptr<int>& value)
        {
            if (!value) return true;

            TryBindStatementInt(stmt, index, *value);
            return true;
        }

        void TryBindStatementInt(shared_ptr<sqlite3_stmt*>& stmt, int index, int value)
        {
            int res = sqlite3_bind_int(*stmt, index, value);
            if (!CheckValidResult(stmt, res))
                throw std::runtime_error(strprintf("%s: Failed bind SQL statement - index:%d value:%d\n",
                    __func__, index, value));
        }

        bool TryBindStatementInt64(shared_ptr<sqlite3_stmt*>& stmt, int index, const shared_ptr<int64_t>& value)
        {
            if (!value) return true;

            TryBindStatementInt64(stmt, index, *value);
            return true;
        }

        void TryBindStatementInt64(shared_ptr<sqlite3_stmt*>& stmt, int index, int64_t value)
        {
            int res = sqlite3_bind_int64(*stmt, index, value);
            if (!CheckValidResult(stmt, res))
                throw std::runtime_error(strprintf("%s: Failed bind SQL statement - index:%d value:%d\n",
                    __func__, index, value));
        }

        void SetLastInsertRowId(int64_t value)
        {
            sqlite3_set_last_insert_rowid(m_database.m_db, value);
        }

        int64_t GetLastInsertRowId()
        {
            return sqlite3_last_insert_rowid(m_database.m_db);
        }

    public:

        explicit BaseRepository(SQLiteDatabase& db) : m_database(db)
        {
        }

        virtual ~BaseRepository() = default;

        virtual void Init() = 0;
        virtual void Destroy() = 0;
    };
}

#endif //POCKETDB_BASEREPOSITORY_H

