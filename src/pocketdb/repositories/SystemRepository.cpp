// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include "pocketdb/repositories/SystemRepository.h"

namespace PocketDb
{
    int SystemRepository::GetDbVersion()
    {
        int result = -1;

        SqlTransaction(__func__, [&]()
        {
            Sql(R"sql(
                PRAGMA user_version
            )sql")
            .Select([&](Cursor& cursor) {
                if (cursor.Step())
                    cursor.CollectAll(result);
            });
        });

        return result;
    }

    void SystemRepository::SetDbVersion(int version)
    {
        SqlTransaction(__func__, [&]()
        {
            Sql(R"sql( PRAGMA user_version = )sql" + to_string(version))
            .Run();
        });
    }

}
