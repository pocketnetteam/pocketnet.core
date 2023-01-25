// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include "pocketdb/stmt.h"

namespace PocketDb
{

    // ----------------------------------------------
    // STMT
    // ----------------------------------------------

    Stmt::~Stmt()
    {
        Finalize();
    }

    void Stmt::Init(SQLiteDatabase& db, const string& sql)
    {
        int res = sqlite3_prepare_v2(db.m_db, sql.c_str(), (int) sql.size(), &m_stmt, nullptr);
        if (res != SQLITE_OK)
            throw runtime_error(strprintf("SQLiteDatabase: Failed to setup SQL statements: %s\nSql: %s",
                sqlite3_errstr(res), sql));
    }

    int Stmt::Finalize()
    {
        if (!m_stmt) return SQLITE_ERROR;
        m_currentBindIndex = 1;
        auto res = sqlite3_finalize(m_stmt);
        m_stmt = nullptr;
        return res;
    }

    int Stmt::Reset()
    {
        if (!m_stmt) return SQLITE_ERROR;
        m_currentBindIndex = 1;
        if (auto rc = sqlite3_reset(m_stmt); rc != SQLITE_OK)
            return rc;
        return sqlite3_clear_bindings(m_stmt);
    }

    int Stmt::Run()
    {
        if (!m_stmt)
            throw runtime_error(strprintf("%s: Stmt::Step() Statement not ready\n", __func__));

        // Step
        int res = sqlite3_step(m_stmt);

        // Allways reset
        Reset();
        
        if (res != SQLITE_ROW && res != SQLITE_DONE)
            throw runtime_error(strprintf("%s: Failed execute SQL statement\n", __func__));

        return res;
    }

    void Stmt::TryBindStatementText(int index, const string& value)
    {
        int res = sqlite3_bind_text(m_stmt, index, value.c_str(), (int)value.size(), SQLITE_STATIC);
        if (!CheckValidResult(res))
            throw runtime_error(strprintf("%s: Failed bind SQL statement - index:%d value:%s\n",
                __func__, index, value));
    }

    bool Stmt::TryBindStatementText(int index, const optional<string>& value)
    {
        if (!value) return true;

        int res = sqlite3_bind_text(m_stmt, index, value->c_str(), (int)value->size(), SQLITE_STATIC);
        if (!CheckValidResult(res))
            return false;

        return true;
    }

    bool Stmt::TryBindStatementInt(int index, const optional<int>& value)
    {
        if (!value) return true;

        TryBindStatementInt(index, *value);
        return true;
    }

    void Stmt::TryBindStatementInt(int index, int value)
    {
        int res = sqlite3_bind_int(m_stmt, index, value);
        if (!CheckValidResult(res))
            throw runtime_error(strprintf("%s: Failed bind SQL statement - index:%d value:%d\n",
                __func__, index, value));
    }

    bool Stmt::TryBindStatementInt64(int index, const optional<int64_t>& value)
    {
        if (!value) return true;

        TryBindStatementInt64(index, *value);
        return true;
    }

    void Stmt::TryBindStatementInt64(int index, int64_t value)
    {
        int res = sqlite3_bind_int64(m_stmt, index, value);
        if (!CheckValidResult(res))
            throw runtime_error(strprintf("%s: Failed bind SQL statement - index:%d value:%d\n",
                __func__, index, value));
    }

    bool Stmt::TryBindStatementNull(int index)
    {
        int res = sqlite3_bind_null(m_stmt, index);
        if (!CheckValidResult(res))
            return false;

        return true;
    }

    bool Stmt::CheckValidResult(int result)
    {
        if (result != SQLITE_OK) {
            Finalize();
            return false;
        }

        return true;
    }

    int Stmt::Select(const function<void(Cursor&)>& func)
    {
        auto cursor = static_cast<Cursor*>(this);
        func(*cursor);
        return Reset();
    }

    // ----------------------------------------------
    // CURSOR
    // ----------------------------------------------

    bool Cursor::Step()
    {
        int res = sqlite3_step(m_stmt);
        m_currentCollectIndex = 0;

        return (res == SQLITE_ROW);
    }

    tuple<bool, string> Cursor::TryGetColumnString( int index)
    {
        return sqlite3_column_type(m_stmt, index) == SQLITE_NULL ? make_tuple(false, "") : make_tuple(true, string(reinterpret_cast<const char*>(sqlite3_column_text(m_stmt, index))));
    }
    tuple<bool, int64_t> Cursor::TryGetColumnInt64(int index)
    {
        return sqlite3_column_type(m_stmt, index) == SQLITE_NULL ? make_tuple(false, (int64_t)0) : make_tuple(true, (int64_t)sqlite3_column_int64(m_stmt, index));
    }
    tuple<bool, int> Cursor::TryGetColumnInt(int index)
    {
        return sqlite3_column_type(m_stmt, index) == SQLITE_NULL ? make_tuple(false, 0) : make_tuple(true, sqlite3_column_int(m_stmt, index));
    }

}