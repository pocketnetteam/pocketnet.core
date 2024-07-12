// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include "pocketdb/repositories/web/ExplorerRepository.h"

namespace PocketDb
{
    map<int, map<int, int>> ExplorerRepository::GetBlocksStatistic(int bottomHeight, int topHeight)
    {
        map<int, map<int, int>> result;

        SqlTransaction(
            __func__,
            [&]() -> Stmt& {
                return Sql(R"sql(
                    select
                        c.Height,
                        t.Type,
                        count(1)
                    from
                        Chain c indexed by Chain_Height_Uid
                        left join Transactions t on
                            t.RowId = c.TxId
                    where
                        c.Height > ? and
                        c.Height <= ?
                    group by
                        c.Height, t.Type
                )sql")
                .Bind(bottomHeight, topHeight);
            },
            [&] (Stmt& stmt) {
                stmt.Select([&](Cursor& cursor) {
                    while (cursor.Step())
                    {
                        int sHeight, sType, sCount;
                        if (cursor.CollectAll(sHeight, sType, sCount)) 
                            result[sHeight][sType] = sCount;
                    }
                });
            }
        );

        return result;
    }

    UniValue ExplorerRepository::GetTransactionsStatistic(int64_t top, int depth, int period)
    {
        UniValue result(UniValue::VOBJ);

        SqlTransaction(
            __func__,
            [&]() -> Stmt& {
                return Sql(R"sql(
                    select
                        (t.Time / ?),
                        t.Type,
                        count()
                    from
                        Transactions t
                    where
                        t.Type in (1,100,103,104,200,201,202,204,205,208,209,210,211,220,300,301,302,303) and
                        t.Time >= ? and
                        t.time < ?
                    group by
                        t.time / ?, t.Type
                )sql")
                .Bind(period, top - (depth * period), top, period);
            },
            [&] (Stmt& stmt) {
                stmt.Select([&](Cursor& cursor) {
                    while (cursor.Step())
                    {
                        int part, type, count;
                        if (cursor.CollectAll(part, type, count))
                        {
                            if (result.At(to_string(part)).isNull())
                                result.pushKV(to_string(part), UniValue(UniValue::VOBJ));

                            result.At(to_string(part)).pushKV(to_string(type), count);
                        }
                    }
                });
            }
        );

        return result;
    }

    UniValue ExplorerRepository::GetTransactionsStatisticByHours(int topHeight, int depth)
    {
        UniValue result(UniValue::VOBJ);

        SqlTransaction(
            __func__,
            [&]() -> Stmt& {
                return Sql(R"sql(
                    select
                        (c.Height / 60)Hour,
                        t.Type,
                        count()Count
                    from
                        Chain c indexed by Chain_HeightByHour
                        cross join Transactions t on
                            t.RowId = c.TxId and
                            t.Type in (1,100,103,104,200,201,202,204,205,208,209,210,211,220,300,301,302,303)
                    where
                    (c.Height / 60) < (? / 60) and
                    (c.Height / 60) >= (? / 60)
                    group by
                        (c.Height / 60), t.Type
                )sql")
                .Bind(topHeight, topHeight - depth);
            },
            [&] (Stmt& stmt) {
                stmt.Select([&](Cursor& cursor) {
                    while (cursor.Step())
                    {
                        int part, type, count;
                        if (cursor.CollectAll(part, type, count))
                        {
                            if (result.At(to_string(part)).isNull())
                                result.pushKV(to_string(part), UniValue(UniValue::VOBJ));

                            result.At(to_string(part)).pushKV(to_string(type), count);
                        }
                    }
                });
            }
        );

        return result;
    }

