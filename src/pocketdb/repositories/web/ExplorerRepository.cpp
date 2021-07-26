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

    map<PocketTxType, int> ExplorerRepository::GetStatistic(int64_t startTime, int64_t endTime)
    {
        map<PocketTxType, int> result;

        auto stmt = SetupSqlStatement(R"sql(
            select t.Type, count(*)
            from Transactions t
            where   t.Time >= ?
                and t.Time < ?
                and t.Type > 3
            group by t.Type
        )sql");

        TryBindStatementInt64(stmt, 1, startTime);
        TryBindStatementInt64(stmt, 2, endTime);

        TryTransactionStep(__func__, [&]()
        {
            while (sqlite3_step(*stmt) == SQLITE_ROW)
            {
                auto [ok0, sType] = TryGetColumnInt(*stmt, 0);
                auto [ok1, sCount] = TryGetColumnInt(*stmt, 1);

                if (ok0 && ok1)
                    result[(PocketTxType)sType] = sCount;
            }

            FinalizeSqlStatement(*stmt);
        });

        return result;
    }

    tuple<int64_t, int64_t> ExplorerRepository::GetAddressSpent(const string& addressHash)
    {
        int64_t spent = 0;
        int64_t unspent = 0;

        auto stmt = SetupSqlStatement(R"sql(
            select (o.SpentHeight isnull), sum(o.Value)
            from TxOutputs o
            where o.AddressHash = ?
            group by (o.SpentHeight isnull)
        )sql");

        TryBindStatementText(stmt, 1, addressHash);

        TryTransactionStep(__func__, [&]()
        {
            while (sqlite3_step(*stmt) == SQLITE_ROW)
            {
                auto [ok0, sUnspent] = TryGetColumnInt(*stmt, 0);
                auto [ok1, sAmount] = TryGetColumnInt64(*stmt, 1);

                if (ok0 && ok1)
                    if (sUnspent == 1)
                        unspent = sAmount;
                    else
                        spent = sAmount;
            }

            FinalizeSqlStatement(*stmt);
        });

        return {spent, unspent};
    }
    
    UniValue GetAddressTransactions(const string& addressHash, int firstNumber)
    {
        // TODO (brangr): implement
    }

}