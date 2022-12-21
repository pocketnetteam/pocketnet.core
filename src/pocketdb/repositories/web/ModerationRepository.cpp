// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include "pocketdb/repositories/web/ModerationRepository.h"

namespace PocketDb
{
    void ModerationRepository::Init() {}

    void ModerationRepository::Destroy() {}

    UniValue ModerationRepository::GetAssignedJury(const string& address, const Pagination& pagination)
    {
        UniValue result(UniValue::VARR);

        TryTransactionStep(__func__, [&]()
        {
            string orderBy = " order by f.Height ";
            orderBy += (pagination.Desc ? " desc " : " asc ");

            auto stmt = SetupSqlStatement(R"sql(
                select f.Hash
                from Transactions u indexed by Transactions_Type_Last_String1_Height_Id
                cross join JuryModers jm on jm.AccountId = u.Id
                cross join Transactions f on f.ROWID = jm.FlagRowId
                where u.Type in (100)
                  and u.Last = 1
                  and u.Height is not null
                  and u.String1 ? 
                  and f.Height <= ?
                )sql" + orderBy + R"sql(
                limit ? offset ?
            )sql");

            TryBindStatementText(stmt, 1, address);
            TryBindStatementInt(stmt, 2, pagination.TopHeight);
            TryBindStatementInt(stmt, 3, pagination.PageSize);
            TryBindStatementInt(stmt, 4, pagination.PageStart);
            
            while (sqlite3_step(*stmt) == SQLITE_ROW)
            {
                if (auto[ok, hash] = TryGetColumnInt(*stmt, 0); ok)
                    result.push_back(hash);
            }

            FinalizeSqlStatement(*stmt);
        });

        return result;
    }

}