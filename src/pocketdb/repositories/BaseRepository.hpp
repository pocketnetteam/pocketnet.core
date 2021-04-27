// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 Bitcoin developers
// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETDB_BASEREPOSITORY_HPP
#define POCKETDB_BASEREPOSITORY_HPP

#include "pocketdb/SQLiteDatabase.hpp"

namespace PocketDb
{
    using std::shared_ptr;

    class BaseRepository
    {
    protected:
        SQLiteDatabase &m_database;

        bool TryStepStatement(sqlite3_stmt *stmt)
        {
            int res = sqlite3_step(stmt);
            if (res != SQLITE_ROW && res != SQLITE_DONE)
                LogPrintf("%s: Unable to execute statement: %s: %s\n",
                    __func__, sqlite3_sql(stmt), sqlite3_errstr(res));

            return !(res != SQLITE_ROW && res != SQLITE_DONE);
        }

        bool TryBindStatementText(sqlite3_stmt *stmt, int index, const shared_ptr<std::string> &value)
        {
            if (!value) return true;

            int res = sqlite3_bind_text(stmt, index, value->c_str(), (int) value->size(), SQLITE_STATIC);
            if (!CheckValidResult(stmt, res))
            {
                return false;
            }

            return true;
        }

        bool TryBindStatementInt(sqlite3_stmt *stmt, int index, int value)
        {
            if (!value) return true;

            int res = sqlite3_bind_int(stmt, index, value);
            if (!CheckValidResult(stmt, res))
            {
                return false;
            }

            return true;
        }

        bool TryBindStatementInt(sqlite3_stmt *stmt, int index, const shared_ptr<int> &value)
        {
            if (!value) return true;

            int res = sqlite3_bind_int(stmt, index, *value);
            if (!CheckValidResult(stmt, res))
            {
                return false;
            }

            return true;
        }

        bool TryBindStatementInt64(sqlite3_stmt *stmt, int index, const shared_ptr<int64_t> &value)
        {
            if (!value) return true;

            int res = sqlite3_bind_int64(stmt, index, *value);
            if (!CheckValidResult(stmt, res))
            {
                return false;
            }

            return true;
        }


        bool CheckValidResult(sqlite3_stmt *stmt, int result)
        {
            if (result != SQLITE_OK)
            {
                std::cout << strprintf("%s: Unable to bind statement: %s\n", __func__, sqlite3_errstr(result));
                sqlite3_clear_bindings(stmt);
                sqlite3_reset(stmt);
                return false;
            }

            return true;
        }

        sqlite3_stmt *SetupSqlStatement(sqlite3_stmt *stmt, const std::string &sql) const
        {
            int res;

            if (!stmt)
            {
                if ((res = sqlite3_prepare_v2(m_database.m_db, sql.c_str(), (int) sql.size(), &stmt, nullptr)) !=
                    SQLITE_OK)
                {
                    throw std::runtime_error(
                        strprintf("SQLiteDatabase: Failed to setup SQL statements: %s\n", sqlite3_errstr(res)));
                }
            }

            return stmt;
        }

        int FinalizeSqlStatement(sqlite3_stmt *stmt)
        {
            return sqlite3_finalize(stmt);
        }

    public:
        explicit BaseRepository(SQLiteDatabase &db) :
            m_database(db)
        {
        }

        virtual void Init() = 0;
        virtual void Destroy() = 0;
    };

}

#endif //POCKETDB_BASEREPOSITORY_HPP
