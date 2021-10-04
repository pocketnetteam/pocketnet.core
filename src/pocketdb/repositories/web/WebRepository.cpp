// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include "WebRepository.h"

namespace PocketDb
{
    void WebRpcRepository::Init() {}

    void WebRpcRepository::Destroy() {}

    UniValue WebRepository::____(const string& address)
    {
        UniValue result(UniValue::VOBJ);

        string sql = R"sql(

        )sql";

        TryTransactionStep(__func__, [&]()
        {
            auto stmt = SetupSqlStatement(sql);

            TryBindStatementText(stmt, 1, address);

            if (sqlite3_step(*stmt) == SQLITE_ROW)
            {
//                if (auto[ok, value] = TryGetColumnString(*stmt, 0); ok) result.pushKV("address", value);
//                if (auto[ok, value] = TryGetColumnInt64(*stmt, 1); ok) result.pushKV("id", value);
            }

            FinalizeSqlStatement(*stmt);
        });

        return result;
    }

}