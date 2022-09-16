// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include "ExplorerRepository.h"

namespace PocketDb {

    void ExplorerRepository::Init() {}

    void ExplorerRepository::Destroy() {}

    map<int, map<int, int>> ExplorerRepository::GetBlocksStatistic(int bottomHeight, int topHeight)
    {
        map<int, map<int, int>> result;

        TryTransactionStep(__func__, [&]()
        {
            auto stmt = SetupSqlStatement(R"sql(
                select t.Height, t.Type, count(*)
                from Transactions t indexed by Transactions_Height_Type
                where   t.Height > ?
                    and t.Height <= ?
                group by t.Height, t.Type
            )sql");

            TryBindStatementInt(stmt, 1, bottomHeight);
            TryBindStatementInt(stmt, 2, topHeight);

            while (sqlite3_step(*stmt) == SQLITE_ROW)
            {
                auto [ok0, sHeight] = TryGetColumnInt(*stmt, 0);
                auto [ok1, sType] = TryGetColumnInt(*stmt, 1);
                auto [ok2, sCount] = TryGetColumnInt(*stmt, 2);

                if (ok0 && ok1 && ok2)
                    result[sHeight][sType] = sCount;
            }

            FinalizeSqlStatement(*stmt);
        });

        return result;
    }

    UniValue ExplorerRepository::GetTransactionsStatistic(int64_t top, int depth, int period)
    {
        UniValue result(UniValue::VOBJ);

        TryTransactionStep(__func__, [&]()
        {
            auto stmt = SetupSqlStatement(R"sql(
                select (t.Time / ?), t.Type, count()
                from Transactions t
                where t.Type in (1,100,103,200,201,202,204,205,208,300,301,302,303)
                  and t.Time >= ?
                  and t.time < ?
                group by t.time / ?, t.Type
            )sql");

            int i = 1;
            TryBindStatementInt(stmt, i++, period);
            TryBindStatementInt64(stmt, i++, top - (depth * period));
            TryBindStatementInt64(stmt, i++, top);
            TryBindStatementInt(stmt, i++, period);

            while (sqlite3_step(*stmt) == SQLITE_ROW)
            {
                auto [okPart, part] = TryGetColumnString(*stmt, 0);
                auto [okType, type] = TryGetColumnString(*stmt, 1);
                auto [okCount, count] = TryGetColumnInt(*stmt, 2);

                if (!okPart || !okType || !okCount)
                    continue;

                if (result.At(part).isNull())
                    result.pushKV(part, UniValue(UniValue::VOBJ));

                result.At(part).pushKV(type, count);
            }

            FinalizeSqlStatement(*stmt);
        });

        return result;
    }

    UniValue ExplorerRepository::GetTransactionsStatisticByHours(int topHeight, int depth)
    {
        UniValue result(UniValue::VOBJ);

        TryTransactionStep(__func__, [&]()
        {
            auto stmt = SetupSqlStatement(R"sql(
                select (t.Height / 60)Hour, t.Type, count()Count
                from Transactions t indexed by Transactions_Type_HeightByHour
                where t.Type in (1,100,103,200,201,202,204,205,208,300,301,302,303)
                  and (t.Height / 60) < (? / 60)
                  and (t.Height / 60) >= (? / 60)
                group by (t.Height / 60), t.Type
            )sql");

            TryBindStatementInt(stmt, 1, topHeight);
            TryBindStatementInt(stmt, 2, topHeight - depth);

            while (sqlite3_step(*stmt) == SQLITE_ROW)
            {
                auto [okPart, part] = TryGetColumnString(*stmt, 0);
                auto [okType, type] = TryGetColumnString(*stmt, 1);
                auto [okCount, count] = TryGetColumnInt(*stmt, 2);

                if (!okPart || !okType || !okCount)
                    continue;

                if (result.At(part).isNull())
                    result.pushKV(part, UniValue(UniValue::VOBJ));

                result.At(part).pushKV(type, count);
            }

            FinalizeSqlStatement(*stmt);
        });

        return result;
    }

