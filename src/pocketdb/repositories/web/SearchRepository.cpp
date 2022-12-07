// Copyright (c) 2018-2022 The Pocketnet developers
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
        string keyword = "\"" + request.Keyword + "\"" + " OR " + request.Keyword + "*";

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
                    order by rank
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

            while (sqlite3_step(*stmt) == SQLITE_ROW)
            {
                if (auto[ok, value] = TryGetColumnInt64(*stmt, 0); ok)
                    ids.push_back(value);
            }

            FinalizeSqlStatement(*stmt);
        });

        return ids;
    }

    vector<int64_t> SearchRepository::SearchUsersOld(const SearchRequest& request)
    {
        auto func = __func__;
        vector<int64_t> result;

        string heightWhere = request.TopBlock > 0 ? " and t.Height <= ? " : "";
        string keyword = "\"" + request.Keyword + "\"" + " OR " + request.Keyword + "*";

        string sql = R"sql(
            select
                t.Id

            from web.Content f

            join web.ContentMap fm on fm.ROWID = f.ROWID

            cross join Transactions t indexed by Transactions_Last_Id_Height
                on t.Id = fm.ContentId

            cross join Payload p on p.TxHash=t.Hash

            where t.Last = 1
                and t.Type = 100
                and t.Height is not null
                )sql" + heightWhere + R"sql(
                and fm.FieldType in ( )sql" + join(vector<string>(request.FieldTypes.size(), "?"), ",") + R"sql( )
                and f.Value match ?
        )sql";

        if (request.OrderByRank)
            sql += " order by rank, t.Id ";

        sql += " limit ? ";
        sql += " offset ? ";

        TryTransactionStep(__func__, [&]()
        {
            int i = 1;
            auto stmt = SetupSqlStatement(sql);

            if (request.TopBlock > 0)
                TryBindStatementInt(stmt, i++, request.TopBlock);
            for (const auto& fieldtype: request.FieldTypes)
                TryBindStatementInt(stmt, i++, fieldtype);
            TryBindStatementText(stmt, i++, keyword);
            TryBindStatementInt(stmt, i++, request.PageSize);
            TryBindStatementInt(stmt, i++, request.PageStart);

            while (sqlite3_step(*stmt) == SQLITE_ROW)
            {
                if (auto[ok, value] = TryGetColumnInt64(*stmt, 0); ok) result.push_back(value);
            }

            FinalizeSqlStatement(*stmt);
        });

        return result;
    }

    vector<int64_t> SearchRepository::SearchUsers(const string& keyword)
    {
        auto func = __func__;
        vector<int64_t> result;

        string _keyword = "\"" + keyword + "\"" + " OR " + keyword + "*";

        string sql = R"sql(
            select names.* from (
                select
                      fm.ContentId
                    , ROW_NUMBER() OVER ( ORDER BY RANK, length(f.Value) ) as ROWNUMBER
                    , RANK as RNK
                    , r.Value as Rating
                from web.Content f
                join web.ContentMap fm on fm.ROWID = f.ROWID
                left join Ratings r on r.Id = fm.ContentId and r.Last = 1 and r.Type = 0
                where fm.FieldType in (?)
                    and f.Value match ?
                limit ?
            ) names

            union

            select about.* from (
                select
                      fm.ContentId
                    , ROW_NUMBER() OVER ( ORDER BY r.Value DESC) as ROWNUMBER
                    , RANK as RNK
                    , r.Value as Rating
                from web.Content f
                join web.ContentMap fm on fm.ROWID = f.ROWID
                left join Ratings r on r.Id = fm.ContentId and r.Last = 1 and r.Type = 0
                where fm.FieldType in (?)
                    and f.Value match ?
                limit ?
            ) about

            order by ROWNUMBER, RNK, Rating desc
        )sql";

        TryTransactionStep(__func__, [&]()
        {
            int i = 1;
            auto stmt = SetupSqlStatement(sql);

            TryBindStatementInt(stmt, i++, (int)ContentFieldType::ContentFieldType_AccountUserName);
            TryBindStatementText(stmt, i++, _keyword);
            TryBindStatementInt(stmt, i++, 10);

            TryBindStatementInt(stmt, i++, (int)ContentFieldType::ContentFieldType_AccountUserAbout);
            TryBindStatementText(stmt, i++, _keyword);
            TryBindStatementInt(stmt, i++, 10);

            while (sqlite3_step(*stmt) == SQLITE_ROW)
            {
                if (auto[ok, value] = TryGetColumnInt64(*stmt, 0); ok)
                    if (find(result.begin(), result.end(), value) == result.end())
                        result.push_back(value);
            }

            FinalizeSqlStatement(*stmt);
        });

        return result;
    }

    vector<string> SearchRepository::GetRecommendedAccountByAddressSubscriptions(const string& address, string& addressExclude, const vector<int>& contentTypes, const string& lang, int cntOut, int nHeight, int depth)
    {
        auto func = __func__;
        vector<string> ids;

        if (address.empty())
            return ids;

        string contentTypesFilter = join(vector<string>(contentTypes.size(), "?"), ",");

        string excludeAddressFilter = "?";
        if (!addressExclude.empty())
            excludeAddressFilter += ", ?";

        string langFilter = "";
        if (!lang.empty())
            langFilter = "cross join Payload lang on lang.TxHash = u.Hash and lang.String1 = ?";

        int minReputation = 300;
        int limitSubscriptions = 30;
        int limitSubscriptionsTotal = 30;
        int cntRates = 1;

        string sql = R"sql(
            select Contents.String1
            from Transactions Rates indexed by Transactions_Type_Last_String1_Height_Id
            cross join Transactions Contents indexed by Transactions_Type_Last_String2_Height
                on Contents.String2 = Rates.String2
                    and Contents.Last = 1
                    and Contents.Height > 0
                    and Contents.Type in ( )sql" + contentTypesFilter + R"sql( )
                    and Contents.String1 not in ( )sql" + excludeAddressFilter + R"sql( )
            cross join Transactions u indexed by Transactions_Type_Last_String1_Height_Id
                on u.String1 = Contents.String1
                    and u.Type in (100)
                    and u.Last = 1
                    and u.Height > 0
            )sql" + langFilter + R"sql(
            where Rates.Type in (300)
                and Rates.Int1 = 5
                and Rates.Height > ?
                and Rates.Last in (0, 1)
                and Rates.String1 in
               (
               select address
               from (
                    select rnk, address
                    from (
                         select 1 as rnk, subscribes.String2 as address
                         from Transactions subscribes indexed by Transactions_String1_Last_Height
                          cross join Transactions u indexed by Transactions_Type_Last_String1_Height_Id
                             on u.Type in (100)
                                 and u.Last = 1 and u.Height > 0
                                 and u.String1 = subscribes.String2
                          cross join Ratings r indexed by Ratings_Type_Id_Last_Value
                             on r.Id = u.Id
                                 and r.Type = 0
                                 and r.Last = 1
                                 and r.Value > ?
                         where subscribes.Type in (302, 303)
                           and subscribes.Last = 1
                           and subscribes.String1 = ?
                           and subscribes.Height is not null
                         order by subscribes.Height desc
                         limit ?
                         )
                    union
                    select rnk, address
                    from (
                         select 2 as rnk, subscribers.String1 as address
                         from Transactions subscribers indexed by Transactions_Type_Last_String2_Height
                          cross join Transactions u indexed by Transactions_Type_Last_String1_Height_Id
                             on u.Type in (100)
                                 and u.Last = 1 and u.Height > 0
                                 and u.String1 = subscribers.String1
                          cross join Ratings r indexed by Ratings_Type_Id_Last_Value
                             on r.Id = u.Id
                                 and r.Type = 0
                                 and r.Last = 1
                                 and r.Value > ?
                         where subscribers.Type in (302, 303)
                           and subscribers.Last = 1
                           and subscribers.String2 = ?
                           and subscribers.Height is not null
                         order by random()
                         limit ?
                         ))
               order by rnk
               limit ?
               )
            group by Contents.String1
            having count(*) > ?
            order by count (*) desc
            limit ?
        )sql";

        TryTransactionStep(__func__, [&]()
        {
            auto stmt = SetupSqlStatement(sql);

            int i = 1;
            for (const auto& contenttype: contentTypes)
                TryBindStatementInt(stmt, i++, contenttype);

            TryBindStatementText(stmt, i++, address);
            if (!addressExclude.empty())
                TryBindStatementText(stmt, i++, addressExclude);

            if (!lang.empty())
                TryBindStatementText(stmt, i++, lang);

            TryBindStatementInt(stmt, i++, nHeight-depth);

            TryBindStatementInt(stmt, i++, minReputation);
            TryBindStatementText(stmt, i++, address);
            TryBindStatementInt(stmt, i++, limitSubscriptions);

            TryBindStatementInt(stmt, i++, minReputation);
            TryBindStatementText(stmt, i++, address);
            TryBindStatementInt(stmt, i++, limitSubscriptions);

            TryBindStatementInt(stmt, i++, limitSubscriptionsTotal);

            TryBindStatementInt(stmt, i++, cntRates);

            TryBindStatementInt(stmt, i++, cntOut);

            while (sqlite3_step(*stmt) == SQLITE_ROW)
            {
                if (auto[ok, value] = TryGetColumnString(*stmt, 0); ok)
                    ids.push_back(value);
            }

            FinalizeSqlStatement(*stmt);
        });

        return ids;
    }

    vector<int64_t> SearchRepository::GetRecommendedContentByAddressSubscriptions(const string& contentAddress, string& address, const vector<int>& contentTypes, const string& lang, int cntOut, int nHeight, int depth)
    {
        auto func = __func__;
        vector<int64_t> ids;

        if (contentAddress.empty())
            return ids;

        string contentTypesFilter = join(vector<string>(contentTypes.size(), "?"), ",");

        string excludeAddressFilter = "?";
        if (!address.empty())
            excludeAddressFilter += ", ?";

        string langFilter = "";
        if (!lang.empty())
            langFilter = "cross join Payload lang indexed by Payload_String1_TxHash on lang.TxHash = Contents.Hash and lang.String1 = ?";

        int minReputation = 300;
        int limitSubscriptions = 50;
        int limitSubscriptionsTotal = 30;
        int cntRates = 1;

        string sql = R"sql(
            select recomendations.Id
            from (
                 select Contents.String1,
                        Contents.Id,
                        --Rates.String2,
                        count(*) count
                 from Transactions Rates indexed by Transactions_Type_Last_String1_Height_Id
                  cross join Transactions Contents indexed by Transactions_Type_Last_String2_Height
                     on Contents.String2 = Rates.String2
                         and Contents.Last = 1
                         and Contents.Height > 0
                         and Contents.Type in ( )sql" + contentTypesFilter + R"sql( )
                         and Contents.String1 not in ( )sql" + excludeAddressFilter + R"sql( )
                  )sql" + langFilter + R"sql(
                 where Rates.Type in (300)
                   and Rates.Int1 = 5
                   and Rates.Height > ?
                   and Rates.Last in (0, 1)
                   and Rates.String1 in
                       (
                       select address
                       from (
                            select rnk, address
                            from (
                                 select 1 as rnk, subscribes.String2 as address
                                 from Transactions subscribes indexed by Transactions_String1_Last_Height
                                  cross join Transactions u indexed by Transactions_Type_Last_String1_Height_Id
                                     on u.Type in (100)
                                         and u.Last = 1 and u.Height > 0
                                         and u.String1 = subscribes.String2
                                  cross join Ratings r indexed by Ratings_Type_Id_Last_Value
                                     on r.Id = u.Id
                                         and r.Type = 0
                                         and r.Last = 1
                                         and r.Value > ?
                                 where subscribes.Type in (302, 303)
                                   and subscribes.Last = 1
                                   and subscribes.String1 = ?
                                   and subscribes.Height is not null
                                 order by subscribes.Height desc
                                 limit ?
                                 )
                            union
                            select rnk, address
                            from (
                                 select 2 as rnk, subscribers.String1 as address
                                 from Transactions subscribers indexed by Transactions_Type_Last_String2_Height
                                  cross join Transactions u indexed by Transactions_Type_Last_String1_Height_Id
                                     on u.Type in (100)
                                         and u.Last = 1 and u.Height > 0
                                         and u.String1 = subscribers.String1
                                  cross join Ratings r indexed by Ratings_Type_Id_Last_Value
                                     on r.Id = u.Id
                                         and r.Type = 0
                                         and r.Last = 1
                                         and r.Value > ?
                                 where subscribers.Type in (302, 303)
                                   and subscribers.Last = 1
                                   and subscribers.String2 = ?
                                   and subscribers.Height is not null
                                 order by random()
                                 limit ?
                                 ))
                       order by rnk
                       limit ?
                       )
                 --group by Rates.String2
                 group by Contents.Id
                 having count(*) > ?
                 order by count(*) desc
                 ) recomendations
            group by recomendations.String1
            order by recomendations.count desc
            limit ?
        )sql";

        TryTransactionStep(__func__, [&]()
        {
            auto stmt = SetupSqlStatement(sql);

            int i = 1;
            for (const auto& contenttype: contentTypes)
                TryBindStatementInt(stmt, i++, contenttype);

            TryBindStatementText(stmt, i++, contentAddress);
            if (!address.empty())
                TryBindStatementText(stmt, i++, address);

            if (!lang.empty())
                TryBindStatementText(stmt, i++, lang);

            TryBindStatementInt(stmt, i++, nHeight-depth);

            TryBindStatementInt(stmt, i++, minReputation);
            TryBindStatementText(stmt, i++, contentAddress);
            TryBindStatementInt(stmt, i++, limitSubscriptions);

            TryBindStatementInt(stmt, i++, minReputation);
            TryBindStatementText(stmt, i++, contentAddress);
            TryBindStatementInt(stmt, i++, limitSubscriptions);

            TryBindStatementInt(stmt, i++, limitSubscriptionsTotal);

            TryBindStatementInt(stmt, i++, cntRates);

            TryBindStatementInt(stmt, i++, cntOut);

            while (sqlite3_step(*stmt) == SQLITE_ROW)
            {
                if (auto[ok, value] = TryGetColumnInt64(*stmt, 0); ok)
                    ids.push_back(value);
            }

            FinalizeSqlStatement(*stmt);
        });

        return ids;
    }

    vector<int64_t> SearchRepository::GetRandomContentByAddress(const string& contentAddress, const vector<int>& contentTypes, const string& lang, int cntOut)
    {
        auto func = __func__;
        vector<int64_t> ids;

        if (contentAddress.empty())
            return ids;

        string contentTypesFilter = join(vector<string>(contentTypes.size(), "?"), ",");

        string langFilter = "";
        if (!lang.empty())
            langFilter = "cross join Payload lang on lang.TxHash = Contents.Hash and lang.String1 = ?";

        string sql = R"sql(
            select Contents.Id
            from Transactions Contents indexed by Transactions_Type_Last_String1_Height_Id
            )sql" + langFilter + R"sql(
            where Contents.Type in ( )sql" + contentTypesFilter + R"sql( )
              and Contents.Last = 1
              and Contents.String1 = ?
              and Contents.Height > 0
            order by random()
            limit ?
        )sql";

        TryTransactionStep(__func__, [&]()
        {
            auto stmt = SetupSqlStatement(sql);

            int i = 1;

            if (!lang.empty())
                TryBindStatementText(stmt, i++, lang);

            for (const auto& contenttype: contentTypes)
                TryBindStatementInt(stmt, i++, contenttype);

            TryBindStatementText(stmt, i++, contentAddress);

            TryBindStatementInt(stmt, i++, cntOut);

            while (sqlite3_step(*stmt) == SQLITE_ROW)
            {
                if (auto[ok, value] = TryGetColumnInt64(*stmt, 0); ok)
                    ids.push_back(value);
            }

            FinalizeSqlStatement(*stmt);
        });

        return ids;
    }

    vector<int64_t> SearchRepository::GetContentFromAddressSubscriptions(const string& address, const vector<int>& contentTypes, const string& lang, int cntOut, bool rest)
    {
        auto func = __func__;
        vector<int64_t> ids;

        if (address.empty())
            return ids;

        string contentTypesFilter = join(vector<string>(contentTypes.size(), "?"), ",");

        string langFilter = "";
        if (!lang.empty())
            langFilter = "cross join Payload lang on lang.TxHash = Contents.Hash and lang.String1 = ?";

        string limitFilter = "= floor(? / "
                             "(select count(s.String2) from Transactions s indexed by Transactions_Type_Last_String1_Height_Id where s.Type in (302, 303) and s.Last = 1 and s.String1 = ? and s.Height > 0)) ";
        if (rest)
            limitFilter = limitFilter + " + 1 limit ?";
        else
            limitFilter = "<" + limitFilter;

        string sql = R"sql(
            select
                result.Id
            from (
                    select
                        Contents.Id,
                        row_number() over (partition by contents.String1 order by r.Value desc ) as rowNumber
                    from Transactions Contents indexed by Transactions_Type_Last_String1_Height_Id
                    cross join Ratings r on contents.Id = r.Id and r.Last = 1 and r.Type = 2
                    )sql" + langFilter + R"sql(
                    where Contents.Type in ( )sql" + contentTypesFilter + R"sql( )
                      and Contents.Last = 1
                      and Contents.String1 in (
                        select subscribtions.String2
                        from Transactions subscribtions indexed by Transactions_Type_Last_String1_Height_Id
                        where subscribtions.Type in (302, 303)
                            and subscribtions.Last = 1
                            and subscribtions.String1 = ?
                            and subscribtions.Height > 0
                      )
                      and Contents.Height > 0
                )result
            where result.rowNumber
        )sql" + limitFilter;

        TryTransactionStep(__func__, [&]()
        {
            auto stmt = SetupSqlStatement(sql);

            int i = 1;

            if (!lang.empty())
                TryBindStatementText(stmt, i++, lang);

            for (const auto& contenttype: contentTypes)
                TryBindStatementInt(stmt, i++, contenttype);

            TryBindStatementText(stmt, i++, address);

            TryBindStatementInt(stmt, i++, cntOut);

            TryBindStatementText(stmt, i++, address);

            if (rest)
                TryBindStatementInt(stmt, i++, cntOut);

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