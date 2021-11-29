// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include "ExplorerRepository.h"

namespace PocketDb {

    void ExplorerRepository::Init() {}

    void ExplorerRepository::Destroy() {}

    map<int, map<int, int>> ExplorerRepository::GetStatistic(int bottomHeight, int topHeight)
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

    UniValue ExplorerRepository::GetStatistic(int64_t startTime, int64_t endTime, StatisticDepth depth)
    {
        auto func = __func__;
        UniValue result(UniValue::VOBJ);

        // TODO (brangr): Optimization is needed
        return result;

        string formatTime;
        string delimiter;
        switch (depth)
        {
            case StatisticDepth_Day:
                formatTime = "%Y%m%d%H";
                delimiter = "3600";
                break;
            default:
            case StatisticDepth_Month:
                formatTime = "%Y%m%d";
                delimiter = "604800";
                break;
            case StatisticDepth_Year:
                formatTime = "%Y%m";
                delimiter = "604800";
                break;
        }

        TryTransactionStep(func, [&]()
        {
            auto stmt = SetupSqlStatement(R"sql(
                select 'txs',
                    strftime(')sql" + formatTime + R"sql(', datetime(max(t.Time), 'unixepoch')),
                    t.Type,
                    count(*)Count
                from Transactions t indexed by Transactions_Time_Type_Height
                where   t.Time >= ?
                    and t.Time < ?
                group by (t.Time / )sql" + delimiter + R"sql( ), t.Type

                union

                select 'users',
                    strftime(')sql" + formatTime + R"sql(', datetime(q.MaxTime, 'unixepoch')),
                    q.Type,
                    (
                        select count(*)
                        from Transactions t indexed by Transactions_Type_Time_Height
                        where t.Type = q.Type and (t.Time / )sql" + delimiter + R"sql( ) <= q.Time
                    )Count
                from (
                    select (t.Time / )sql" + delimiter + R"sql( )Time, max(t.Time)MaxTime, t.Type, count(*)Cnt
                    from Transactions t indexed by Transactions_Type_Time_Height
                    where   t.Type in (100, 101, 102)
                        and t.Time < ?
                    group by (t.Time / )sql" + delimiter + R"sql( ), t.Type
                ) q
                where q.Time >= (? / )sql" + delimiter + R"sql( )
            )sql");

            TryBindStatementInt64(stmt, 1, startTime);
            TryBindStatementInt64(stmt, 2, endTime);
            TryBindStatementInt64(stmt, 3, endTime);
            TryBindStatementInt64(stmt, 4, startTime);

            LogPrint(BCLog::SQL, "%s: %s\n", func, sqlite3_expanded_sql(*stmt));

            while (sqlite3_step(*stmt) == SQLITE_ROW)
            {
                auto [ok0, sGroup] = TryGetColumnString(*stmt, 0);
                auto [ok1, sDate] = TryGetColumnString(*stmt, 1);
                auto [ok2, sType] = TryGetColumnString(*stmt, 2);
                auto [ok3, sCount] = TryGetColumnInt(*stmt, 3);

                if (ok0 && ok1 && ok2 && ok3)
                {
                    if (result.At(sGroup).isNull())
                        result.pushKV(sGroup, UniValue(UniValue::VOBJ));

                    if (result.At(sGroup).At(sDate).isNull())
                        result.At(sGroup).pushKV(sDate, UniValue(UniValue::VOBJ));

                    result.At(sGroup).At(sDate).pushKV(sType, sCount);
                }
            }

            FinalizeSqlStatement(*stmt);
        });

        return result;
    }

    tuple<int, int64_t> ExplorerRepository::GetAddressInfo(const string& addressHash)
    {
        int lastChange = 0;
        int64_t balance = 0;

        TryTransactionStep(__func__, [&]()
        {
            auto stmt = SetupSqlStatement(R"sql(
                select Height, Value
                from Balances
                where AddressHash = ?
            )sql");

            TryBindStatementText(stmt, 1, addressHash);

            while (sqlite3_step(*stmt) == SQLITE_ROW)
            {
                auto [ok0, height] = TryGetColumnInt(*stmt, 0);
                auto [ok1, value] = TryGetColumnInt64(*stmt, 1);

                if (ok0 && ok1)
                {
                    lastChange = height;
                    balance = value;
                }
            }

            FinalizeSqlStatement(*stmt);
        });

        return {lastChange, balance};
    }

