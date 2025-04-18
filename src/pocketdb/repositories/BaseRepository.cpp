// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include "pocketdb/repositories/BaseRepository.h"
#include "init.h"

namespace PocketDb
{
    string BaseRepository::EscapeValue(string value)
    {
        boost::replace_all(value, "%", "\\%");
        boost::replace_all(value, "_", "\\_");

        return value;
    }

    void BaseRepository::BenchLog(const string& func, double time)
    {
        LogPrint(BCLog::SQLBENCH, "SQL Bench `%s`: %.2fms\n", func, time);
        gStatEngineInstance.SetSqlBench(func, time);
    }

    Stmt& BaseRepository::Sql(const string& sql)
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

    Stmt BaseRepository::SqlSingleton(const string& sql)
    {
        Stmt stmt;
        stmt.Init(m_database, sql);
        return stmt;
    }

    void BaseRepository::SetLastInsertRowId(int64_t value)
    {
        sqlite3_set_last_insert_rowid(m_database.m_db, value);
    }

    int64_t BaseRepository::GetLastInsertRowId()
    {
        return sqlite3_last_insert_rowid(m_database.m_db);
    }


} // namespace PocketDb
