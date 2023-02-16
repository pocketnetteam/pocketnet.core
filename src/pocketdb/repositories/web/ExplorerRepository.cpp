// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include "pocketdb/repositories/web/ExplorerRepository.h"

namespace PocketDb
{
    map<int, map<int, int>> ExplorerRepository::GetBlocksStatistic(int bottomHeight, int topHeight)
    {
        map<int, map<int, int>> result;

        SqlTransaction(__func__, [&]()
        {
            Sql(R"sql(
                select
                    c.Height,
                    t.Type,
                    count(1)
                from
                    Chain c indexed by Chain_Height_BlockId

                    left join Transactions t on
                        t.RowId = c.TxId
                where
                    c.Height > ? and
                    c.Height <= ?
                group by
                    c.Height, t.Type
            )sql")
            .Bind(bottomHeight, topHeight)
            .Select([&](Cursor& cursor) {
                while (cursor.Step())
                {
                    int sHeight, sType, sCount;
                    if (cursor.CollectAll(sHeight, sType, sCount)) 
                        result[sHeight][sType] = sCount;
                }
            });
        });

        return result;
    }

    UniValue ExplorerRepository::GetTransactionsStatistic(int64_t top, int depth, int period)
    {
        UniValue result(UniValue::VOBJ);

        SqlTransaction(__func__, [&]()
        {
            Sql(R"sql(
                select
                    (t.Time / ?),
                    t.Type,
                    count()
                from
                    Transactions t
                where
                    t.Type in (1,100,103,200,201,202,204,205,208,209,210,300,301,302,303) and
                    t.Time >= ? and
                    t.time < ?
                group by
                    t.time / ?, t.Type
            )sql")
            .Bind(period, top - (depth * period), top, period)
            .Select([&](Cursor& cursor) {
                while (cursor.Step())
                {
                    std::string part, type; int count;
                    if (cursor.CollectAll(part, type, count))
                    {
                        if (result.At(part).isNull())
                            result.pushKV(part, UniValue(UniValue::VOBJ));

                        result.At(part).pushKV(type, count);
                    }
                }
            });
        });

        return result;
    }

    UniValue ExplorerRepository::GetTransactionsStatisticByHours(int topHeight, int depth)
    {
        UniValue result(UniValue::VOBJ);

        SqlTransaction(__func__, [&]()
        {
            Sql(R"sql(
                select
                    (c.Height / 60)Hour,
                    t.Type,
                    count()Count

                from
                    Chain c indexed by Chain_Height_Uid

                    cross join Transactions t on
                        t.RowId = c.TxId and
                        t.Type in (1,100,103,200,201,202,204,205,208,209,210,300,301,302,303)

                where
                  (c.Height / 60) < (? / 60) and
                  (c.Height / 60) >= (? / 60)

                group by
                    (c.Height / 60), t.Type
            )sql")
            .Bind(topHeight, topHeight - depth)
            .Select([&](Cursor& cursor) {
                while (cursor.Step())
                {
                    std::string part; // TODO (optimization): should it be string???
                    std::string type; // TODO (optimization): should it be string???
                    int count {};
                    if (cursor.CollectAll(part, type, count))
                    {
                        if (result.At(part).isNull())
                            result.pushKV(part, UniValue(UniValue::VOBJ));

                        result.At(part).pushKV(type, count);
                    }
                }
            });
        });

        return result;
    }