    template<typename T>
    UniValue ExplorerRepository::_getTransactions(T stmtOut)
    {
        UniValue result(UniValue::VARR);
        map<string, tuple<UniValue, UniValue, UniValue>> txs;

        // Select outputs
        {
            TryTransactionStep(__func__, [&]()
            {
                shared_ptr<sqlite3_stmt*> stmt;
                stmtOut(stmt);

                while (sqlite3_step(*stmt) == SQLITE_ROW)
                {
                    if (auto [ok0, hash] = TryGetColumnString(*stmt, 0); ok0)
                    {
                        // Create new transaction in array if not exists
                        if (txs.find(hash) == txs.end())
                        {
                            UniValue tx(UniValue::VOBJ);
                            UniValue vin(UniValue::VARR);
                            UniValue vout(UniValue::VARR);
                            
                            tx.pushKV("txid", hash);
                            if (auto [ok, value] = TryGetColumnInt(*stmt, 1); ok) tx.pushKV("rowNumber", value);
                            if (auto [ok, value] = TryGetColumnInt(*stmt, 2); ok) tx.pushKV("type", value);
                            if (auto [ok, value] = TryGetColumnInt(*stmt, 3); ok) tx.pushKV("height", value);
                            if (auto [ok, value] = TryGetColumnString(*stmt, 4); ok) tx.pushKV("blockHash", value);
                            if (auto [ok, value] = TryGetColumnInt64(*stmt, 5); ok) tx.pushKV("nTime", value);

                            tuple<UniValue, UniValue, UniValue> _tx{tx, vin, vout};
                            txs.emplace(hash, _tx);
                        }

                        // Extend transaction with outputs
                        UniValue txOut(UniValue::VOBJ);
                        if (auto [ok, value] = TryGetColumnInt(*stmt, 6); ok) txOut.pushKV("n", value);
                        if (auto [ok, value] = TryGetColumnInt64(*stmt, 8); ok) txOut.pushKV("value", value / 100000000.0);

                        {
                            UniValue scriptPubKey(UniValue::VOBJ);

                            if (auto [ok, value] = TryGetColumnString(*stmt, 7); ok)
                            {
                                UniValue addr(UniValue::VARR);
                                addr.read(value);
                                scriptPubKey.pushKV("addresses", addr);
                            }

                            if (auto [ok, value] = TryGetColumnString(*stmt, 9); ok)
                                scriptPubKey.pushKV("hex", value);

                            txOut.pushKV("scriptPubKey", scriptPubKey);
                        }

                        get<2>(txs[hash]).push_back(txOut);
                    }
                }

                FinalizeSqlStatement(*stmt);
            });
        }

        // Select inputs
        {
            string sql = R"sql(
                select SpentTxHash, TxHash, Number, AddressHash, Value
                from TxOutputs indexed by TxOutputs_SpentTxHash
                where SpentTxHash in ( )sql" + join(vector<string>(txs.size(), "?"), ",") + R"sql( ) )sql";

            TryTransactionStep(__func__, [&]()
            {
                auto stmt = SetupSqlStatement(sql);

                size_t i = 1;
                for (auto& tx : txs)
                    TryBindStatementText(stmt, i++, tx.first);

                while (sqlite3_step(*stmt) == SQLITE_ROW)
                {
                    if (auto [ok0, hash] = TryGetColumnString(*stmt, 0); ok0)
                    {
                        UniValue txInp(UniValue::VOBJ);
                        txInp.pushKV("txid", hash);
                        if (auto [ok, value] = TryGetColumnString(*stmt, 1); ok) txInp.pushKV("txid", value);
                        if (auto [ok, value] = TryGetColumnInt(*stmt, 2); ok) txInp.pushKV("vout", value);
                        if (auto [ok, value] = TryGetColumnString(*stmt, 3); ok) txInp.pushKV("address", value);
                        if (auto [ok, value] = TryGetColumnInt64(*stmt, 4); ok) txInp.pushKV("value", value / 100000000.0);

                        get<1>(txs[hash]).push_back(txInp);
                    }
                }

                FinalizeSqlStatement(*stmt);
            });
        }

