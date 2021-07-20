// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 Bitcoin developers
// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include "WebUserRepository.h"

void PocketDb::WebUserRepository::Init() {}

void PocketDb::WebUserRepository::Destroy() {}

UniValue PocketDb::WebUserRepository::GetUserAddress(std::string& name, int count)
{
    string sql = R"sql(
        SELECT p.String2 as Name, u.AddressHash
        FROM vUsers u
        JOIN Payload p on u.Hash = p.TxHash
        WHERE p.String2 = ?
        GROUP BY u.Id
        ORDER BY u.Id
        LIMIT ?
    )sql";

    auto result = UniValue(UniValue::VARR);

    TryTransactionStep([&]() {
        auto stmt = SetupSqlStatement(sql);

        TryBindStatementText(stmt, 1, name);
        TryBindStatementInt(stmt, 2, count);

        while (sqlite3_step(*stmt) == SQLITE_ROW) {
            UniValue record(UniValue::VOBJ);

            if (auto [ok, valueStr] = TryGetColumnString(*stmt, 0); ok) record.pushKV("name", valueStr);
            if (auto [ok, valueStr] = TryGetColumnString(*stmt, 1); ok) record.pushKV("address", valueStr);

            result.push_back(record);
        }

        FinalizeSqlStatement(*stmt);
    });

    return result;
}
