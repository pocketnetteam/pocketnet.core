// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include "pocketdb/repositories/web/BarteronRepository.h"

namespace PocketDb
{
    vector<string> BarteronRepository::GetAccountIds(const vector<string>& addresses)
    {
        vector<string> result;

        SqlTransaction(__func__, [&]()
        {
            Sql(R"sql(
                with
                addr as (
                    select
                        RowId as id
                    from
                        Registry
                    where
                        String in ( )sql" + join(vector<string>(addresses.size(), "?"), ",") + R"sql( )
                )
                select
                    (select r.String from Registry r where r.RowId = a.HashId)
                from
                    addr
                cross join
                    Transactions a
                        on a.Type in (104) and a.RegId1 = addr.id
                cross join
                    Last l
                        on l.TxId = a.RowId
            )sql")
            .Bind(addresses)
            .Select([&](Cursor& cursor) {
                while (cursor.Step())
                {
                    if (auto[ok, value] = cursor.TryGetColumnString(0); ok)
                        result.push_back(value);
                }
            });
        });

        return result;
    }
    
    vector<string> BarteronRepository::GetAccountOffersIds(const string& address)
    {
        vector<string> result;

        SqlTransaction(__func__, [&]()
        {
            Sql(R"sql(
                with
                addr as (
                    select
                        RowId as id
                    from
                        Registry
                    where
                        String = ?
                )
                select
                    (select r.String from Registry r where r.RowId = o.HashId)
                from
                    addr
                cross join
                    Transactions o indexed by Transactions_Type_RegId2_RegId1
                        on o.Type in (211) and o.RegId1 = addr.id
                cross join
                    Last l
                        on l.TxId = o.RowId
            )sql")
            .Bind(address)
            .Select([&](Cursor& cursor) {
                while (cursor.Step())
                {
                    if (auto[ok, value] = cursor.TryGetColumnString(0); ok)
                        result.push_back(value);
                }
            });
        });

        return result;
    }

    vector<string> BarteronRepository::GetFeed(const BarteronOffersFeedDto& args)
    {
        vector<string> result;

        string _filters = "";
        if (!args.Language.empty()) _filters += " cross join lang on pt.String1 = lang.value ";
        if (!args.Tags.empty()) _filters += " cross join tags on bo.Tag = tags.value ";
        if (!args.Location.empty()) _filters += " cross join location on pt.String6 like location.value ";
        if (args.PriceMax > 0) _filters += " cross join priceMax on pt.Int1 <= priceMax.value ";
        if (args.PriceMin > 0) _filters += " cross join priceMin on pt.Int1 >= priceMin.value ";
        if (!args.Search.empty()) _filters += " cross join search on pt.String2 like search.value or pt.String3 like search.value ";

        UniValue _tags(UniValue::VARR);
        for (auto t : args.Tags)
            _tags.push_back(t);

        string _orderBy = " ct.Height ";
        if (args.Pagination.OrderBy == "location")
            _orderBy = " pt.String6 ";
        if (args.Pagination.OrderBy == "price")
            _orderBy = " pt.Int1 ";
        if (args.Pagination.OrderDesc)
            _orderBy += " desc ";
        
        SqlTransaction(__func__, [&]()
        {
            Sql(R"sql(
                with
                lang as (select ? as value),
                tags as (select value from json_each(?)),
                location as (select ? as value),
                priceMax as (select ? as value),
                priceMin as (select ? as value),
                search as (select ? as value)

                select
                    (select r.String from Registry r where r.RowId = t.HashId)
                from
                    Transactions t indexed by Transactions_Type_RegId1_RegId2_RegId3
                cross join
                    Last lt
                        on lt.TxId = t.RowId
                cross join
                    Chain ct indexed by Chain_TxId_Height
                        on ct.TxId = t.RowId and ct.Height <= ?
                cross join
                    Payload pt
                        on pt.TxId = t.RowId
                -- Account
                cross join
                    Transactions u indexed by Transactions_Type_RegId1_RegId2_RegId3
                        on u.Type in (104) and u.RegId1 = t.RegId1
                cross join
                    Last lu
                        on lu.TxId = u.RowId
                cross join
                    Chain cu
                        on cu.TxId = u.RowId
                -- Tags
                cross join
                    web.BarteronOffers bo
                        on bo.AccountId = cu.Uid and bo.OfferId = ct.Uid
                -- Filters
                )sql" + _filters + R"sql(
                where
                    t.Type in (211)
                order by
                    )sql" + _orderBy + R"sql(
                limit ? offset ?
            )sql")
            .Bind(
                args.Language,
                _tags.write(),
                args.Location + "%",
                args.PriceMax,
                args.PriceMin,
                "%" + args.Search + "%",
                args.Pagination.TopHeight,
                args.Pagination.PageSize,
                args.Pagination.PageStart * args.Pagination.PageSize
            )
            .Select([&](Cursor& cursor) {
                while (cursor.Step())
                {
                    if (auto[ok, value] = cursor.TryGetColumnString(0); ok)
                        result.push_back(value);
                }
            });
        });

        return result;
    }

}