        for (auto& ftx : txs)
        {
            auto[tx, vin, vout] = ftx.second;
            tx.pushKV("vin", vin);
            tx.pushKV("vout", vout);
            
            result.push_back(tx);
        }

        return result;
    }

    UniValue ExplorerRepository::GetAddressTransactions(const string& address, int pageInitBlock, int pageStart, int pageSize)
    {
        return _getTransactions([&](shared_ptr<sqlite3_stmt*>& stmt)
        {
            stmt = SetupSqlStatement(R"sql(
                select t.Hash, ptxs.RowNum, t.Type, t.Height, t.BlockHash, t.Time, o.Number, json_group_array(o.AddressHash), o.Value, o.ScriptPubKey
                from (
                    select ROW_NUMBER() OVER (order by txs.TxHeight desc, txs.TxHash asc) RowNum, txs.TxHash
                    from (
                        select distinct o.TxHash, o.TxHeight
                        from TxOutputs o
                        where o.AddressHash = ?
                          and o.SpentHeight is not null
                          and o.TxHeight <= ?
                    ) txs
                ) ptxs
                join TxOutputs o on o.TxHash = ptxs.TxHash
                join Transactions t on t.Hash = ptxs.TxHash
                where RowNum >= ? and RowNum < ?
                group by t.Hash, o.Number
            )sql");

            TryBindStatementText(stmt, 1, address);
            TryBindStatementInt(stmt, 2, pageInitBlock);
            TryBindStatementInt(stmt, 3, pageStart);
            TryBindStatementInt(stmt, 4, pageStart + pageSize);
        });
    }

    UniValue ExplorerRepository::GetBlockTransactions(const string& blockHash, int pageStart, int pageSize)
    {
        return _getTransactions([&](shared_ptr<sqlite3_stmt*>& stmt)
        {
            stmt = SetupSqlStatement(R"sql(
                select ptxs.Hash, ptxs.RowNum, ptxs.Type, ptxs.Height, ptxs.BlockHash, ptxs.Time, o.Number, json_group_array(o.AddressHash), o.Value, o.ScriptPubKey
                from (
                    select ROW_NUMBER() OVER (order by txs.BlockNum asc) RowNum, txs.Hash, txs.Type, txs.Height, txs.BlockHash, txs.Time
                    from (
                        select t.Hash, t.Type, t.Height, t.BlockNum, t.BlockHash, t.Time
                        from Transactions t indexed by Transactions_BlockHash
                        where t.BlockHash = ?
                    ) txs
                ) ptxs
                join TxOutputs o on o.TxHash = ptxs.Hash
                where RowNum >= ? and RowNum < ?
                group by ptxs.Hash, o.Number
            )sql");

            TryBindStatementText(stmt, 1, blockHash);
            TryBindStatementInt(stmt, 2, pageStart);
            TryBindStatementInt(stmt, 3, pageStart + pageSize);
        });
    }
    
    UniValue ExplorerRepository::GetTransactions(const vector<string>& transactions, int pageStart, int pageSize)
    {
        return _getTransactions([&](shared_ptr<sqlite3_stmt*>& stmt)
        {
            stmt = SetupSqlStatement(R"sql(
                select ptxs.Hash, ptxs.RowNum, ptxs.Type, ptxs.Height, ptxs.BlockHash, ptxs.Time, o.Number, json_group_array(o.AddressHash), o.Value, o.ScriptPubKey
                from (
                    select ROW_NUMBER() OVER (order by txs.BlockNum asc) RowNum, txs.Hash, txs.Type, txs.Height, txs.BlockHash, txs.Time
                    from (
                        select t.Hash, t.Type, t.Height, t.BlockNum, t.BlockHash, t.Time
                        from Transactions t
                        where t.Hash in (
                            )sql" + join(vector<string>(transactions.size(), "?"), ",") + R"sql(
                        )
                    ) txs
                ) ptxs
                join TxOutputs o on o.TxHash = ptxs.Hash
                where RowNum >= ? and RowNum < ?
                group by ptxs.Hash, o.Number
            )sql");

            size_t i = 1;
            for (auto& tx : transactions)
                TryBindStatementText(stmt, i++, tx);

            TryBindStatementInt(stmt, i++, pageStart);
            TryBindStatementInt(stmt, i, pageStart + pageSize);
        });
    }

}