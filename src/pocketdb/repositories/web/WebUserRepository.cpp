// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include "pocketdb/repositories/web/WebUserRepository.h"

void PocketDb::WebUserRepository::Init() {}

void PocketDb::WebUserRepository::Destroy() {}

UniValue PocketDb::WebUserRepository::GetUserAddress(std::string& name)
{
    string sql = R"sql(
        SELECT p.String2, u.String1
        FROM Transactions u
        JOIN Payload p on u.Hash = p.TxHash
        WHERE   u.Type in (100, 101, 102)
            and p.String2 = ?
        LIMIT 1
    )sql";

    auto result = UniValue(UniValue::VARR);

    TryTransactionStep(__func__, [&]() {
        auto stmt = SetupSqlStatement(sql);

        TryBindStatementText(stmt, 1, name);

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

UniValue PocketDb::WebUserRepository::GetAddressesRegistrationDates(vector<string>& addresses)
{
    string sql = R"sql(
        WITH addresses (String1, Height) AS (
            SELECT u.String1, MIN(u.Height) AS Height
            FROM Transactions u
            WHERE u.String1 IN ( )sql" + join(vector<string>(addresses.size(), "?"), ",") + R"sql( )
            GROUP BY u.String1   
        )
        SELECT u.String1, u.Time, u.Hash
        FROM Transactions u
        JOIN addresses a ON u.String1 = a.String1 AND u.Height = a.Height
    )sql";

    auto result = UniValue(UniValue::VARR);

    TryTransactionStep(__func__, [&]() {
        auto stmt = SetupSqlStatement(sql);

        for (size_t i = 0; i < addresses.size(); i++)
            TryBindStatementText(stmt, (int)i + 1, addresses[i]);

        while (sqlite3_step(*stmt) == SQLITE_ROW) {
            UniValue record(UniValue::VOBJ);

            if (auto [ok, valueStr] = TryGetColumnString(*stmt, 0); ok) record.pushKV("address", valueStr);
            if (auto [ok, valueStr] = TryGetColumnString(*stmt, 1); ok) record.pushKV("time", valueStr);
            if (auto [ok, valueStr] = TryGetColumnString(*stmt, 2); ok) record.pushKV("txid", valueStr);

            result.push_back(record);
        }

        FinalizeSqlStatement(*stmt);
    });

    return result;
}
