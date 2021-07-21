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

    TryTransactionStep(__func__, [&]() {
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

UniValue PocketDb::WebUserRepository::GetAddressesRegistrationDates(vector<string>& addresses)
{
    string sql = R"sql(
        WITH addresses (AddressHash, Height) AS (
            SELECT AddressHash, MIN(Height) AS Height
            FROM vUsers
    )sql";

    sql += "WHERE AddressHash IN ('";
    sql += addresses[0];
    sql += "'";
    for (size_t i = 1; i < addresses.size(); i++) {
        sql += ",'";
        sql += addresses[i];
        sql += "'";
    }
    sql += ")";

    sql += R"sql(
            GROUP BY AddressHash
        )
        SELECT u.AddressHash, u.Time, u.Hash
        FROM vUsers u
        INNER JOIN addresses a ON u.AddressHash = a.AddressHash AND u.Height = a.Height
    )sql";

    auto result = UniValue(UniValue::VARR);

    TryTransactionStep(__func__, [&]() {
        auto stmt = SetupSqlStatement(sql);

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
