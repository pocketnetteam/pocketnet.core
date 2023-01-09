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
            static auto stmt = SetupSqlStatement(R"sql(
                select Version
                from System
                where Db = ?
            )sql");

            stmt->Bind(db);

            if (stmt->Step() == SQLITE_ROW)
                if (auto[ok, value] = stmt->TryGetColumnInt(0); ok)
                    result = value;

            stmt->Reset();
        });

        return result;
    }

    void SystemRepository::SetDbVersion(const string& db, int version)
    {
        TryTransactionStep(__func__, [&]()
        {
            static auto stmt = SetupSqlStatement(R"sql(
                update System
                    set Version = ?
                where Db = ?
            )sql");

            stmt->Bind(version, db);
            TryStepStatement(stmt);
        });
    }

}
