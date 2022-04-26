// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include "pocketdb/repositories/SystemRepository.h"

namespace PocketDb
{
    int SystemRepository::GetDbVersion(const string& db)
    {
        int result = -1;

        TryTransactionStep(__func__, [&]()
        {
            auto stmt = SetupSqlStatement(R"sql(
                select Version
                from System
                where Db = ?
            )sql");

            TryBindStatementText(stmt, 1, db);

            if (sqlite3_step(*stmt) == SQLITE_ROW)
                if (auto[ok, value] = TryGetColumnInt(*stmt, 0); ok)
                    result = value;

            FinalizeSqlStatement(*stmt);
        });

        return result;
    }

    void SystemRepository::SetDbVersion(const string& db, int version)
    {
        TryTransactionStep(__func__, [&]()
        {
            auto stmt = SetupSqlStatement(R"sql(
                update System
                    set Version = ?
                where Db = ?
            )sql");

            TryBindStatementInt(stmt, 1, version);
            TryBindStatementText(stmt, 2, db);
            TryStepStatement(stmt);
        });
    }

}