    UniValue ExplorerRepository::GetTransactionsStatisticByDays(int topHeight, int depth)
    {
        UniValue result(UniValue::VOBJ);

        SqlTransaction(__func__, [&]()
        {
            Sql(R"sql(
                select
                    (c.Height / 1440)Day,
                    t.Type,
                    count()Count

                from
                    Chain c indexed by Chain_Height_Uid

                    join Transactions t on
                        t.RowId = c.TxId and
                        t.Type in (1,100,103,200,201,202,204,205,208,209,210,300,301,302,303)

                where
                  (c.Height / 1440) < (? / 1440) and
                  (c.Height / 1440) >= (? / 1440)

                group by
                    (c.Height / 1440), t.Type
            )sql")
            .Bind(topHeight, topHeight - depth)
            .Select([&](Cursor& cursor) {
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
        });

        return result;
    }

    UniValue ExplorerRepository::GetContentStatisticByHours(int topHeight, int depth)
    {
        UniValue result(UniValue::VOBJ);

        SqlTransaction(__func__, [&]()
        {
            Sql(R"sql(
                select
                    (c.Height / 60),
                    (
                        select
                            count()
                        from Transactions u1
                        join Chain c1 on
                            c1.TxId = u1.RowId and
                            c1.Height <= c.Height and
                            exists (select 1 from Last l where l.TxId = c1.TxId)
                        where
                            u1.Type in (100)
                    )cnt
                from
                    Transactions u
                    join Chain c indexed by Chain_Height_Uid on
                        c.TxId = u.RowId and
                        (c.Height / 60) <= (? / 60) and
                        (c.Height / 60) > (? / 60)
                where
                    u.Type in (3)
                group by
                    (c.Height / 60)
                order by
                    (c.Height / 60) desc
            )sql")
            .Bind(topHeight, topHeight - depth)
            .Select([&](Cursor& cursor) {
                while (cursor.Step())
                {
                    std::string part; int count;
                    if (cursor.CollectAll(part, count))
                        result.pushKV(part, count);
                }
            });
        });

        return result;
    }

    UniValue ExplorerRepository::GetContentStatisticByDays(int topHeight, int depth)
    {
        UniValue result(UniValue::VOBJ);

        SqlTransaction(__func__, [&]()
        {
            Sql(R"sql(
                select
                    (c.Height / 1440),
                    (
                        select
                            count()
                        from Transactions u1
                        join Chain c1 on
                            c1.TxId = u1.RowId and
                            c1.Height <= c.Height and
                            exists (select 1 from Last l where l.TxId = c1.TxId)
                        where
                            u1.Type in (100)
                    )cnt
                from
                    Transactions u
                    join Chain c indexed by Chain_Height_Uid on
                        c.TxId = u.RowId and
                        (c.Height / 1440) <= (? / 1440) and
                        (c.Height / 1440) > (? / 1440)
                where
                    u.Type in (3)
                group by
                    (c.Height / 1440)
                order by
                    (c.Height / 1440) desc
            )sql")
            .Bind(topHeight, topHeight - depth)
            .Select([&](Cursor& cursor)
            {
                while (cursor.Step())
                {
                    std::string part; int count;
                    if (cursor.CollectAll(part, count))
                        result.pushKV(part, count);
                }
            });
        });

        return result;
    }

    UniValue ExplorerRepository::GetContentStatistic()
    {
        UniValue result(UniValue::VOBJ);

        SqlTransaction(__func__, [&]()
        {
            Sql(R"sql(
                select t.Type, count()
                from Transactions t indexed by Transactions_Type_Last_Height_Id
                where t.Type in (100,200,201,202,208,209,210)
                  and t.Last = 1
                  and t.Height > 0
                group by t.Type
            )sql")
            .Select([&](Cursor& cursor) {
                while (cursor.Step())
                {
                    auto [okType, type] = cursor.TryGetColumnString(0);
                    auto [okCount, count] = cursor.TryGetColumnInt(1);

                    if (okType && okCount)
                        result.pushKV(type, count);
                }
            });
        });

        return result;
    }

    map<string, tuple<int, int64_t>> ExplorerRepository::GetAddressesInfo(const vector<string>& hashes)
    {
        map<string, tuple<int, int64_t>> infos{};

        if (hashes.empty())
            return infos;

        SqlTransaction(__func__, [&]()
        {
            Sql(R"sql(
                select AddressHash, Height, Value
                from Balances indexed by Balances_AddressHash_Last
                where AddressHash in ( )sql" + join(vector<string>(hashes.size(), "?"), ",") + R"sql( )
                  and Last = 1
            )sql")
            .Bind(hashes)
            .Select([&](Cursor& cursor) {
                while (cursor.Step())
                {
                    auto [ok0, address] = cursor.TryGetColumnString(0);
                    auto [ok1, height] = cursor.TryGetColumnInt(1);
                    auto [ok2, value] = cursor.TryGetColumnInt64(2);

                    if (ok0 && ok1 && ok2)
                        infos.emplace(address, make_tuple(height, value));
                }
            });
        });

        return infos;
    }

    map<string, int> ExplorerRepository::GetAddressTransactions(const string& address, int pageInitBlock, int pageStart, int pageSize)
    {
        map<string, int> txHashes;

        SqlTransaction(__func__, [&]()
        {
            Sql(R"sql(
                select distinct o.TxHash
                from TxOutputs o indexed by TxOutputs_AddressHash_TxHeight_SpentHeight
                join Transactions t on t.Hash = o.TxHash
                where o.AddressHash = ?
                  and o.TxHeight <= ?
                order by o.TxHeight desc, t.BlockNum desc
                limit ?, ?
            )sql")
            .Bind(address, pageInitBlock, pageSize, pageSize)
            .Select([&](Cursor& cursor) {
                int i = 0;
                while (cursor.Step())
                {
                    if (auto[ok, value] = cursor.TryGetColumnString(0); ok)
                        txHashes.emplace(value, i++);
                }
            });
        });

        return txHashes;
    }

    map<string, int> ExplorerRepository::GetBlockTransactions(const string& blockHash, int pageStart, int pageSize)
    {
        map<string, int> txHashes;

        SqlTransaction(__func__, [&]()
        {
            Sql(R"sql(
                select t.Hash
                from Transactions t indexed by Transactions_BlockHash
                where t.BlockHash = ?
                order by t.BlockNum asc
                limit ?, ?
            )sql")
            .Bind(blockHash, pageStart, pageSize)
            .Select([&](Cursor& cursor) {
                int i = 0;
                while (cursor.Step())
                {
                    if (auto[ok, value] = cursor.TryGetColumnString(0); ok)
                        txHashes.emplace(value, i++);
                }
            });
        });

        return txHashes;
    }
    
    UniValue ExplorerRepository::GetBalanceHistory(const vector<string>& addresses, int topHeight, int count)
    {
        UniValue result(UniValue::VARR);

        SqlTransaction(__func__, [&]()
        {
            Sql(R"sql(
                select b.Height, sum(b.Value)Amount
                from Balances b indexed by Balances_Height
                where b.AddressHash in ( )sql" + join(vector<string>(addresses.size(), "?"), ",") + R"sql( )
                  and b.Height <= ?
                group by b.Height
                order by b.Height desc
                limit ?
            )sql")
            .Bind(addresses, topHeight, count)
            .Select([&](Cursor& cursor) {
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
        });

        return result;
    }
}