    UniValue ExplorerRepository::GetTransactionsStatisticByDays(int topHeight, int depth)
    {
        UniValue result(UniValue::VOBJ);

        TryTransactionStep(__func__, [&]()
        {
            auto stmt = SetupSqlStatement(R"sql(
                select (t.Height / 1440)Day, t.Type, count()Count
                from Transactions t indexed by Transactions_Type_HeightByDay
                where t.Type in (1,100,103,200,201,202,204,205,208,300,301,302,303)
                  and (t.Height / 1440) < (? / 1440)
                  and (t.Height / 1440) >= (? / 1440)
                group by (t.Height / 1440), t.Type
            )sql");

            TryBindStatementInt(stmt, 1, topHeight);
            TryBindStatementInt(stmt, 2, topHeight - depth);

            while (sqlite3_step(*stmt) == SQLITE_ROW)
            {
                auto [okPart, part] = TryGetColumnInt(*stmt, 0);
                auto [okType, type] = TryGetColumnInt(*stmt, 1);
                auto [okCount, count] = TryGetColumnInt(*stmt, 2);

                if (!okPart || !okType || !okCount)
                    continue;

                if (result.At(to_string(part)).isNull())
                    result.pushKV(to_string(part), UniValue(UniValue::VOBJ));

                result.At(to_string(part)).pushKV(to_string(type), count);
            }

            FinalizeSqlStatement(*stmt);
        });

        return result;
    }

    UniValue ExplorerRepository::GetContentStatisticByHours(int topHeight, int depth)
    {
        UniValue result(UniValue::VOBJ);

        TryTransactionStep(__func__, [&]()
        {
            auto stmt = SetupSqlStatement(R"sql(
                select (u.Height / 60)
                  ,(
                    select
                      count()
                    from Transactions u1 indexed by Transactions_Type_Last_Height_Id
                    where u1.Type in (100)
                    and u1.Height <= u.Height
                    and u1.Last = 1
                  )cnt
                from Transactions u indexed by Transactions_Type_HeightByHour
                where u.Type in (3)
                  and (u.Height / 60) <= (? / 60)
                  and (u.Height / 60) > (? / 60)

                group by (u.Height / 60)
                order by (u.Height / 60) desc
            )sql");

            TryBindStatementInt(stmt, 1, topHeight);
            TryBindStatementInt(stmt, 2, topHeight - depth);

            while (sqlite3_step(*stmt) == SQLITE_ROW)
            {
                auto [okPart, part] = TryGetColumnString(*stmt, 0);
                auto [okCount, count] = TryGetColumnInt(*stmt, 1);

                if (!okPart || !okCount)
                    continue;

                result.pushKV(part, count);
            }

            FinalizeSqlStatement(*stmt);
        });

        return result;
    }

    UniValue ExplorerRepository::GetContentStatisticByDays(int topHeight, int depth)
    {
        UniValue result(UniValue::VOBJ);

        TryTransactionStep(__func__, [&]()
        {
            auto stmt = SetupSqlStatement(R"sql(
                select (u.Height / 1440)
                  ,(
                    select
                      count()
                    from Transactions u1 indexed by Transactions_Type_Last_Height_Id
                    where u1.Type in (100)
                    and u1.Height <= u.Height
                    and u1.Last = 1
                  )cnt

                from Transactions u indexed by Transactions_Type_HeightByDay
                
                where u.Type in (3)
                  and (u.Height / 1440) <= (? / 1440)
                  and (u.Height / 1440) > (? / 1440)
                
                group by (u.Height / 1440)
                order by (u.Height / 1440) desc
            )sql");

            TryBindStatementInt(stmt, 1, topHeight);
            TryBindStatementInt(stmt, 2, topHeight - depth);

            while (sqlite3_step(*stmt) == SQLITE_ROW)
            {
                auto [okPart, part] = TryGetColumnString(*stmt, 0);
                auto [okCount, count] = TryGetColumnInt(*stmt, 1);

                if (!okPart || !okCount)
                    continue;

                result.pushKV(part, count);
            }

            FinalizeSqlStatement(*stmt);
        });

        return result;
    }

    UniValue ExplorerRepository::GetContentStatistic()
    {
        UniValue result(UniValue::VOBJ);

        TryTransactionStep(__func__, [&]()
        {
            auto stmt = SetupSqlStatement(R"sql(
                select t.Type, count()
                from Transactions t indexed by Transactions_Type_Last_Height_Id
                where t.Type in (100,200,201,202,208)
                  and t.Last = 1
                  and t.Height > 0
                group by t.Type
            )sql");

            while (sqlite3_step(*stmt) == SQLITE_ROW)
            {
                auto [okType, type] = TryGetColumnString(*stmt, 0);
                auto [okCount, count] = TryGetColumnInt(*stmt, 1);

                if (okType && okCount)
                    result.pushKV(type, count);
            }

            FinalizeSqlStatement(*stmt);
        });

        return result;
    }

