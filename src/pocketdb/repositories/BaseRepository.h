// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETDB_BASEREPOSITORY_H
#define POCKETDB_BASEREPOSITORY_H

#include <utility>
#include <util/system.h>
#include <unordered_map>
#include <boost/algorithm/string/replace.hpp>

#include "shutdown.h"
#include "pocketdb/SQLiteDatabase.h"
#include "pocketdb/helpers/TransactionHelper.h"
#include "pocketdb/stmt.h"

namespace PocketDb
{
    using namespace std;
    using namespace PocketHelpers;

    class BaseRepository
    {
    private:
        unordered_map<size_t, shared_ptr<Stmt>> _statements;

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
                    throw runtime_error(strprintf("%s: can't begin transaction\n", func));

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
                    throw runtime_error(strprintf("%s: can't commit transaction\n", func));

                int64_t nTime2 = GetTimeMicros();

                LogPrint(BCLog::SQLBENCH, "SQL Bench `%s`: %.2fms\n", func, 0.001 * (nTime2 - nTime1));
            }
            catch (const exception& ex)
            {
                m_database.AbortTransaction();
                throw runtime_error(func + ": " + ex.what());
            }
        }

        void TryStepStatement(const shared_ptr<Stmt>& stmt)
        {
            int res = stmt->Step();
            if (res != SQLITE_ROW && res != SQLITE_DONE)
                throw runtime_error(strprintf("%s: Failed execute SQL statement\n", __func__));
        }

        void TryTransactionBulk(const string& func, const vector<shared_ptr<Stmt>>& stmts)
        {
            if (!m_database.BeginTransaction())
                throw runtime_error(strprintf("%s: can't begin transaction\n", func));
                
            for (auto stmt : stmts)
                TryStepStatement(stmt);

            if (!m_database.CommitTransaction())
                throw runtime_error(strprintf("%s: can't commit transaction\n", func));
        }

        shared_ptr<Stmt> SetupSqlStatement(const string& sql)
        {
            size_t key = hash<string>{}(sql);

            if (_statements.find(key) == _statements.end())
            {
                auto stmt = make_shared<Stmt>();
                stmt->Init(m_database, sql);
                _statements[key] = stmt;
            }

            _statements[key]->Reset();
            return _statements[key];
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