    UniValue ExplorerRepository::GetTransactionsStatisticByDays(int topHeight, int depth)
    {
        UniValue result(UniValue::VOBJ);

        SqlTransaction(
            __func__,
            [&]() -> Stmt& {
                return Sql(R"sql(
                    select
                        (c.Height / 1440)Day,
                        t.Type,
                        count()Count
                    from
                        Chain c indexed by Chain_HeightByDay
                        join Transactions t on
                            t.RowId = c.TxId and
                            t.Type in (1,100,103,104,200,201,202,204,205,208,209,210,211,220,300,301,302,303)
                    where
                    (c.Height / 1440) < (? / 1440) and
                    (c.Height / 1440) >= (? / 1440)
                    group by
                        (c.Height / 1440), t.Type
                )sql")
                .Bind(topHeight, topHeight - depth);
            },
            [&] (Stmt& stmt) {
                stmt.Select([&](Cursor& cursor) {
                    while (cursor.Step())
                    {
                        int part, type, count;
                        if (cursor.CollectAll(part, type, count))
                        {
                            if (result.At(to_string(part)).isNull())
                                result.pushKV(to_string(part), UniValue(UniValue::VOBJ));

                            result.At(to_string(part)).pushKV(to_string(type), count);
                        }
                    }
                });
            }
        );

        return result;
    }

    UniValue ExplorerRepository::GetContentStatisticByHours(int topHeight, int depth)
    {
        UniValue result(UniValue::VOBJ);

        SqlTransaction(
            __func__,
            [&]() -> Stmt& {
                return Sql(R"sql(
                    with
                        maxHeight as ( select ? as value ),
                        minHeight as ( select ? as value ),
                        base as (
                            select
                                count() as value
                            from
                                Transactions u
                            cross join
                                First f
                                    on f.TxId = u.RowId
                            where
                                u.Type in (100)
                        ),
                        val as (
                            select
                                (c.Height / 60) as height,
                                (
                                    select
                                        count()
                                    from
                                        Chain c1 indexed by Chain_HeightByHour
                                    cross join
                                        Transactions u on
                                            u.RowId = c1.TxId and u.Type in (100)
                                    cross join
                                        First f
                                            on f.TxId = u.RowId
                                    where
                                        (c1.Height / 60) = (c.Height / 60)
                                ) as cnt
                            from
                                maxHeight,
                                minHeight,
                                Chain c indexed by Chain_HeightByHour
                            where
                                (c.Height / 60) <= (maxHeight.value / 60) and
                                (c.Height / 60) > (minHeight.value / 60)
                            group by
                                (c.Height / 60)
                            order by
                                (c.Height / 60) desc
                        )
                    select
                        v.height,
                        (
                            select
                                ifnull(base.value - sum(v2.cnt), base.value)
                            from
                                val v2
                            where
                                v2.height > v.height
                        ) as cnt
                    from
                        base,
                        val v
                )sql")
                .Bind(topHeight, topHeight - depth);
            },
            [&] (Stmt& stmt) {
                stmt.Select([&](Cursor& cursor) {
                    while (cursor.Step())
                    {
                        int part, count;
                        if (cursor.CollectAll(part, count))
                            result.pushKV(to_string(part), count);
                    }
                });
            }
        );

        return result;
    }

    UniValue ExplorerRepository::GetContentStatisticByDays(int topHeight, int depth)
    {
        UniValue result(UniValue::VOBJ);

        SqlTransaction(
            __func__,
            [&]() -> Stmt& {
                return Sql(R"sql(
                    with
                        maxHeight as ( select ? as value ),
                        minHeight as ( select ? as value ),
                        base as (
                            select
                                count() as value
                            from
                                Transactions u
                            cross join
                                First f
                                    on f.TxId = u.RowId
                            where
                                u.Type in (100)
                        ),
                        val as (
                            select
                                (c.Height / 1440) as height,
                                (
                                    select
                                        count()
                                    from
                                        Chain c1 indexed by Chain_HeightByDay
                                    cross join
                                        Transactions u on
                                            u.RowId = c1.TxId and u.Type in (100)
                                    cross join
                                        First f
                                            on f.TxId = u.RowId
                                    where
                                        (c1.Height / 1440) = (c.Height / 1440)
                                ) as cnt
                            from
                                maxHeight,
                                minHeight,
                                Chain c indexed by Chain_HeightByDay
                            where
                                (c.Height / 1440) <= (maxHeight.value / 1440) and
                                (c.Height / 1440) > (minHeight.value / 1440)
                            group by
                                (c.Height / 1440)
                            order by
                                (c.Height / 1440) desc
                        )
                    select
                        v.height,
                        (
                            select
                                ifnull(base.value - sum(v2.cnt), base.value)
                            from
                                val v2
                            where
                                v2.height > v.height
                        ) as cnt
                    from
                        base,
                        val v
                )sql")
                .Bind(topHeight, topHeight - depth);
            },
            [&] (Stmt& stmt) {
                stmt.Select([&](Cursor& cursor)
                {
                    while (cursor.Step())
                    {
                        int part, count;
                        if (cursor.CollectAll(part, count))
                            result.pushKV(to_string(part), count);
                    }
                });
            }
        );

        return result;
    }

