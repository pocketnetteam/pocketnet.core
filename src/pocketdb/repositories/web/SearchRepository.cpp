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

    vector<int64_t> SearchRepository::SearchUsers(const SearchRequest& request)
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

    UniValue SearchRepository::GetRecomendedAccountsBySubscriptions(const string& address, int cntOut)
    {
        auto func = __func__;
        UniValue result(UniValue::VARR);

        if (address.empty())
            return result;

        string sql = R"sql(
            select
                recommendation.address,
                p.String2 as name,
                p.String3 as avatar

                , ifnull((
                    select r.Value
                    from Ratings r indexed by Ratings_Type_Id_Last_Height
                    where r.Type=0 and r.Id=u.Id and r.Last=1)
                ,0) as Reputation

                , (
                    select count(*)
                    from Transactions subs indexed by Transactions_Type_Last_String2_Height
                    where subs.Type in (302,303) and subs.Height is not null and subs.Last = 1 and subs.String2 = u.String1
                ) as SubscribersCount
            from (
                select
                    t.String2 address
                from Transactions t indexed by Transactions_Type_Last_String1_String2_Height
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
                group by t.String2
                order by count(*) desc
                limit ?
            )recommendation
            cross join Transactions u indexed by Transactions_Type_Last_String1_Height_Id on u.String1 = recommendation.address
                and u.Type in (100,101,102)
                and u.Last=1
                and u.Height is not null
            cross join Payload p on p.TxHash = u.Hash

        )sql";

        TryTransactionStep(__func__, [&]()
        {
            auto stmt = SetupSqlStatement(sql);

            int i = 1;
            TryBindStatementText(stmt, i++, address);
            TryBindStatementText(stmt, i++, address);
            TryBindStatementInt(stmt, i++, cntOut);

            while (sqlite3_step(*stmt) == SQLITE_ROW)
            {
                UniValue record(UniValue::VOBJ);
                if (auto[ok, value] = TryGetColumnString(*stmt, 0); ok) record.pushKV("address", value);
                if (auto[ok, value] = TryGetColumnString(*stmt, 1); ok) record.pushKV("name", value);
                if (auto[ok, value] = TryGetColumnString(*stmt, 2); ok) record.pushKV("avatar", value);
                if (auto[ok, value] = TryGetColumnInt(*stmt, 3); ok) record.pushKV("reputation", value / 10.0);
                if (auto[ok, value] = TryGetColumnInt(*stmt, 4); ok) record.pushKV("subscribers_count", value);
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
            select recommendation.address,
                   p.String2              as name,
                   p.String3              as avatar

                , ifnull((
                    select r.Value
                    from Ratings r indexed by Ratings_Type_Id_Last_Height
                    where r.Type=0 and r.Id=u.Id and r.Last=1)
                ,0) as Reputation

                , (
                    select count(*)
                    from Transactions subs indexed by Transactions_Type_Last_String2_Height
                    where subs.Type in (302,303) and subs.Height is not null and subs.Last = 1 and subs.String2 = u.String1
                ) as SubscribersCount
            from (
                select
                    tOtherContents.String1 as address
                from Transactions tOtherContents
                         indexed by Transactions_Type_Last_String1_String2_Height
                where tOtherContents.String2 in (
                    select tOtherLikes.String2 as OtherLikedContent
                    from Transactions tOtherlikes
                    where tOtherLikes.String1 in (
                        select tLikes.String1 as Liker
                        from Transactions tLikes
                        where tLikes.String2 in (
                            select tContents.String2 as BloggerContent
                            from Transactions tContents
                            where tContents.Type in ( )sql" + contentTypesFilter + R"sql( )
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
            )recommendation
             cross join Transactions u indexed by Transactions_Type_Last_String1_Height_Id
                      on u.String1 = recommendation.address
                      and u.Type in (100,101,102)
                      and u.Last=1
                      and u.Height is not null
             cross join Payload p on p.TxHash = u.Hash
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

            while (sqlite3_step(*stmt) == SQLITE_ROW)
            {
                UniValue record(UniValue::VOBJ);
                if (auto[ok, value] = TryGetColumnString(*stmt, 0); ok) record.pushKV("address", value);
                if (auto[ok, value] = TryGetColumnString(*stmt, 1); ok) record.pushKV("name", value);
                if (auto[ok, value] = TryGetColumnString(*stmt, 2); ok) record.pushKV("avatar", value);
                if (auto[ok, value] = TryGetColumnInt(*stmt, 3); ok) record.pushKV("reputation", value / 10.0);
                if (auto[ok, value] = TryGetColumnInt(*stmt, 4); ok) record.pushKV("subscribers_count", value);
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
            select recommendation.address,
                   p.String2              as name,
                   p.String3              as avatar

                , ifnull((
                    select r.Value
                    from Ratings r indexed by Ratings_Type_Id_Last_Height
                    where r.Type=0 and r.Id=u.Id and r.Last=1)
                ,0) as Reputation

                , (
                    select count(*)
                    from Transactions subs indexed by Transactions_Type_Last_String2_Height
                    where subs.Type in (302,303) and subs.Height is not null and subs.Last = 1 and subs.String2 = u.String1
                ) as SubscribersCount
            from (
                select
                        tOtherContents.String1 as address
                from Transactions tOtherContents
                         indexed by Transactions_Type_Last_String2_Height
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
                limit ?
            )recommendation
            cross join Transactions u on u.String1 = recommendation.address
                    and u.Type in (100,101,102)
                    and u.Last=1
                    and u.Height is not null
            cross join Payload p on p.TxHash = u.Hash
        )sql";

        TryTransactionStep(__func__, [&]()
        {
            auto stmt = SetupSqlStatement(sql);

            int i = 1;
            TryBindStatementText(stmt, i++, address);
            TryBindStatementInt(stmt, i++, nHeight - depth);
            TryBindStatementInt(stmt, i++, nHeight - depth);
            TryBindStatementInt(stmt, i++, nHeight - depth);
            for (const auto& contenttype: contentTypes)
                TryBindStatementInt(stmt, i++, contenttype);
            TryBindStatementText(stmt, i++, address);
            TryBindStatementInt(stmt, i++, nHeight - depth);
            TryBindStatementInt(stmt, i++, cntOut);

            while (sqlite3_step(*stmt) == SQLITE_ROW)
            {
                UniValue record(UniValue::VOBJ);
                if (auto[ok, value] = TryGetColumnString(*stmt, 0); ok) record.pushKV("address", value);
                if (auto[ok, value] = TryGetColumnString(*stmt, 1); ok) record.pushKV("name", value);
                if (auto[ok, value] = TryGetColumnString(*stmt, 2); ok) record.pushKV("avatar", value);
                if (auto[ok, value] = TryGetColumnInt(*stmt, 3); ok) record.pushKV("reputation", value / 10.0);
                if (auto[ok, value] = TryGetColumnInt(*stmt, 4); ok) record.pushKV("subscribers_count", value);
                result.push_back(record);
            }

            FinalizeSqlStatement(*stmt);
        });

        return result;
    }

    UniValue SearchRepository::GetRecomendedContentsByScoresOnSimilarContents(const string& contentid, const vector<int>& contentTypes, int depth, int cntOut)
    {
        auto func = __func__;
        UniValue result(UniValue::VARR);

        if (contentid.empty())
            return result;

        string contentTypesFilter = join(vector<string>(contentTypes.size(), "?"), ",");

        string sql = R"sql(
            select OtherRaters.String2 OtherScoredContent, count(*) cnt
            from Transactions OtherRaters indexed by Transactions_Type_Last_String1_Height_Id
            cross join Transactions Contents indexed by Transactions_Type_Last_String2_Height
                on OtherRaters.String2 = Contents.String2 and Contents.Last = 1 and Contents.Type in ( )sql" + contentTypesFilter + R"sql( ) and Contents.Height > 0
            where OtherRaters.Type in (300)
              and OtherRaters.Int1 > 3
              and OtherRaters.Last in (1, 0)
              and OtherRaters.Height >= (select Height
                                         from Transactions indexed by Transactions_Type_Last_String2_Height
                                         where Type in ( )sql" + contentTypesFilter + R"sql( )
                                           and String2 = ?
                                           and Last = 1) - ?
              and OtherRaters.String1 in (
                select String1 as Rater
                from Transactions Raters indexed by Transactions_Type_Last_String2_Height
                where Raters.Type in (300)
                  and Raters.Int1 > 3
                  and Raters.String2 = ?
                  and Raters.Last in (1, 0)
            )
              and OtherRaters.String2 != ?
            group by OtherRaters.String2
            order by count(*) desc
            limit ?
        )sql";

        TryTransactionStep(__func__, [&]()
        {
            auto stmt = SetupSqlStatement(sql);

            int i = 1;
            for (const auto& contenttype: contentTypes)
                TryBindStatementInt(stmt, i++, contenttype);
            for (const auto& contenttype: contentTypes)
                TryBindStatementInt(stmt, i++, contenttype);
            TryBindStatementText(stmt, i++, contentid);
            TryBindStatementInt(stmt, i++, depth);
            TryBindStatementText(stmt, i++, contentid);
            TryBindStatementText(stmt, i++, contentid);
            TryBindStatementInt(stmt, i++, cntOut);

            while (sqlite3_step(*stmt) == SQLITE_ROW)
            {
                UniValue record(UniValue::VOBJ);
                if (auto[ok, value] = TryGetColumnString(*stmt, 0); ok) record.pushKV("contentid", value);
                result.push_back(record);
            }

            FinalizeSqlStatement(*stmt);
        });

        return result;
    }

    UniValue SearchRepository::GetRecomendedContentsByScoresFromAddress(const string& address, const vector<int>& contentTypes, int nHeight, int depth, int cntOut)
    {
        auto func = __func__;
        UniValue result(UniValue::VARR);

        if (address.empty())
            return result;

        string contentTypesFilter = join(vector<string>(contentTypes.size(), "?"), ",");

        string sql = R"sql(
            select OtherRaters.String2 as OtherScoredContent, count(*) cnt
            from Transactions OtherRaters indexed by Transactions_Type_Last_String1_Height_Id
            cross join Transactions Contents indexed by Transactions_Type_Last_String2_Height
                on OtherRaters.String2 = Contents.String2 and Contents.Last = 1 and Contents.Type in ( )sql" + contentTypesFilter + R"sql( ) and Contents.Height > 0
            where OtherRaters.String1 in (
                select Scores.String1 as Rater
                from Transactions Scores indexed by Transactions_Type_Last_String2_Height
                where Scores.String2 in (
                    select addressScores.String2 as ContentsScoredByAddress
                    from Transactions addressScores indexed by Transactions_Type_Last_String1_Height_Id
                    where addressScores.String1 = ?
                      and addressScores.Type in (300)
                      and addressScores.Last in (1, 0)
                      and addressScores.Int1 > 3
                      and addressScores.Height >= ?
                )
                  and Scores.Type in (300)
                  and Scores.Last in (1, 0)
                  and Scores.Int1 > 3
                  and Scores.Height >= ?
            )
              and OtherRaters.Type in (300)
              and OtherRaters.Last in (1, 0)
              and OtherRaters.Int1 > 3
              and OtherRaters.Height >= ?
            group by OtherRaters.String2
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
            TryBindStatementInt(stmt, i++, cntOut);

            while (sqlite3_step(*stmt) == SQLITE_ROW)
            {
                UniValue record(UniValue::VOBJ);
                if (auto[ok, value] = TryGetColumnString(*stmt, 0); ok) record.pushKV("contentid", value);
                result.push_back(record);
            }

            FinalizeSqlStatement(*stmt);
        });

        return result;
    }
}