    map<string, tuple<int, int64_t>> ExplorerRepository::GetAddressesInfo(const vector<string>& hashes)
    {
        map<string, tuple<int, int64_t>> infos{};

        if (hashes.empty())
            return infos;

        TryTransactionStep(__func__, [&]()
        {
            auto stmt = SetupSqlStatement(R"sql(
                select AddressHash, Height, Value
                from Balances indexed by Balances_AddressHash_Last
                where AddressHash in ( )sql" + join(vector<string>(hashes.size(), "?"), ",") + R"sql( )
                  and Last = 1
            )sql");

            size_t i = 1;
            for (auto& hash : hashes)
                TryBindStatementText(stmt, i++, hash);

            while (sqlite3_step(*stmt) == SQLITE_ROW)
            {
                auto [ok0, address] = TryGetColumnString(*stmt, 0);
                auto [ok1, height] = TryGetColumnInt(*stmt, 1);
                auto [ok2, value] = TryGetColumnInt64(*stmt, 2);

                if (ok0 && ok1 && ok2)
                    infos.emplace(address, make_tuple(height, value));
            }

            FinalizeSqlStatement(*stmt);
        });

        return infos;
    }

    map<string, int> ExplorerRepository::GetAddressTransactions(const string& address, int pageInitBlock, int pageStart, int pageSize)
    {
        map<string, int> txHashes;

        TryTransactionStep(__func__, [&]()
        {
            auto stmt = SetupSqlStatement(R"sql(
                select distinct o.TxHash
                from TxOutputs o indexed by TxOutputs_AddressHash_TxHeight_SpentHeight
                join Transactions t on t.Hash = o.TxHash
                where o.AddressHash = ?
                  and o.TxHeight <= ?
                order by o.TxHeight desc, t.BlockNum desc
                limit ?, ?
            )sql");

            TryBindStatementText(stmt, 1, address);
            TryBindStatementInt(stmt, 2, pageInitBlock);
            TryBindStatementInt(stmt, 3, pageStart);
            TryBindStatementInt(stmt, 4, pageSize);

            int i = 0;
            while (sqlite3_step(*stmt) == SQLITE_ROW)
            {
                if (auto[ok, value] = TryGetColumnString(*stmt, 0); ok)
                    txHashes.emplace(value, i++);
            }

            FinalizeSqlStatement(*stmt);
        });

        return txHashes;
    }

    map<string, int> ExplorerRepository::GetBlockTransactions(const string& blockHash, int pageStart, int pageSize)
    {
        map<string, int> txHashes;

        TryTransactionStep(__func__, [&]()
        {
            auto stmt = SetupSqlStatement(R"sql(
                select t.Hash
                from Transactions t indexed by Transactions_BlockHash
                where t.BlockHash = ?
                order by t.BlockNum asc
                limit ?, ?
            )sql");

            TryBindStatementText(stmt, 1, blockHash);
            TryBindStatementInt(stmt, 2, pageStart);
            TryBindStatementInt(stmt, 3, pageSize);

            int i = 0;
            while (sqlite3_step(*stmt) == SQLITE_ROW)
            {
                if (auto[ok, value] = TryGetColumnString(*stmt, 0); ok)
                    txHashes.emplace(value, i++);
            }

            FinalizeSqlStatement(*stmt);
        });

        return txHashes;
    }
    
    UniValue ExplorerRepository::GetBalanceHistory(const vector<string>& addresses, int topHeight, int count)
    {
        UniValue result(UniValue::VARR);

        TryTransactionStep(__func__, [&]()
        {
            auto stmt = SetupSqlStatement(R"sql(
                select b.Height, sum(b.Value)Amount
                from Balances b indexed by Balances_Height
                where b.AddressHash in ( )sql" + join(vector<string>(addresses.size(), "?"), ",") + R"sql( )
                  and b.Height <= ?
                group by b.Height
                order by b.Height desc
                limit ?
            )sql");

            int i = 1;
            for (const string& address : addresses)
                TryBindStatementText(stmt, i++, address);
            TryBindStatementInt(stmt, i++, topHeight);
            TryBindStatementInt(stmt, i++, count);

            while (sqlite3_step(*stmt) == SQLITE_ROW)
            {
                if (auto[okHeight, height] = TryGetColumnInt(*stmt, 0); okHeight)
                {
                    if (auto[okValue, value] = TryGetColumnInt64(*stmt, 1); okValue)
                    {
                        UniValue record(UniValue::VARR);
                        record.push_back(height);
                        record.push_back(value);
                        
                        result.push_back(record);
                    }
                }
            }

            FinalizeSqlStatement(*stmt);
        });

        return result;
    }
}