    UniValue ExplorerRepository::GetContentStatistic()
    {
        UniValue result(UniValue::VOBJ);

        SqlTransaction(
            __func__,
            [&]() -> Stmt& {
                return Sql(R"sql(
                    select
                        t.Type,
                        count()
                    from Transactions t indexed by Transactions_Type_RegId2_RegId1
                    cross join Last l on
                        l.TxId = t.RowId
                    where
                        t.Type in (1,100,103,104,200,201,202,204,205,208,209,210,211,220,300,301,302,303)
                    group by
                        t.Type
                )sql");
            },
            [&] (Stmt& stmt) {
                stmt.Select([&](Cursor& cursor) {
                    while (cursor.Step())
                    {
                        int type, count;
                        if (cursor.CollectAll(type, count))
                            result.pushKV(to_string(type), count);
                    }
                });
            }
        );

        return result;
    }

    map<string, tuple<int, int64_t>> ExplorerRepository::GetAddressesInfo(const vector<string>& hashes)
    {
        map<string, tuple<int, int64_t>> infos{};

        if (hashes.empty())
            return infos;

        SqlTransaction(
            __func__,
            [&]() -> Stmt& {
                return Sql(R"sql(
                    with addresses as (
                        select
                            r.String as hash,
                            r.RowId as id
                        from
                            Registry r
                        where
                            r.String in ( )sql" + join(vector<string>(hashes.size(), "?"), ",") + R"sql( )
                    )
                    select
                        a.hash,
                        b.Value
                    from
                        Balances b
                        join addresses a on
                            b.AddressId = a.id
                )sql")
                .Bind(hashes);
            },
            [&] (Stmt& stmt) {
                stmt.Select([&](Cursor& cursor) {
                    while (cursor.Step())
                    {
                        string address; int64_t value;
                        if (cursor.CollectAll(address, value))
                            // TODO (optimization): height removed from balances, passing here "-1"
                            // may be calculate on the flight
                            infos.emplace(address, make_tuple(-1, value));
                    }
                });
            }
        );

        return infos;
    }

    map<string, int> ExplorerRepository::GetAddressTransactions(const string& address, int topHeight, int pageStart, int pageSize, int direction, const vector<TxType>& types)
    {
        map<string, int> txHashes;

        string sqlIncoming = R"sql(
            ,outputs as (
                select distinct
                    t.RowId,
                    c.Height
                from
                    TxOutputs o indexed by TxOutputs_AddressId_TxIdDesc_Number,
                    address,
                    topheight
                cross join
                    Transactions t on
                        t.RowId = o.TxId and
                        ( ? or t.Type in ( )sql" + join(vector<string>(types.size(), "?"), ",") + R"sql( ) )
                cross join
                    Chain c indexed by Chain_TxId_Height on
                        c.TxId = o.TxId and
                        c.Height <= topheight.value
                where
                    o.AddressId = address.id
                limit ?
            )
        )sql";

