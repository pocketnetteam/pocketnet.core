// Copyright (c) 2018-2022 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include "pocketdb/repositories/CheckpointRepository.h"

namespace PocketDb
{
    bool CheckpointRepository::IsSocialCheckpoint(const string& txHash, TxType txType, int code)
    {
        bool result = false;

        TryTransactionStep(__func__, [&]()
        {
            auto stmt = SetupSqlStatement(R"sql(
                select 1
                from Social
                where TxHash = ?
                  and TxType = ?
                  and Code = ?
            )sql");

            TryBindStatementText(stmt, 1, txHash);
            TryBindStatementInt(stmt, 2, txType);
            TryBindStatementInt(stmt, 3, code);

            result = sqlite3_step(*stmt) == SQLITE_ROW;

            FinalizeSqlStatement(*stmt);
        });

        return result;
    }

    bool CheckpointRepository::IsLotteryCheckpoint(int height, const string& hash)
    {
        bool result = false;

        TryTransactionStep(__func__, [&]()
        {
            auto stmt = SetupSqlStatement(R"sql(
                select 1
                from Lottery
                where Height = ?
                  and Hash = ?
            )sql");

            TryBindStatementInt(stmt, 1, height);
            TryBindStatementText(stmt, 2, hash);

            result = sqlite3_step(*stmt) == SQLITE_ROW;

            FinalizeSqlStatement(*stmt);
        });

        return result;
    }

    bool CheckpointRepository::IsOpReturnCheckpoint(const string& txHash, const string& hash)
    {
        bool result = false;

        TryTransactionStep(__func__, [&]()
        {
            auto stmt = SetupSqlStatement(R"sql(
                select 1
                from OpReturn
                where TxHash = ?
                  and Hash = ?
            )sql");

            TryBindStatementText(stmt, 1, txHash);
            TryBindStatementText(stmt, 2, hash);

            result = sqlite3_step(*stmt) == SQLITE_ROW;

            FinalizeSqlStatement(*stmt);
        });

        return result;
    }
}
