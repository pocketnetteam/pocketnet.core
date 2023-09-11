// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include "pocketdb/repositories/web/BarteronRepository.h"

namespace PocketDb
{
    vector<string> BarteronRepository::GetAccountIds(const vector<string>& addresses)
    {
        vector<string> result;

        SqlTransaction(
            __func__,
            [&]() -> Stmt& {
                return Sql(R"sql(
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
                        (select r.String from Registry r where r.RowId = a.RowId)
                    from
                        addr
                    cross join
                        Transactions a
                            on a.Type in (104) and a.RegId1 = addr.id
                    cross join
                        Last l
                            on l.TxId = a.RowId
                )sql")
                .Bind(addresses);
            },
            [&] (Stmt& stmt) {
                stmt.Select([&](Cursor& cursor) {
                    while (cursor.Step())
                    {
                        if (auto[ok, value] = cursor.TryGetColumnString(0); ok)
                            result.push_back(value);
                    }
                });
            }
        );

        return result;
    }
    
    vector<string> BarteronRepository::GetAccountOffersIds(const string& address)
    {
        vector<string> result;

        SqlTransaction(
            __func__,
            [&]() -> Stmt& {
                return Sql(R"sql(
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
                        (select r.String from Registry r where r.RowId = o.RowId)
                    from
                        addr
                    cross join
                        Transactions o indexed by Transactions_Type_RegId2_RegId1
                            on o.Type in (211) and o.RegId1 = addr.id
                    cross join
                        Last l
                            on l.TxId = o.RowId
                )sql")
                .Bind(address);
            },
            [&] (Stmt& stmt) {
                stmt.Select([&](Cursor& cursor) {
                    while (cursor.Step())
                    {
                        if (auto[ok, value] = cursor.TryGetColumnString(0); ok)
                            result.push_back(value);
                    }
                });
            }
        );

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
        if (args.Page.OrderBy == "location")
            _orderBy = " pt.String6 ";
        if (args.Page.OrderBy == "price")
            _orderBy = " pt.Int1 ";
        if (args.Page.OrderDesc)
            _orderBy += " desc ";
        
        SqlTransaction(
            __func__,
            [&]() -> Stmt& {
                return Sql(R"sql(
                    with
                    lang as (select ? as value),
                    tags as (select value from json_each(?)),
                    location as (select ? as value),
                    priceMax as (select ? as value),
                    priceMin as (select ? as value),
                    search as (select ? as value)

                    select
                        (select r.String from Registry r where r.RowId = t.RowId)
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
                    args.Page.TopHeight,
                    args.Page.PageSize,
                    args.Page.PageStart * args.Page.PageSize
                );
            },
            [&] (Stmt& stmt) {
                stmt.Select([&](Cursor& cursor) {
                    while (cursor.Step())
                    {
                        if (auto[ok, value] = cursor.TryGetColumnString(0); ok)
                            result.push_back(value);
                    }
                });
            }
        );

        return result;
    }

    vector<string> BarteronRepository::GetDeals(const BarteronOffersDealDto& args)
    {
        vector<string> result;

        // string _orderBy = " ct.Height ";
        // if (args.Page.OrderBy == "location")
        //     _orderBy = " pt.String6 ";
        // if (args.Page.OrderBy == "price")
        //     _orderBy = " pt.Int1 ";
        // if (args.Page.OrderDesc)
        //     _orderBy += " desc ";
        
        SqlTransaction(
            __func__,
            [&]() -> Stmt& {
                return Sql(R"sql(
                    with
                    offer as (
                        select
                            c.Uid as value
                        from
                            Registry r
                        cross join
                            Transactions t on t.RowId = r.RowId
                        cross join
                            Chain c on c.TxId = t.RowId
                        where r.String = ?
                    ),
                    addr as (
                        select ? as value
                    ),
                    price as (
                        select ? as value
                    ),
                    loc as (
                        select ? as value
                    )
                    select
                        (select r.String from Registry r where r.RowId = to2.RowId)
                    from
                        offer,
                        addr,
                        price,
                        loc

                    -- Source offer
                    cross join
                        BarteronOffers o1 indexed by BarteronOffers_OfferId_Tag_AccountId
                            on o1.OfferId = offer.value
                    cross join
                        BarteronOfferTags t1 -- autoindex OfferId_Tag
                            on t1.OfferId = o1.OfferId

                    -- Offer potencial for deal
                    cross join
                        BarteronOffers o2 -- autoindex Tag_OfferId_AccountId
                            on o2.Tag = t1.Tag and o2.OfferId != t1.OfferId and o2.AccountId != o1.AccountId
                    cross join
                        BarteronOfferTags t2 -- autoindex OfferId_Tag
                            on t2.OfferId = o2.OfferId and t2.Tag = o1.Tag

                    -- Offer potencial account
                    cross join Chain cu2 indexed by Chain_Uid_Height on cu2.Uid = o2.AccountId
                    cross join Transactions u2 on u2.RowId = cu2.TxId
                    cross join Last lu2 on lu2.TxId = u2.RowId
                    cross join Registry ru2 on ru2.RowId = u2.RegId1

                    -- Filter found deals by another conditions
                    cross join Chain c1 on c1.Uid = o1.OfferId
                    cross join Last l1 on l1.TxId = c1.TxId
                    cross join Payload p1 on p1.TxId = l1.TxId
                    cross join Chain co2 on co2.Uid = o2.OfferId
                    cross join Transactions to2 on to2.RowId = co2.TxId
                    cross join Last lo2 on lo2.TxId = co2.TxId
                    cross join Payload po2 on po2.TxId = lo2.TxId

                    where
                        ( ? or abs(po2.Int1 - p1.Int1) < price.value ) and
                        ( ? or substr(po2.String6, 1, loc.value) like substr(p1.String6, 1, loc.value) ) and
                        ( ? or ru2.String = addr.value )

                    limit 5 -- todo add pagination ?
                )sql")
                .Bind(
                    args.Offer,
                    args.Address,
                    args.Price,
                    args.Location,
                    (args.Price < 0),
                    (args.Location < 0),
                    (args.Address.empty())
                );
            },
            [&] (Stmt& stmt) {
                stmt.Select([&](Cursor& cursor) {
                    while (cursor.Step())
                    {
                        if (auto[ok, value] = cursor.TryGetColumnString(0); ok)
                            result.push_back(value);
                    }
                });
            }
        );

        return result;
    }

}