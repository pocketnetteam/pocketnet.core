// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include "pocketdb/repositories/web/AppRepository.h"

namespace PocketDb
{

    vector<string> AppRepository::List(const AppListDto& args)
    {
        vector<string> result;

        string _keyword = "\"" + args.Search + "\"" + " OR " + args.Search + "*";

        string _orderBy = " ct.Height ";
        // TODO app : add sorting by rating and comment count
        if (args.Page.OrderDesc)
            _orderBy += " desc ";
        
        SqlTransaction(
            __func__,
            [&]() -> Stmt& {
                return Sql(R"sql(
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
                    where
                        t.Type in (221) and
                        (? or t.RowId in (
                            select
                                tm.ContentId
                            from
                                web.Tags tag indexed by Tags_Lang_Value_Id
                            join
                                web.TagsMap tm indexed by TagsMap_TagId_ContentId on
                                    tm.TagId = tag.Id
                            where
                                tag.Value in ( )sql" + join(vector<string>(args.Tags.size(), "?"), ",") + R"sql( ) and
                                tag.Lang = 'en'
                        )) and
                        (? or ct.Uid in (
                            select
                                cm.ContentId
                            from
                                web.Content c,
                                web.ContentMap cm
                            where
                                c.ROWID = cm.ROWID and
                                cm.FieldType in (3,5) and
                                c.Value match ?
                        ))
                    order by
                        )sql" + _orderBy + R"sql(
                    limit ? offset ?
                )sql")
                .Bind(
                    args.Page.TopHeight,
                    args.Tags.empty(),
                    args.Tags,
                    args.Search.empty(),
                    _keyword,
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

    map<string, UniValue> AppRepository::AdditionalInfo(const vector<string>& txs)
    {
        map<string, UniValue> result;

        SqlTransaction(
            __func__,
            [&]() -> Stmt& {
                return Sql(R"sql(
                    with
                        app as (
                            select
                                r.RowId,
                                r.String as Hash
                            from
                                Registry r
                            where
                                r.String in ( )sql" + join(vector<string>(txs.size(), "?"), ",") + R"sql( )
                        )
                    select
                        app.Hash,
                        r.Value
                    from
                        app
                    cross join
                        Transactions t on
                            t.RowId = app.RowId
                    cross join
                        Chain c on
                            c.TxId = t.RowId
                    cross join
                        Ratings r on
                            r.Type = 2 and
                            r.Last = 1 and
                            r.Uid = c.Uid
                )sql")
                .Bind(
                    txs
                );
            },
            [&] (Stmt& stmt)
            {
                stmt.Select([&](Cursor& cursor)
                {
                    while (cursor.Step())
                    {
                        string txHash;
                        int rating;
                        cursor.CollectAll(txHash, rating);
                        
                        UniValue txData(UniValue::VOBJ);
                        txData.pushKV("r", rating);
                        
                        result.emplace(txHash, txData);
                    }
                });
            }
        );

        return result;
    }

    vector<string> AppRepository::Scores(const string& tx, const Pagination& pg)
    {
        vector<string> result;

        SqlTransaction(
            __func__,
            [&]() -> Stmt& {
                return Sql(R"sql(
                    with
                        apps as (
                            select
                                r.RowId
                            from
                                Registry r
                            where
                                r.String in (?)
                        )
                    select
                        (select r.String from Registry r where r.RowId = t.RowId)
                    from
                        apps
                    cross join
                        Transactions t indexed by Transactions_Type_RegId2_RegId1 on
                            t.Type in (300) and
                            t.RegId2 = apps.RowId
                    cross join
                        Chain c on
                            c.TxId = t.RowId and
                            c.Height <= ?
                    order by
                        c.Height desc, c.BlockNum desc
                    limit ? offset ?
                )sql")
                .Bind(
                    tx,
                    pg.TopHeight,
                    pg.PageSize,
                    pg.PageStart * pg.PageSize
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

    vector<string> AppRepository::Comments(const string& tx, const Pagination& pg)
    {
        vector<string> result;

        SqlTransaction(
            __func__,
            [&]() -> Stmt& {
                return Sql(R"sql(
                    with
                        apps as (
                            select
                                r.RowId
                            from
                                Registry r
                            where
                                r.String in (?)
                        )
                    select
                        (select r.String from Registry r where r.RowId = t.RowId)
                    from
                        apps
                    cross join
                        Transactions t indexed by Transactions_Type_RegId3_RegId1 on
                            t.Type in (204,205,206) and
                            t.RegId3 = apps.RowId
                    cross join
                        Last l on
                            l.TxId = t.RowId
                    cross join
                        Chain c on
                            c.TxId = t.RowId and
                            c.Height <= ?
                    order by
                        c.Height desc, c.BlockNum desc
                    limit ? offset ?
                )sql")
                .Bind(
                    tx,
                    pg.TopHeight,
                    pg.PageSize,
                    pg.PageStart * pg.PageSize
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