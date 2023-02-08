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

    struct Pagination {
        int TopHeight;
        int PageStart;
        int PageSize;
        string OrderBy;
        bool Desc;
    };

    class BaseRepository
    {
    private:
        unordered_map<string, shared_ptr<Stmt>> _statements;

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
        void SqlTransaction(const string& func, T sql)
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

        Stmt& Sql(const string& sql)
        {
            auto itr = _statements.find(sql);
            if (itr == _statements.end())
            {
                auto stmt = make_shared<Stmt>();
                stmt->Init(m_database, sql);
                itr = _statements.emplace(sql, std::move(stmt)).first;
            }

            return *itr->second;
        }

        bool ExistsRows(Stmt& stmt)
        {
            bool res = false;
            stmt.Select([&](Cursor& cursor) {
                res = cursor.StepV() == SQLITE_ROW;
            });
            return res;
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

        virtual void Init() { }

        virtual void Destroy()
        {
            _statements.clear();
        }
    };
}

#endif //POCKETDB_BASEREPOSITORY_H

