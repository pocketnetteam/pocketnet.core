// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include "ExplorerRepository.h"

namespace PocketDb {

    void ExplorerRepository::Init() {}

    void ExplorerRepository::Destroy() {}

    map<PocketTxType, map<int, int>> ExplorerRepository::GetStatistic(int lastHeight, int count)
    {
        map<PocketTxType, map<int, int>> result;

        auto stmt = SetupSqlStatement(R"sql(
            select t.Type, t.Height, count(*)
            from Transactions t
            where   t.Height > ?
                and t.Height <= ?
            group by t.Type, t.Height
        )sql");

        TryBindStatementInt(stmt, 1, lastHeight);
        TryBindStatementInt(stmt, 2, count);

        TryTransactionStep(__func__, [&]()
        {
            while (sqlite3_step(*stmt) == SQLITE_ROW)
            {
                auto [ok0, sType] = TryGetColumnInt(*stmt, 0);
                auto [ok1, sBlock] = TryGetColumnInt(*stmt, 1);
                auto [ok2, sCount] = TryGetColumnInt(*stmt, 2);

                if (ok0 && ok1 && ok2)
                    result[(PocketTxType)sType][sBlock] = sCount;
            }

            FinalizeSqlStatement(*stmt);
        });

        return result;
    }

}