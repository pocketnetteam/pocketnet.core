// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include "pocketdb/stmt.h"
#include <chainparams.h>

namespace PocketDb
{

    // ----------------------------------------------
    // STMT
    // ----------------------------------------------

    void Stmt::Init(SQLiteDatabase& db, const string& sql)
    {
        m_stmt = std::make_unique<StmtWrapper>();
        auto res = m_stmt->PrepareV2(db, sql);
        if (res != SQLITE_OK)
            throw runtime_error(strprintf("SQLiteDatabase: Failed to setup SQL statements: %s\nSql: %s",
                sqlite3_errstr(res), sql));
    }

    int Stmt::Run()
    {
        if (!m_stmt)
            throw runtime_error(strprintf("%s: Stmt::Step() Statement not ready\n", __func__));

        ResetCurrentBindIndex();

        // Cursor is required to preform reset logic.
        Cursor cursor(m_stmt);
        auto res = cursor.StepV();
        
        if (res != SQLITE_ROW && res != SQLITE_DONE)
            throw runtime_error(strprintf("%s: Failed execute SQL statement\n", __func__));

        return res;
    }

    void Stmt::TryBindStatementText(int index, const string& value)
    {
        auto res = m_stmt->BindText(index, value);
        if (!CheckValidResult(res))
            throw runtime_error(strprintf("%s: Failed bind SQL statement - index:%d value:%s\n",
                __func__, index, value));
    }

    bool Stmt::TryBindStatementText(int index, const optional<string>& value)
    {
        if (!value) return true;

        TryBindStatementText(index, *value);
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
        int res = m_stmt->BindInt(index, value);
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
        int res = m_stmt->BindInt64(index, value);
        if (!CheckValidResult(res))
            throw runtime_error(strprintf("%s: Failed bind SQL statement - index:%d value:%d\n",
                __func__, index, value));
    }

    bool Stmt::TryBindStatementNull(int index)
    {
        int res = m_stmt->BindNull(index);
        if (!CheckValidResult(res))
            return false;

        return true;
    }

    bool Stmt::CheckValidResult(int result)
    {
        if (result != SQLITE_OK) {
            return false;
        }

        return true;
    }

    void Stmt::ResetCurrentBindIndex()
    {
        m_currentBindIndex = 1;
    }

    void Stmt::Select(const function<void(Cursor&)>& func)
    {
        if (Params().NetworkID() == NetworkId::NetworkRegTest)
            LogPrintf("%s\n", Log());
            
        ResetCurrentBindIndex(); // At this point there will be no more binds
        Cursor cursor(m_stmt);
        func(cursor);
    }

    // ----------------------------------------------
    // CURSOR
    // ----------------------------------------------

    Cursor::~Cursor()
    {
        Reset();
    }

    bool Cursor::Step()
    {
        return (StepV() == SQLITE_ROW);
    }

    int Cursor::StepV()
    {
        ResetCurrentCollectIndex();
        if (!m_stmt) throw runtime_error(strprintf("%s: Cursor::TryGetColumnString() Statement is null\n", __func__));
        return m_stmt->Step();
    }

    template <class T>
    bool _collectSetAndReturn(const std::optional<T>& val, T& valOut)
    {
        if (val) {
            valOut = *val;
        }
        return val.has_value();
    }

    bool Cursor::_collect(const int& index, std::string& val)
    {
        return _collectSetAndReturn(m_stmt->GetColumnText(index), val);
    }

    bool Cursor::_collect(const int& index, optional<string>& val)
    {
        val = m_stmt->GetColumnText(index);
        return val.has_value();
    }

    bool Cursor::_collect(const int& index, int& val)
    {
        return _collectSetAndReturn(m_stmt->GetColumnInt(index), val);
    }

    bool Cursor::_collect(const int& index, optional<int>& val)
    {
        val = m_stmt->GetColumnInt(index);
        return val.has_value();
    }

    bool Cursor::_collect(const int& index, int64_t& val)
    {
        return _collectSetAndReturn(m_stmt->GetColumnInt64(index), val);
    }

    bool Cursor::_collect(const int& index, optional<int64_t>& val)
    {
        val = m_stmt->GetColumnInt64(index);
        return val.has_value();
    }

    bool Cursor::_collect(const int& index, bool& val)
    {
        int v;
        auto res = Collect(index, v);
        if (res) val = !!v;
        return res;
    }

    bool Cursor::_collect(const int& index, optional<bool>& val)
    {
        if (auto res = m_stmt->GetColumnInt64(index); res)
            val = !!*res;
        return val.has_value();
    }

    tuple<bool, string> Cursor::TryGetColumnString(int index)
    {
        return TryGetColumn<std::string>(index);
    }
    tuple<bool, int64_t> Cursor::TryGetColumnInt64(int index)
    {
        return TryGetColumn<int64_t>(index);
    }
    tuple<bool, int> Cursor::TryGetColumnInt(int index)
    {
        return TryGetColumn<int>(index);
    }

    int Cursor::Reset()
    {
        if (!m_stmt) return SQLITE_ERROR;
        auto res = m_stmt->Reset();
        if (res == SQLITE_OK) m_stmt->ClearBindings();
        m_stmt.reset();
        return res;
    }

    StmtWrapper::~StmtWrapper()
    {
        Finalize();
    }

    int StmtWrapper::PrepareV2(SQLiteDatabase& db, const std::string& sql)
    {
        return sqlite3_prepare_v2(db.m_db, sql.c_str(), (int) sql.size(), &m_stmt, nullptr);
    }

    int StmtWrapper::Step()
    {
        return sqlite3_step(m_stmt);
    }

    int StmtWrapper::BindText(int index, const std::string& val)
    {
        return sqlite3_bind_text(m_stmt, index, val.c_str(), (int)val.size(), SQLITE_STATIC);
    }

    int StmtWrapper::BindInt(int index, int val)
    {
        return sqlite3_bind_int(m_stmt, index, val);
    }

    int StmtWrapper::BindInt64(int index, int64_t val)
    {
        return sqlite3_bind_int64(m_stmt, index, val);
    }

    int StmtWrapper::BindNull(int index)
    {
        return sqlite3_bind_null(m_stmt, index);
    }

    int StmtWrapper::GetColumnType(int index)
    {
        return sqlite3_column_type(m_stmt, index);
    }

    optional<string> StmtWrapper::GetColumnText(int index)
    {
        if (GetColumnType(index) != SQLITE_TEXT) {
            return nullopt;
        }
        return string(reinterpret_cast<const char*>(sqlite3_column_text(m_stmt, index)));
    }

    std::optional<int> StmtWrapper::GetColumnInt(int index)
    {
        if (GetColumnType(index) != SQLITE_INTEGER) {
            return nullopt;
        }
        return (int)sqlite3_column_int(m_stmt, index);
    }

    std::optional<int64_t> StmtWrapper::GetColumnInt64(int index)
    {
        if (GetColumnType(index) != SQLITE_INTEGER) {
            return nullopt;
        }
        return (int64_t)sqlite3_column_int64(m_stmt, index);
    }

    int StmtWrapper::Reset()
    {
        return sqlite3_reset(m_stmt);
    }

    int StmtWrapper::Finalize()
    {
        return sqlite3_finalize(m_stmt);
    }
    
    int StmtWrapper::ClearBindings()
    {
        return sqlite3_clear_bindings(m_stmt);
    }

    const char* StmtWrapper::ExpandedSql()
    {
        return sqlite3_expanded_sql(m_stmt);
    }

} // namespace PocketDb