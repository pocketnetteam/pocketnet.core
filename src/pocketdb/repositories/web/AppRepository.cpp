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

    map<string, BarteronAccountAdditionalInfo> BarteronRepository::GetAccountsAdditionalInfo(const vector<string>& txids)
    {
        map<string, BarteronAccountAdditionalInfo> result;

        SqlTransaction(__func__, [&]()
        {
            Sql(R"sql(
                with
                data as (
                    select
                        t.Hash as txid,
                        c.Uid as uid,
                        t.RegId1 as addrid
                    from
                        vTx t
                        cross join Chain c on
                            c.TxId = t.RowId
                    where
                        t.Hash in ( )sql" + join(vector<string>(txids.size(), "?"), ",") + R"sql( )
                ),
                regdate as (
                    select
                        t.Time as val
                    from
                        data
                        cross join Chain c indexed by Chain_Uid_Height on
                            c.Uid = data.uid
                        cross join First f on
                            f.TxId = c.TxId
                        cross join Transactions t on
                            t.RowId = c.TxId
                ),
                rating as (
                    select
                        cast (ifnull(avg(s.Int1), 0) * 10 as integer) as val
                    from
                        data,
                        Transactions o indexed by Transactions_Type_RegId1_RegId2_RegId3
                        cross join Transactions s indexed by Transactions_Type_RegId2_RegId1 on
                            s.Type = 300 and
                            s.RegId2 = o.RegId2 and
                            s.RegId1 != addrid
                        cross join Chain c on -- chain only
                            c.TxId = s.RowId
                    where
                        o.Type = 211 and
                        o.RegId1 = addrid
                )
                select
                    data.txid,
                    regdate.val,
                    rating.val
                from
                    data,
                    regdate,
                    rating
            )sql")
            .Bind(txids)
            .Select([&](Cursor& cursor) {
                while (cursor.Step()) {
                    string txid;
                    int64_t regdate;
                    int rating;
                    if (cursor.CollectAll(txid, regdate, rating)) {
                        result.insert({std::move(txid), {regdate, rating}});
                    }
                }
            });
        });

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
        if (args.PriceMax > 0) _filters += " cross join priceMax on pt.Int1 <= priceMax.value ";
        if (args.PriceMin > 0) _filters += " cross join priceMin on pt.Int1 >= priceMin.value ";
        if (!args.Search.empty()) _filters += " cross join search on pt.String2 like search.value or pt.String3 like search.value ";

        string _tagsStr = "[]";
        string _locationStr = "[]";

        if (!args.Tags.empty()) {
            UniValue _tags(UniValue::VARR);
            for (auto t : args.Tags)
                _tags.push_back(t);

            _tagsStr = _tags.write();
            _filters += " cross join tags on bo.Tag = tags.value ";
        }

        if (!args.Location.empty()) {
            UniValue _location(UniValue::VARR);
            for (auto t : args.Location)
                _location.push_back(t + "%");

            _locationStr = _location.write();
            _filters += " cross join location on pt.String6 like location.value ";
        }

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
                        location as (select value from json_each(?)),
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
                    left join
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
                    _tagsStr,
                    _locationStr,
                    args.PriceMax,
                    args.PriceMin,
                    args.Search,
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

        string _orderBy = " co2.Height ";
        if (args.Page.OrderBy == "location")
            _orderBy = " po2.String6 ";
        if (args.Page.OrderBy == "price")
            _orderBy = " po2.Int1 ";
        if (args.Page.OrderDesc)
            _orderBy += " desc ";

        string _filters = "";

        string _locationStr = "[]";
        if (!args.Location.empty()) {
            UniValue _location(UniValue::VARR);
            for (auto t : args.Location)
                _location.push_back(t + "%");

            _locationStr = _location.write();
            _filters += " cross join location on po2.String6 like location.value ";
        }
        
        SqlTransaction(
            __func__,
            [&]() -> Stmt& {
                return Sql(R"sql(
                    with
                        price as (
                            select
                                ? as min,
                                ? as max
                        ),
                        location as (select value from json_each(?)),
                        search as (
                            select ? as value
                        )
                    select distinct
                        (select r.String from Registry r where r.RowId = to2.RowId)
                    from
                        price,
                        search

                    -- Source offer
                    -- cross join
                    --     BarteronOffers o1 indexed by BarteronOffers_OfferId_Tag_AccountId
                    --         on o1.OfferId = offer.value
                    -- cross join
                    --     BarteronOfferTags t1 -- autoindex OfferId_Tag
                    --         on t1.OfferId = o1.OfferId

                    -- Offer potencial for deal
                    cross join
                        BarteronOffers o2 -- autoindex Tag_OfferId_AccountId
                            on (? or o2.Tag in ( )sql" + join(vector<string>(args.TheirTags.size(), "?"), ",") + R"sql( )) -- TODO (losty): shouldn't belong to user that asks for deals -- and o2.OfferId != t1.OfferId and o2.AccountId != o1.AccountId
                    cross join
                        BarteronOfferTags t2 on -- autoindex OfferId_Tag
                            t2.OfferId = o2.OfferId
                            and (? or t2.Tag in ( )sql" + join(vector<string>(args.MyTags.size(), "?"), ",") + R"sql( ))

                    -- Offer potencial account
                    cross join Chain cu2 indexed by Chain_Uid_Height on cu2.Uid = o2.AccountId
                    cross join Transactions u2 on u2.RowId = cu2.TxId
                    cross join Last lu2 on lu2.TxId = u2.RowId
                    cross join Registry ru2 on ru2.RowId = u2.RegId1

                    -- Filter found deals by another conditions
                    -- cross join Chain c1 on c1.Uid = o1.OfferId
                    -- cross join Last l1 on l1.TxId = c1.TxId
                    -- cross join Payload p1 on p1.TxId = l1.TxId
                    cross join Chain co2 on co2.Uid = o2.OfferId
                    cross join Transactions to2 on to2.RowId = co2.TxId
                    cross join Last lo2 on lo2.TxId = co2.TxId
                    cross join Payload po2 on po2.TxId = lo2.TxId

                    -- Filters
                    )sql" + _filters + R"sql(

                    where
                        ( ? or po2.Int1 >= price.min ) and
                        ( ? or po2.Int1 <= price.max ) and
                        ( ? or po2.String2 like search.value or po2.String3 like search.value) and
                        ( ? or ru2.String in ( )sql" + join(vector<string>(args.Addresses.size(), "?"), ",") + R"sql( ) ) and
                        ( ? or ru2.String not in ( )sql" + join(vector<string>(args.ExcludeAddresses.size(), "?"), ",") + R"sql( ) ) and
                        cu2.Height <= ?

                    order by
                        )sql" + _orderBy + R"sql(
                    limit ? offset ?
                )sql")
                .Bind(
                    args.PriceMin,
                    args.PriceMax,
                    _locationStr,
                    args.Search,
                    args.TheirTags.empty(),
                    args.TheirTags,
                    args.MyTags.empty(),
                    args.MyTags,
                    (args.PriceMin < 0),
                    (args.PriceMax < 0),
                    args.Search.empty(),
                    args.Addresses.empty(),
                    args.Addresses,
                    args.ExcludeAddresses.empty(),
                    args.ExcludeAddresses,
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

    map<string, vector<string>> BarteronRepository::GetComplexDeal(const BarteronOffersComplexDealDto& args)
    {
        map<string, vector<string>> result;

        string _filters = "";

        string _locationStr = "[]";
        if (!args.Location.empty()) {
            UniValue _location(UniValue::VARR);
            for (auto t : args.Location)
                _location.push_back(t + "%");

            _locationStr = _location.write();
            _filters += " cross join location on ( p1.String6 like location.value or p2.String6 like location.value )";
        }
        
        SqlTransaction(
            __func__,
            [&]() -> Stmt& {
                return Sql(
                    R"sql(
                        with
                            location as (select value from json_each(?)),
                            mytag as (
                                select ? as value
                            )
                        select
                            (select r.String from Registry r where r.RowId = tx1.RowId),
                            (select r.String from Registry r where r.RowId = tx2.RowId)
                        from
                            mytag
                            cross join BarteronOffers o1 on
                                o1.Tag in ( )sql" + join(vector<string>(args.TheirTags.size(), "?"), ",") + R"sql( ) and
                                not exists (select 1 from BarteronOfferTags tt where tt.OfferId = o1.OfferId and tt.Tag = mytag.value)
                            cross join BarteronOfferTags t1 on
                                t1.OfferId = o1.OfferId

                            cross join BarteronOffers o2 on
                                o2.Tag = t1.Tag
                            cross join BarteronOfferTags t2 on
                                t2.OfferId = o2.OfferId and
                                t2.Tag = mytag.value

                            cross join Chain c1 on
                                c1.Uid = o1.OfferId
                            cross join Last l1 on
                                l1.TxId = c1.TxId
                            cross join Transactions tx1 on
                                tx1.RowId = c1.TxId and
                                (? or tx1.RegId1 not in (select r.RowId from Registry r where r.String in ( )sql" + join(vector<string>(args.ExcludeAddresses.size(), "?"), ",") + R"sql( )))
                            cross join Payload p1 on
                                p1.TxId = c1.TxId

                            cross join Chain c2 on
                                c2.Uid = o2.OfferId
                            cross join Last l2 on
                                l2.TxId = c2.TxId
                            cross join Transactions tx2 on
                                tx2.RowId = c2.TxId and
                                tx2.RegId1 != tx1.RegId1 and
                                (? or tx2.RegId1 not in (select r.RowId from Registry r where r.String in ( )sql" + join(vector<string>(args.ExcludeAddresses.size(), "?"), ",") + R"sql( )))
                            cross join Payload p2 on
                                p2.TxId = c2.TxId

                            -- Filters
                            )sql" + _filters + R"sql(
                    )sql"
                )
                .Bind(
                    _locationStr,
                    args.MyTag,
                    args.TheirTags,
                    args.ExcludeAddresses.empty(),
                    args.ExcludeAddresses,
                    args.ExcludeAddresses.empty(),
                    args.ExcludeAddresses
                );
            },
            [&] (Stmt& stmt) {
                stmt.Select([&](Cursor& cursor) {
                    while (cursor.Step()) {
                        string target, intermediate;
                        if (cursor.CollectAll(target, intermediate)) {
                            result[target].emplace_back(intermediate);
                        }
                    }
                });
            }
        );

        return result;
    }
}