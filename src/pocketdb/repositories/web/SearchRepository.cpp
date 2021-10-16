// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include "pocketdb/repositories/web/SearchRepository.h"

namespace PocketDb
{
    void SearchRepository::Init() {}

    void SearchRepository::Destroy() {}

    UniValue SearchRepository::SearchTags(const SearchRequest& request)
    {
        UniValue result(UniValue::VARR);

        string keyword = "%" + request.Keyword + "%";
        string sql = R"sql(
            select Value
            from Tags t indexed by Tags_Value
            where t.Value like ?
            limit ?
            offset ?
        )sql";

        TryTransactionStep(__func__, [&]()
        {
            auto stmt = SetupSqlStatement(sql);
            TryBindStatementText(stmt, 1, keyword);
            TryBindStatementInt(stmt, 2, request.PageSize);
            TryBindStatementInt(stmt, 3, request.PageStart);

            while (sqlite3_step(*stmt) == SQLITE_ROW)
            {
                if (auto[ok, value] = TryGetColumnString(*stmt, 0); ok)
                    result.push_back(value);
            }

            FinalizeSqlStatement(*stmt);
        });

        return result;
    }

    vector<int64_t> SearchRepository::SearchIds(const SearchRequest& request)
    {
        vector<int64_t> ids;

        // First search request
        string keyword = "%" + request.Keyword + "%";
        string fieldTypes = join(request.FieldTypes | transformed(static_cast<std::string(*)(int)>(std::to_string)), ",");
        string txTypes = join(request.TxTypes | transformed(static_cast<std::string(*)(int)>(std::to_string)), ",");
        string heightWhere = request.TopBlock > 0 ? " and t.Height <= ? " : "";
        string addressWhere = !request.Address.empty() ? " and t.String1 = ? " : "";

        string sql = R"sql(
            select t.Id
            from Transactions t indexed by Transactions_Type_String1_Height_Time_Int1
            join web.Content c on c.FieldType in ()sql" + fieldTypes + R"sql() and c.Value like ? and c.ContentId = t.Id
            where t.Type in ()sql" + txTypes + R"sql()
                and t.Height is not null
                )sql" + heightWhere + R"sql(
                )sql" + addressWhere + R"sql(
            order by t.Height desc
            limit ?
            offset ?
        )sql";
        
        TryTransactionStep(__func__, [&]()
        {
            auto stmt = SetupSqlStatement(sql);
            int i = 1;
            TryBindStatementText(stmt, i++, keyword);

            if (request.TopBlock > 0)
                TryBindStatementInt(stmt, i++, request.TopBlock);

            if (!request.Address.empty())
                TryBindStatementText(stmt, i++, request.Address);

            TryBindStatementInt(stmt, i++, request.PageSize);
            TryBindStatementInt(stmt, i++, request.PageStart);

            while (sqlite3_step(*stmt) == SQLITE_ROW)
            {
                if (auto[ok, value] = TryGetColumnInt64(*stmt, 0); ok)
                    ids.push_back(value);
            }

            FinalizeSqlStatement(*stmt);
        });

        return ids;
    }

}