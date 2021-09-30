//
// Created by Joknek on 9/30/2021.
//

#include "NotifierRepository.h"

namespace PocketDb
{
    void NotifierRepository::Init() {}

    void NotifierRepository::Destroy() {}

    UniValue NotifierRepository::GetPostLang(const string &postHash)
    {
        UniValue result(UniValue::VOBJ);

        string sql = R"sql(
            select p.String1 Lang from Transactions t
            inner join Payload p on t.Hash = p.TxHash
            where Type in (200, 201, 202, 203) and t.Hash = ?
        )sql";

        TryTransactionStep(__func__, [&]()
        {
            auto stmt = SetupSqlStatement(sql);

            TryBindStatementText(stmt, 1, postHash);

            if (sqlite3_step(*stmt) == SQLITE_ROW)
            {
                if (auto[ok, value] = TryGetColumnString(*stmt, 0); ok) result.pushKV("lang", value);
            }

            FinalizeSqlStatement(*stmt);
        });

        return result;
    }
}