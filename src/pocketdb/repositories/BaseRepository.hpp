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

        static bool TryBindStatementText(sqlite3_stmt *stmt, int index, const shared_ptr<std::string> &value)
        {
            if (!value) return true;

            int res = sqlite3_bind_text(stmt, index, value->c_str(), (int) value->size(), SQLITE_STATIC);
            if (!CheckValidResult(stmt, res))
            {
                return false;
            }

            return true;
        }

        static bool TryBindStatementInt(sqlite3_stmt *stmt, int index, const shared_ptr<int> &value)
        {
            if (!value) return true;

            int res = sqlite3_bind_int(stmt, index, *value);
            if (!CheckValidResult(stmt, res))
            {
                return false;
            }

            return true;
        }

        static bool TryBindStatementInt64(sqlite3_stmt *stmt, int index, const shared_ptr<int64_t> &value)
        {
            if (!value) return true;

            int res = sqlite3_bind_int64(stmt, index, *value);
            if (!CheckValidResult(stmt, res))
            {
                return false;
            }

            return true;
        }


        static bool CheckValidResult(sqlite3_stmt *stmt, int result)
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

    public:
        explicit BaseRepository(SQLiteDatabase &db) :
            m_database(db)
        {
        }
        virtual void Init() = 0;
    };

}

#endif //POCKETDB_BASEREPOSITORY_HPP
