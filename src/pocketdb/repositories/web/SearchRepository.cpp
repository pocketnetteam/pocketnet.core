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
            where t.Value match ?
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
        auto func = __func__;
        vector<int64_t> ids;

        if (request.Keyword.empty())
            return ids;

        // First search request
        string fieldTypes = join(request.FieldTypes | transformed(static_cast<std::string(*)(int)>(std::to_string)), ",");
        string txTypes = join(request.TxTypes | transformed(static_cast<std::string(*)(int)>(std::to_string)), ",");
        string heightWhere = request.TopBlock > 0 ? " and t.Height <= ? " : "";
        string addressWhere = !request.Address.empty() ? " and t.String1 = ? " : "";
        string keyword = request.Keyword + "*";

        LogPrint(BCLog::RPCERROR, "Search keyword debug = `%s`\n", keyword);

        string sql = R"sql(
            select t.Id
            from Transactions t indexed by Transactions_Type_Last_String1_Height_Id
            where t.Type in ( )sql" + txTypes + R"sql( )
                and t.Last = 1
                and t.Height is not null
                )sql" + heightWhere + R"sql(
                )sql" + addressWhere + R"sql(
                and t.Id in (
                    select cm.ContentId
                    from web.Content c, web.ContentMap cm
                    where c.ROWID = cm.ROWID
                        and cm.FieldType in ( )sql" + fieldTypes + R"sql( )
                        and c.Value match ?
                    )
            order by t.Id desc
            limit ?
            offset ?
        )sql";
        
        TryTransactionStep(func, [&]()
        {
            auto stmt = SetupSqlStatement(sql);

            int i = 1;
            if (request.TopBlock > 0)
                TryBindStatementInt(stmt, i++, request.TopBlock);
            if (!request.Address.empty())
                TryBindStatementText(stmt, i++, request.Address);
            TryBindStatementText(stmt, i++, keyword);
            TryBindStatementInt(stmt, i++, request.PageSize);
            TryBindStatementInt(stmt, i++, request.PageStart);

            LogPrint(BCLog::SQL, "%s: %s\n", func, sqlite3_expanded_sql(*stmt));

            while (sqlite3_step(*stmt) == SQLITE_ROW)
            {
                if (auto[ok, value] = TryGetColumnInt64(*stmt, 0); ok)
                    ids.push_back(value);
            }

            FinalizeSqlStatement(*stmt);
        });

        return ids;
    }

    map<int, string> SearchRepository::SearchUsers(const string& searchstr, const vector<int> fieldTypes, bool orderbyrank)
    {
        auto func = __func__;
        map<int, string> result;

        string sql = R"sql(
            select
                t.Id,
                f.Value,
                fm.FieldType
            from web.Content f
            join web.ContentMap fm on fm.ROWID = f.ROWID
            join Transactions t on t.Id = fm.ContentId
            join Payload p on p.TxHash=t.Hash
            where t.Last = 1
                and t.Type = 100
                and t.Height is not null
                and fm.FieldType in ( )sql" + join(vector<string>(fieldTypes.size(), "?"), ",") + R"sql( )
                and f.Value match ?
        )sql";

        if (orderbyrank)
            sql += " order by rank ";


        TryTransactionStep(__func__, [&]()
        {
            auto stmt = SetupSqlStatement(sql);

            int i = 1;
            for (const auto& fieldtype: fieldTypes)
                TryBindStatementInt(stmt, i++, fieldtype);
                
            TryBindStatementText(stmt, i++, "\"" + searchstr + "\"" + " OR " + searchstr + "*");

            LogPrint(BCLog::SQL, "%s: %s\n", func, sqlite3_expanded_sql(*stmt));

            while (sqlite3_step(*stmt) == SQLITE_ROW)
            {
                auto[ok0, id] = TryGetColumnInt(*stmt, 0);
                auto[ok, value] = TryGetColumnString(*stmt, 1);
                result[id] = value;
            }

            FinalizeSqlStatement(*stmt);
        });

        return result;
    }

    UniValue SearchRepository::GetRecomendedAccountsBySubscriptions(const string& address, int cntOut)
    {
        auto func = __func__;
        UniValue result(UniValue::VARR);

        if (address.empty())
            return result;

        string sql = R"sql(
            select
                t.String2 address,
                p.String2 as name,
                p.String3 as avatar
            from Transactions t indexed by Transactions_Type_Last_String1_String2_Height
            cross join Transactions u indexed by Transactions_Type_Last_String1_Height_Id on u.String1 = t.String2
            cross join Payload p on p.TxHash = u.Hash
            where t.Last = 1
                and t.Type in (302,303)
                and t.Height is not null
                and t.String2 != ?
                and t.String1 in (select s.String1
                                  from Transactions s indexed by Transactions_Type_Last_String2_Height
                                  where s.Type in (302,303)
                                    and s.Last = 1
                                    and s.Height is not null
                                    and s.String2 = ?)
                and u.Type in (100,101,102)
                and u.Last=1
                and u.Height is not null
                group by t.String2
                order by count(*) desc
                limit ?
        )sql";

        TryTransactionStep(__func__, [&]()
        {
            auto stmt = SetupSqlStatement(sql);

            int i = 1;
            TryBindStatementText(stmt, i++, address);
            TryBindStatementText(stmt, i++, address);
            TryBindStatementInt(stmt, i++, cntOut);

            LogPrint(BCLog::SQL, "%s: %s\n", func, sqlite3_expanded_sql(*stmt));

            while (sqlite3_step(*stmt) == SQLITE_ROW)
            {
                UniValue record(UniValue::VOBJ);
                if (auto[ok, value] = TryGetColumnInt(*stmt, 0); ok) record.pushKV("address", value);
                if (auto[ok, value] = TryGetColumnInt(*stmt, 0); ok) record.pushKV("name", value);
                if (auto[ok, value] = TryGetColumnInt(*stmt, 0); ok) record.pushKV("avatar", value);
                result.push_back(record);
            }

            FinalizeSqlStatement(*stmt);
        });

        return result;
    }

    UniValue SearchRepository::GetRecomendedAccountsByScoresOnSimilarAccounts(const string& address, const vector<int>& contentTypes, int nHeight, int depth, int cntOut)
    {
        auto func = __func__;
        UniValue result(UniValue::VARR);

        if (address.empty())
            return result;

        string contentTypesFilter = join(vector<string>(contentTypes.size(), "?"), ",");

        string sql = R"sql(
            select tOtherContents.String1 as address,
                   p.String2              as name,
                   p.String3              as avatar
            from Transactions tOtherContents
                     indexed by Transactions_Type_Last_String1_String2_Height
                     cross join Transactions u indexed by Transactions_Type_Last_String1_Height_Id
                                on u.String1 = tOtherContents.String1
                     cross join Payload p on p.TxHash = u.Hash
            where tOtherContents.String2 in (
                select tOtherLikes.String2 as OtherLikedContent
                from Transactions tOtherlikes
                where tOtherLikes.String1 in (
                    select tLikes.String1 as Liker
                    from Transactions tLikes
                    where tLikes.String2 in (
                        select tContents.String2 as BloggerContent
                        from Transactions tContents
                        where tContent.Type in ( )sql" + contentTypesFilter + R"sql( )
                          and tContents.Last = 1
                          and tContents.String1 = ?
                          and tContents.Height >= ?
                    )
                      and tLikes.Type in (300)
                      and tLikes.Last in (1, 0)
                      and tLikes.Int1 > 3
                      and tLikes.Height >= ?
                )
                  and tOtherLikes.Type in (300)
                  and tOtherLikes.Last in (1, 0)
                  and tOtherLikes.Int1 > 3
                  and tOtherLikes.Height >= ?
            )
              and tOtherContents.Type in ( )sql" + contentTypesFilter + R"sql( )
              and tOtherContents.String1 != ?
              and tOtherContents.Last = 1
              and tOtherContents.Height >= ?
            group by tOtherContents.String1
            order by count(*) desc
            limit ?
        )sql";

        TryTransactionStep(__func__, [&]()
        {
            auto stmt = SetupSqlStatement(sql);

            int i = 1;
            for (const auto& contenttype: contentTypes)
                TryBindStatementInt(stmt, i++, contenttype);
            TryBindStatementText(stmt, i++, address);
            TryBindStatementInt(stmt, i++, nHeight - depth);
            TryBindStatementInt(stmt, i++, nHeight - depth);
            TryBindStatementInt(stmt, i++, nHeight - depth);
            for (const auto& contenttype: contentTypes)
                TryBindStatementInt(stmt, i++, contenttype);
            TryBindStatementText(stmt, i++, address);
            TryBindStatementInt(stmt, i++, nHeight - depth);
            TryBindStatementInt(stmt, i++, cntOut);

            LogPrint(BCLog::SQL, "%s: %s\n", func, sqlite3_expanded_sql(*stmt));

            while (sqlite3_step(*stmt) == SQLITE_ROW)
            {
                UniValue record(UniValue::VOBJ);
                if (auto[ok, value] = TryGetColumnInt(*stmt, 0); ok) record.pushKV("address", value);
                if (auto[ok, value] = TryGetColumnInt(*stmt, 0); ok) record.pushKV("name", value);
                if (auto[ok, value] = TryGetColumnInt(*stmt, 0); ok) record.pushKV("avatar", value);
                result.push_back(record);
            }

            FinalizeSqlStatement(*stmt);
        });

        return result;
    }

    UniValue SearchRepository::GetRecomendedAccountsByScoresFromAddress(const string& address, const vector<int>& contentTypes, int nHeight, int depth, int cntOut)
    {
        auto func = __func__;
        UniValue result(UniValue::VARR);

        if (address.empty())
            return result;

        string contentTypesFilter = join(vector<string>(contentTypes.size(), "?"), ",");

        string sql = R"sql(
            select tOtherContents.String1 as address,
                   p.String2              as name,
                   p.String3              as avatar
            from Transactions tOtherContents
                     indexed by Transactions_Type_Last_String2_Height
                     join Transactions u on u.String1 = tOtherContents.String1
                     join Payload p on p.TxHash = u.Hash
            where tOtherContents.String2 in (
                select tOtherLikes.String2 as OtherLikedContent
                from Transactions tOtherlikes
                where tOtherLikes.String1 in (
                    select tLikes.String1 as Liker
                    from Transactions tLikes
                    where tLikes.String2 in (
                        select tAddressLikes.String2 as ContentsLikedByAddress
                        from Transactions tAddressLikes
                        where tAddressLikes.String1 = ?
                          and tAddressLikes.Type in (300)
                          and tAddressLikes.Last in (1, 0)
                          and tAddressLikes.Int1 > 3
                          and tAddressLikes.Height >= ?
                    )
                      and tLikes.Type in (300)
                      and tLikes.Last in (1, 0)
                      and tLikes.Int1 > 3
                      and tLikes.Height >= ?
                )
                  and tOtherLikes.Type in (300)
                  and tOtherLikes.Last in (1, 0)
                  and tOtherLikes.Int1 > 3
                  and tOtherLikes.Height >= ?
            )
              and tOtherContents.Type in ( )sql" + contentTypesFilter + R"sql( )
              and tOtherContents.String1 != ?
              and tOtherContents.Last = 1
              and tOtherContents.Height >= ?
            group by tOtherContents.String1
            order by count(*) desc
            limit ?;
        )sql";

        TryTransactionStep(__func__, [&]()
        {
            auto stmt = SetupSqlStatement(sql);

            int i = 1;
            TryBindStatementText(stmt, i++, address);


            for (const auto& contenttype: contentTypes)
                TryBindStatementInt(stmt, i++, contenttype);
            TryBindStatementInt(stmt, i++, nHeight - depth);
            TryBindStatementInt(stmt, i++, nHeight - depth);
            TryBindStatementInt(stmt, i++, nHeight - depth);
            for (const auto& contenttype: contentTypes)
                TryBindStatementInt(stmt, i++, contenttype);
            TryBindStatementText(stmt, i++, address);
            TryBindStatementInt(stmt, i++, nHeight - depth);
            TryBindStatementInt(stmt, i++, cntOut);

            LogPrint(BCLog::SQL, "%s: %s\n", func, sqlite3_expanded_sql(*stmt));

            while (sqlite3_step(*stmt) == SQLITE_ROW)
            {
                UniValue record(UniValue::VOBJ);
                if (auto[ok, value] = TryGetColumnInt(*stmt, 0); ok) record.pushKV("address", value);
                if (auto[ok, value] = TryGetColumnInt(*stmt, 0); ok) record.pushKV("name", value);
                if (auto[ok, value] = TryGetColumnInt(*stmt, 0); ok) record.pushKV("avatar", value);
                result.push_back(record);
            }

            FinalizeSqlStatement(*stmt);
        });

        return result;
    }

    UniValue SearchRepository::GetRecomendedContentsByScoresOnSimilarContents()
    {
        return UniValue();
    }

    UniValue SearchRepository::GetRecomendedContentsByScoresFromAddress()
    {
        return UniValue();
    }
}