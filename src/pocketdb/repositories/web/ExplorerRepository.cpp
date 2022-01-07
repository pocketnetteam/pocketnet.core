// Copyright (c) 2018-2021 Pocketnet developers
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

    UniValue ExplorerRepository::GetTransactionsStatisticByHours(int topHeight, int depth)
    {
        UniValue result(UniValue::VOBJ);

        TryTransactionStep(__func__, [&]()
        {
            auto stmt = SetupSqlStatement(R"sql(
                select (t.Height / 60)Hour, t.Type, count()Count
                from Transactions t indexed by Transactions_Type_HeightByHour
                where t.Type in (1,100,103,200,201,204,205,300,301,302,303)
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
                where t.Type in (1,100,103,200,201,204,205,300,301,302,303)
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

    UniValue ExplorerRepository::GetContentStatistic()
    {
        UniValue result(UniValue::VOBJ);

        TryTransactionStep(__func__, [&]()
        {
            auto stmt = SetupSqlStatement(R"sql(
                select t.Type, count()Count
                from Transactions t indexed by Transactions_Type_Last_Height_Id
                where t.Type in (100,101,102,200,201)
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

    template<typename T>
    UniValue ExplorerRepository::_getTransactions(T stmtOut)
    {
        auto func = __func__;
        UniValue result(UniValue::VARR);
        map<string, tuple<UniValue, UniValue, UniValue>> txs;

        // Select outputs
        {
            TryTransactionStep(func, [&]()
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
                        from TxOutputs o indexed by TxOutputs_AddressHash_TxHeight_SpentHeight
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