        string sqlOutgoing = R"sql(
            ,inputs as (
                select distinct
                    t.RowId
                from
                    TxOutputs o indexed by TxOutputs_AddressId_TxIdDesc_Number,
                    address,
                    topheight
                cross join
                    TxInputs i indexed by TxInputs_TxId_Number_SpentTxId on
                        i.TxId = o.TxId and i.Number = o.Number
                cross join
                    Transactions t on
                        t.RowId = i.SpentTxId and
                        ( ? or t.Type in ( )sql" + join(vector<string>(types.size(), "?"), ",") + R"sql( ) )
                cross join
                    Chain c indexed by Chain_TxId_Height on
                        c.TxId = o.TxId and
                        c.Height <= topheight.value
                where
                    o.AddressId = address.id
                limit ?
            )
        )sql";

        string sqlEnd = R"sql(
            with
                topHeight as ( select ? as value ),
                address as ( select r.RowId as id from Registry r where r.String = ? )
                
                )sql" + (direction == 0 || direction == 1 ? sqlIncoming : "") + R"sql(
                )sql" + (direction == 0 || direction == -1 ? sqlOutgoing : "") + R"sql(
                
            )sql" +
                (direction == 0 || direction == 1
                    ? R"sql( select (select r.String from Registry r where r.RowId = o.RowId) as Hash, o.RowId from outputs o )sql" 
                    : ""
                ) +
            R"sql(

            )sql" + (direction == 0 ? " union " : "") + R"sql(

            )sql" +
                (direction == 0 || direction == -1
                    ? R"sql( select (select r.String from Registry r where r.RowId = i.RowId) as Hash, i.RowId from inputs i )sql" 
                    : ""
                ) +
            R"sql(
            
            order by RowId desc
            limit ? offset ?
        )sql";

        SqlTransaction(
            __func__,
            [&]() -> Stmt& {
                auto& stmt = Sql(sqlEnd);

                stmt.Bind(
                    topHeight,
                    address
                );

                if (direction == 0 || direction == 1)
                {
                    stmt.Bind(
                        types.empty(),
                        types,
                        pageSize + pageStart
                    );
                }

                if (direction == 0 || direction == -1)
                {
                    stmt.Bind(
                        types.empty(),
                        types,
                        pageSize + pageStart
                    );
                }

                stmt.Bind(
                    pageSize,
                    pageStart
                );

                return stmt;
            },
            [&] (Stmt& stmt) {
                stmt.Select([&](Cursor& cursor) {
                    int i = 0;
                    while (cursor.Step())
                    {
                        if (auto[ok, value] = cursor.TryGetColumnString(0); ok)
                            txHashes.emplace(value, i++);
                    }
                });
            }
        );

        return txHashes;
    }

    map<string, int> ExplorerRepository::GetBlockTransactions(const string& blockHash, int pageStart, int pageSize)
    {
        map<string, int> txHashes;

        SqlTransaction(
            __func__,
            [&]() -> Stmt& {
                return Sql(R"sql(
                    with block as (
                        select
                            r.RowId as id
                        from
                            Registry r
                        where
                            r.String = ?
                    )
                    select
                        (select r.String from Registry r where r.RowId = t.RowId)
                    from
                        block
                    cross join
                        Chain c indexed by Chain_BlockId_Height
                            on c.BlockId = block.id
                    cross join
                        Transactions t
                            on t.RowId = c.TxId
                    order by
                        c.BlockNum asc
                    limit
                        ?, ?
                )sql")
                .Bind(blockHash, pageStart, pageSize);
            },
            [&] (Stmt& stmt) {
                stmt.Select([&](Cursor& cursor) {
                    int i = 0;
                    while (cursor.Step())
                    {
                        if (auto[ok, value] = cursor.TryGetColumnString(0); ok)
                            txHashes.emplace(value, i++);
                    }
                });
            }
        );

        return txHashes;
    }
    
    // TODO (aok, api) : implement
    UniValue ExplorerRepository::GetBalanceHistory(const vector<string>& addresses, int topHeight, int count)
    {
        UniValue result(UniValue::VARR);

        SqlTransaction(
            __func__,
            [&]() -> Stmt& {
                return Sql(R"sql(
                    select b.Height, sum(b.Value)Amount
                    from Balances b indexed by Balances_Height
                    where b.AddressHash in ( )sql" + join(vector<string>(addresses.size(), "?"), ",") + R"sql( )
                    and b.Height <= ?
                    group by b.Height
                    order by b.Height desc
                    limit ?
                )sql")
                .Bind(addresses, topHeight, count);
            },
            [&] (Stmt& stmt) {
                stmt.Select([&](Cursor& cursor) {
                    while (cursor.Step())
                    {
                        if (auto[okHeight, height] = cursor.TryGetColumnInt(0); okHeight)
                        {
                            if (auto[okValue, value] = cursor.TryGetColumnInt64(1); okValue)
                            {
                                UniValue record(UniValue::VARR);
                                record.push_back(height);
                                record.push_back(value);
                                
                                result.push_back(record);
                            }
                        }
                    }
                });
            }
        );

        return result;
    }
}