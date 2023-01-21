// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include "pocketdb/stmt.h"



PocketDb::Stmt::~Stmt()
{
    Finalize();
}

void PocketDb::Stmt::Init(SQLiteDatabase& db, const std::string& sql)
{
    int res = sqlite3_prepare_v2(db.m_db, sql.c_str(), (int) sql.size(), &m_stmt, nullptr);
    if (res != SQLITE_OK)
        throw std::runtime_error(strprintf("SQLiteDatabase: Failed to setup SQL statements: %s\nSql: %s",
            sqlite3_errstr(res), sql));
}

int PocketDb::Stmt::Finalize()
{
    if (!m_stmt) return SQLITE_ERROR;
    ResetInternalIndicies();
    auto res = sqlite3_finalize(m_stmt);
    m_stmt = nullptr;
    return res;
}

int PocketDb::Stmt::Reset()
{
    if (!m_stmt) return SQLITE_ERROR;
    ResetInternalIndicies();
    if (auto rc = sqlite3_reset(m_stmt); rc != SQLITE_OK)
        return rc;
    return sqlite3_clear_bindings(m_stmt);
}

int PocketDb::Stmt::Step(bool reset)
{
    if (!m_stmt)
        throw runtime_error(strprintf("%s: Stmt::Step() Statement not ready\n", __func__));

    // Step
    int res = sqlite3_step(m_stmt);

    // Reset if need
    if (reset)
        Reset();
    
    if (res != SQLITE_ROW && res != SQLITE_DONE)
        throw runtime_error(strprintf("%s: Failed execute SQL statement\n", __func__));

    return res;
}

void PocketDb::Stmt::TryBindStatementText(int index, const std::string& value)
{
    int res = sqlite3_bind_text(m_stmt, index, value.c_str(), (int)value.size(), SQLITE_STATIC);
    if (!CheckValidResult(res))
        throw std::runtime_error(strprintf("%s: Failed bind SQL statement - index:%d value:%s\n",
            __func__, index, value));
}

bool PocketDb::Stmt::TryBindStatementText(int index, const std::optional<std::string>& value)
{
    if (!value) return true;

    int res = sqlite3_bind_text(m_stmt, index, value->c_str(), (int)value->size(), SQLITE_STATIC);
    if (!CheckValidResult(res))
        return false;

    return true;
}

bool PocketDb::Stmt::TryBindStatementInt(int index, const optional<int>& value)
{
    if (!value) return true;

    TryBindStatementInt(index, *value);
    return true;
}

void PocketDb::Stmt::TryBindStatementInt(int index, int value)
{
    int res = sqlite3_bind_int(m_stmt, index, value);
    if (!CheckValidResult(res))
        throw std::runtime_error(strprintf("%s: Failed bind SQL statement - index:%d value:%d\n",
            __func__, index, value));
}

bool PocketDb::Stmt::TryBindStatementInt64(int index, const optional<int64_t>& value)
{
    if (!value) return true;

    TryBindStatementInt64(index, *value);
    return true;
}

void PocketDb::Stmt::TryBindStatementInt64(int index, int64_t value)
{
    int res = sqlite3_bind_int64(m_stmt, index, value);
    if (!CheckValidResult(res))
        throw std::runtime_error(strprintf("%s: Failed bind SQL statement - index:%d value:%d\n",
            __func__, index, value));
}

bool PocketDb::Stmt::TryBindStatementNull(int index)
{
    int res = sqlite3_bind_null(m_stmt, index);
    if (!CheckValidResult(res))
        return false;

    return true;
}

bool PocketDb::Stmt::CheckValidResult(int result)
{
    if (result != SQLITE_OK) {
        Finalize();
        return false;
    }

    return true;
}

std::tuple<bool, std::string> PocketDb::Stmt::TryGetColumnString( int index)
{
    return sqlite3_column_type(m_stmt, index) == SQLITE_NULL ? make_tuple(false, "") : make_tuple(true, string(reinterpret_cast<const char*>(sqlite3_column_text(m_stmt, index))));
}
std::tuple<bool, int64_t> PocketDb::Stmt::TryGetColumnInt64(int index)
{
    return sqlite3_column_type(m_stmt, index) == SQLITE_NULL ? make_tuple(false, (int64_t)0) : make_tuple(true, (int64_t)sqlite3_column_int64(m_stmt, index));
}
std::tuple<bool, int> PocketDb::Stmt::TryGetColumnInt(int index)
{
    return sqlite3_column_type(m_stmt, index) == SQLITE_NULL ? make_tuple(false, 0) : make_tuple(true, sqlite3_column_int(m_stmt, index));
}

void PocketDb::Stmt::ResetInternalIndicies()
{
    m_currentBindIndex = 1;
    m_currentCollectIndex = 0;
}
