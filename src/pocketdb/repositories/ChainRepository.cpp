// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include "pocketdb/repositories/ChainRepository.h"

namespace PocketDb
{
    void ChainRepository::IndexBlock(const string& blockHash, int height, vector<TransactionIndexingInfo>& txs)
    {
        struct ChainData
        {
            const TransactionIndexingInfo indexingInfo;
            const optional<int64_t> lastTxId; 
            const optional<int64_t> id;
        };

        std::vector<ChainData> chainData;
        for (const auto& txInfo: txs)
        {
            // Account and Content must have unique ID
            // Also all edited transactions must have Last=(0/1) field
            optional<int64_t> id;
            optional<int64_t> lastTxId;
            if (txInfo.IsAccount())
                tie(id, lastTxId) = IndexAccount(txInfo.Hash);

            if (txInfo.IsAccountSetting())
                tie(id, lastTxId) = IndexAccountSetting(txInfo.Hash);

            if (txInfo.IsContent())
                tie(id, lastTxId) = IndexContent(txInfo.Hash);

            if (txInfo.IsComment())
                tie(id, lastTxId) = IndexComment(txInfo.Hash);

            if (txInfo.IsBlocking())
                tie(id, lastTxId) = IndexBlocking(txInfo.Hash);

            if (txInfo.IsSubscribe())
                tie(id, lastTxId) = IndexSubscribe(txInfo.Hash);

            ChainData data {txInfo, lastTxId, id};
            chainData.emplace_back(data);
        }

        TryTransactionStep(__func__, [&]()
        {
            int64_t nTime1 = GetTimeMicros();

            IndexBlockData(blockHash);

            // Each transaction is processed individually
            for (const auto& txInfo : chainData)
            {
                // The outputs are needed for the explorer
                // TODO (aok) (v0.20.19+): replace with update inputs spent with TxInputs table over loop
                // UpdateTransactionOutputs(txInfo, height);

                // Calculate and save fee for future selects
                if (txInfo.indexingInfo.IsBoostContent())
                    IndexBoostContent(txInfo.indexingInfo.Hash);

                if (txInfo.lastTxId) ClearOldLast(*txInfo.lastTxId);

                // All transactions must have a blockHash & height relation
                InsertTransactionChainData(blockHash, txInfo.indexingInfo.BlockNumber, height, txInfo.indexingInfo.Hash, txInfo.id, txInfo.lastTxId.has_value());
            }

            int64_t nTime2 = GetTimeMicros();

            // After set height and mark inputs as spent we need recalculcate balances
            IndexBalances(height);

            int64_t nTime3 = GetTimeMicros();

            LogPrint(BCLog::BENCH, "    - IndexBlock: %.2fms + %.2fms = %.2fms\n",
                0.001 * double(nTime2 - nTime1),
                0.001 * double(nTime3 - nTime2),
                0.001 * double(nTime3 - nTime1)
            );
        });
    }

    tuple<bool, bool> ChainRepository::ExistsBlock(const string& blockHash, int height)
    {
        bool exists = false;
        bool last = true;

        string sql = R"sql(
            select
                ifnull((select 1 from Chain c where c.BlockId = (select r.RowId from Registry r where r.String = ?) and c.Height = ? limit 1), 0)current,
                ifnull((select 1 from Chain where Height = ? limit 1), 0)next
        )sql";

        TryTransactionStep(__func__, [&]()
        {
            auto stmt = SetupSqlStatement(sql);
            TryBindStatementText(stmt, 1, blockHash);
            TryBindStatementInt(stmt, 2, height);
            TryBindStatementInt(stmt, 3, height + 1);

            if (sqlite3_step(*stmt) == SQLITE_ROW)
            {
                if (auto[ok, value] = TryGetColumnInt(*stmt, 0); ok && value == 1)
                    exists = true;

                if (auto[ok, value] = TryGetColumnInt(*stmt, 1); ok && value == 1)
                    last = false;
            }

            FinalizeSqlStatement(*stmt);
        });

        return {exists, last};
    }

    void ChainRepository::IndexBlockData(const std::string& blockHash)
    {
        auto stmt = SetupSqlStatement(R"sql(
            INSERT OR IGNORE INTO Registry (String)
            VALUES (?)
        )sql");

        TryBindStatementText(stmt, 1, blockHash);

        TryStepStatement(stmt);
    }

    void ChainRepository::InsertTransactionChainData(const string& blockHash, int blockNumber, int height, const string& txHash, const optional<int64_t>& id, bool fIsCreateLast)
    {
        auto stmt = SetupSqlStatement(R"sql(
            -- TODO (losty-db): use WITH statement for TxId
            with t as (
                select RowId from Transactions where
                    HashId = (select RowId from Registry where String = ?)
            )
            INSERT OR FAIL INTO Chain (TxId, BlockId, BlockNum, Height, Uid)
            select (select t.RowId from t), (select RowId from Registry where String = ?), ?, ?, ?
            where not exists (select 1 from Chain,t where TxId = (t.RowId )) and (select t.RowId > 0 from t)
        )sql");
        TryBindStatementText(stmt, 1, txHash);
        TryBindStatementText(stmt, 2, blockHash);
        TryBindStatementInt(stmt, 3, blockNumber);
        TryBindStatementInt(stmt, 4, height);
        if (id) TryBindStatementInt64(stmt, 5, *id);
        TryStepStatement(stmt);

        if (fIsCreateLast) {
            auto stmtInsertLast = SetupSqlStatement(R"sql(
                INSERT INTO Last (TxId)
                select RowId from Transactions where HashId = (select RowId from Registry where String = ?)
            )sql");
            TryBindStatementText(stmtInsertLast, 1, txHash);
            TryStepStatement(stmtInsertLast);
        }
    }

    void ChainRepository::UpdateTransactionOutputs(const TransactionIndexingInfo& txInfo, int height)
    {
        for (auto& input : txInfo.Inputs)
        {
            auto stmt = SetupSqlStatement(R"sql(
                UPDATE TxOutputs SET
                    SpentHeight = ?,
                    SpentTxId = (select RowId from Transactions where HashId = (select RowId from Registry where String = ?))
                WHERE TxId = ? and Number = ?
            )sql");

            TryBindStatementInt(stmt, 1, height);
            TryBindStatementText(stmt, 2, txInfo.Hash);
            TryBindStatementText(stmt, 3, input.first);
            TryBindStatementInt(stmt, 4, input.second);
            TryStepStatement(stmt);
        }
    }


    void ChainRepository::IndexBalances(int height)
    {
        // Generate new balance records
        auto stmt = SetupSqlStatement(R"sql(
            insert into Balances (AddressId, Last, Height, Value)
            select
                saldo.AddressId,
                1,
                ?,
                sum(ifnull(saldo.Amount,0)) + ifnull(b.Value,0)
            from (

                select 'unspent',
                       o.AddressId,
                       sum(o.Value)Amount
                -- TODO (losty-db): fix me
                from TxOutputs o
                join Chain c on c.TxId = o.TxId and c.Height = ?
                group by o.AddressId

                union

                select 'spent',
                       o.AddressId,
                       -sum(o.Value)Amount
                from TxInputs i
                join Chain ci on ci.TxId = i.SpentTxId and ci.Height = ?
                join TxOutputs o on o.TxId = i.TxId and o.Number = i.Number
                group by o.AddressId

            ) saldo
            left join Balances b indexed by Balances_AddressId_Last
                on b.AddressId = saldo.AddressId and b.Last = 1
            where saldo.AddressId is not null
            group by saldo.AddressId
        )sql");
        TryBindStatementInt(stmt, 1, height);
        TryBindStatementInt(stmt, 2, height);
        TryBindStatementInt(stmt, 3, height);
        TryStepStatement(stmt);

        // Remove old Last records
        auto stmtOld = SetupSqlStatement(R"sql(
            update Balances indexed by Balances_AddressId_Last_Height
              set Last = 0
            where Balances.Last = 1
              and Balances.Height < ?
              and Balances.AddressId in (
                select b.AddressId
                from Balances b indexed by Balances_Height
                where b.Height = ?
              )
        )sql");
        TryBindStatementInt(stmtOld, 1, height);
        TryBindStatementInt(stmtOld, 2, height);
        TryStepStatement(stmtOld);
    }

    // <id, lastTxID>
    pair<int64_t, int64_t> ChainRepository::IndexAccount(const string& txHash)
    {
        // Get new ID or copy previous
        auto sql = string(R"sql(
            with l as (
                select b.RowId
                from Transactions a -- TODO (losty-db): index
                join Transactions b
                    on b.Type in (100, 170)
                    and b.Int1 = a.Int1
                join Last l
                    on l.TxId = b.RowId
                where a.Type in (100,170)
                    and a.RowId = (select RowId from Transactions where HashId = (select RowId from Registry where String = ?))
                limit 1
            )
            select ifnull(
                (select c.Uid from Chain c,l where c.TxId = l.RowId),
                ifnull(
                    -- new record
                    (
                        select max( c.Uid ) + 1
                        from Chain c -- TODO (losty-db): index
                    ),
                    (select 0) -- for first record
                )
           ), l.RowId from l
        )sql");

        int64_t id = 0;
        int64_t lastTxId = -1;
        TryTransactionStep(__func__, [&]()
        {
            auto stmt = SetupSqlStatement(sql);
            TryBindStatementText(stmt, 1, txHash);
            
            if (sqlite3_step(*stmt) == SQLITE_ROW) {
                if (auto [ok, val] = TryGetColumnInt64(*stmt, 0); ok) id = val;
                if (auto [ok, val] = TryGetColumnInt64(*stmt, 1); ok) lastTxId = val;
            }
            // TODO (losty-db): error
        });

        return {id, lastTxId};
    }

    pair<int64_t, int64_t> ChainRepository::IndexAccountSetting(const string& txHash)
    {
        // Get new ID or copy previous
        auto sql = string(R"sql(
            with l as (
                select b.RowId
                from Transactions a -- TODO (losty-db): index
                join Transactions b
                    on b.Type in (103)
                    and b.Int1 = a.Int1
                join Last l
                    on l.TxId = b.RowId
                where a.Type in (103)
                    and a.RowId = (select RowId from Transactions where HashId = (select RowId from Registry where String = ?))
                limit 1
            )
            select ifnull(
                        (select c.Uid from Chain c,l where c.TxId = l.RowId),
                        ifnull(
                            -- new record
                            (
                                select max( c.Uid ) + 1
                                from Chain c -- TODO (losty-db): index
                            ),
                            (select 0) -- for first record
                        )
                ), l.RowId from l
        )sql");

        int64_t id = 0;
        int64_t lastTxId = -1;
        TryTransactionStep(__func__, [&]()
        {
            auto stmt = SetupSqlStatement(sql);
            TryBindStatementText(stmt, 1, txHash);
            
            if (sqlite3_step(*stmt) == SQLITE_ROW) {
                if (auto [ok, val] = TryGetColumnInt64(*stmt, 0); ok) id = val;
                if (auto [ok, val] = TryGetColumnInt64(*stmt, 1); ok) lastTxId = val;
            }
            // TODO (losty-db): error
        });

        return {id, lastTxId};
    }

    pair<int64_t, int64_t> ChainRepository::IndexContent(const string& txHash)
    {
        // Get new ID or copy previous
        auto sql = string(R"sql(
            with l as (
                select b.RowId
                from Transactions a -- TODO (losty-db): index
                join Transactions b
                    on b.Type in (200,201,202,209,210,207)
                    and b.RegId2 = a.RegId2
                join Last l
                    on l.TxId = b.RowId
                where a.Type in (200,201,202,209,210,207)
                    and a.RowId = (select RowId from Transactions where HashId = (select RowId from Registry where String = ?))
                limit 1
            )
            select ifnull(
                        (select c.Uid from Chain c,l where c.TxId = l.RowId),
                        ifnull(
                            -- new record
                            (
                                select max( c.Uid ) + 1
                                from Chain c -- TODO (losty-db): index
                            ),
                            (select 0) -- for first record
                        )
                ), l.RowId from l
        )sql");

        int64_t id = 0;
        int64_t lastTxId = -1;
        TryTransactionStep(__func__, [&]()
        {
            auto stmt = SetupSqlStatement(sql);
            TryBindStatementText(stmt, 1, txHash);
            
            if (sqlite3_step(*stmt) == SQLITE_ROW) {
                if (auto [ok, val] = TryGetColumnInt64(*stmt, 0); ok) id = val;
                if (auto [ok, val] = TryGetColumnInt64(*stmt, 1); ok) lastTxId = val;
            }
            // TODO (losty-db): error
        });

        return {id, lastTxId};
    }

    pair<int64_t, int64_t> ChainRepository::IndexComment(const string& txHash)
    {
        // Get new ID or copy previous
        auto sql = string(R"sql(
            with l as (
                select b.RowId
                from Transactions a -- TODO (losty-db): index
                join Transactions b
                    on b.Type in (204,205,206)
                    and b.RegId2 = a.RegId2
                join Last l
                    on l.TxId = b.RowId
                where a.Type in (204,205,206)
                    and a.RowId = (select RowId from Transactions where HashId = (select RowId from Registry where String = ?))
                limit 1
            )
            select ifnull(
                        (select c.Uid from Chain c,l where c.TxId = l.RowId),
                        ifnull(
                            -- new record
                            (
                                select max( c.Uid ) + 1
                                from Chain c -- TODO (losty-db): index
                            ),
                            (select 0) -- for first record
                        )
                ), l.RowId from l
        )sql");

        int64_t id = 0;
        int64_t lastTxId = -1;
        TryTransactionStep(__func__, [&]()
        {
            auto stmt = SetupSqlStatement(sql);
            TryBindStatementText(stmt, 1, txHash);
            
            if (sqlite3_step(*stmt) == SQLITE_ROW) {
                if (auto [ok, val] = TryGetColumnInt64(*stmt, 0); ok) id = val;
                if (auto [ok, val] = TryGetColumnInt64(*stmt, 1); ok) lastTxId = val;
            }
            // TODO (losty-db): error
        });

        return {id, lastTxId};
    }

    pair<int64_t, int64_t> ChainRepository::IndexBlocking(const string& txHash)
    {
        // TODO (o1q): double check multiple locks
        // Set last=1 for new transaction
         auto sql = string(R"sql(
            with l as (
                select b.RowId
                from Transactions a -- TODO (losty-db): index
                join Transactions b
                    on b.Type in (305,306)
                    and b.Int1 = a.Int1
                    and ifnull(b.RegId2,-1) = ifnull(a.RegId2,-1)
                    and ifnull(b.RegId3,-1) = ifnull(a.RegId3,-1)
                join Last l
                    on l.TxId = b.RowId
                where a.Type in (305,306)
                    and a.RowId = (select RowId from Transactions where HashId = (select RowId from Registry where String = ?))
                limit 1
            )
            select ifnull(
                        (select c.Uid from Chain c,l where c.TxId = l.RowId),
                        ifnull(
                            -- new record
                            (
                                select max( c.Uid ) + 1
                                from Chain c -- TODO (losty-db): index
                            ),
                            (select 0) -- for first record
                        )
                ), l.RowId from l
        )sql");

        int64_t id = 0;
        int64_t lastTxId = -1;
        TryTransactionStep(__func__, [&]()
        {
            auto stmt = SetupSqlStatement(sql);
            TryBindStatementText(stmt, 1, txHash);
            
            if (sqlite3_step(*stmt) == SQLITE_ROW) {
                if (auto [ok, val] = TryGetColumnInt64(*stmt, 0); ok) id = val;
                if (auto [ok, val] = TryGetColumnInt64(*stmt, 1); ok) lastTxId = val;
            }
            // TODO (losty-db): error
        });

        // TODO (losty-db): bad bad bad!!!
        TryTransactionStep(__func__, [&]()
        {
            auto insListStmt = SetupSqlStatement(R"sql(
                insert into BlockingLists (IdSource, IdTarget)
                select
                usc.Id,
                utc.Id
                from Transactions b -- TODO (losty-db): index
                join Transactions us -- TODO (losty-db): index
                on us.Type in (100, 170) and us.Int1 = b.Int1
                join Chain usc -- TODO (losty-db): index
                on usc.TxId = us.Id
                    and usc.Last = 1
                join Transactions ut -- TODO (losty-db): index
                on ut.Type in (100, 170)
                    and ut.Int1 in (select b.Int2 union select cast(value as int) from json_each(b.Int3))
                join Chain utc -- TODO (losty-db): index
                on utc.TxId = ut.Id
                    and utc.Last = 1
                where b.Type in (305) and b.Id = (select RowId from Transactions where HashId = (select RowId from Registry where String = ?))
                    and not exists (select 1 from BlockingLists bl where bl.IdSource = usc.Id and bl.IdTarget = utc.Id)
            )sql");
            TryBindStatementText(insListStmt, 1, txHash);
            TryStepStatement(insListStmt);

            auto delListStmt = SetupSqlStatement(R"sql(
                delete from BlockingLists
                where exists
                (select
                1
                from Transactions b -- TODO (losty-db): index
                join Transactions us -- TODO (losty-db): index
                on us.Type in (100, 170) and us.Int1 = b.Int1
                join Chain usc -- TODO (losty-db): index
                on usc.TxId = us.Id
                    and usc.Last = 1
                    and usc.Id = BlockingLists.IdSource
                join Transactions ut -- TODO (losty-db): index
                on ut.Type in (100, 170) and ut.Int1 = b.Int2
                join Chain utc -- TODO (losty-db): index
                on utc.TxId = ut.Id
                    and utc.Last = 1
                    and utc.Id = BlockingLists.IdTarget
                where b.Type in (306) and b.Id = (select RowId from Transactions where HashId = (select RowId from Registry where String = ?))
                )
            )sql");
            TryBindStatementText(delListStmt, 1, txHash);
            TryStepStatement(delListStmt);
        });

        // Clear old last records for set new last
        return {id, lastTxId};
    }

    pair<int64_t, int64_t> ChainRepository::IndexSubscribe(const string& txHash)
    {
         // Get new ID or copy previous
        auto sql = string(R"sql(
            with l as (
                select b.RowId
                from Transactions a -- TODO (losty-db): index
                join Transactions b
                    on b.Type in (302,303,304)
                    and b.RegId1 = a.RegId1
                    and b.RegId2 = a.RegId2
                join Last l
                    on l.TxId = b.RowId
                where a.Type in (302,303,304)
                    and a.RowId = (select RowId from Transactions where HashId = (select RowId from Registry where String = ?))
                limit 1
            )
            select ifnull(
                        (select c.Uid from Chain c,l where c.TxId = l.RowId),
                        ifnull(
                            -- new record
                            (
                                select max( c.Uid ) + 1
                                from Chain c -- TODO (losty-db): index
                            ),
                            (select 0) -- for first record
                        )
                ), l.RowId from l
        )sql");

        int64_t id = 0;
        int64_t lastTxId = -1;
        TryTransactionStep(__func__, [&]()
        {
            auto stmt = SetupSqlStatement(sql);
            TryBindStatementText(stmt, 1, txHash);
            
            if (sqlite3_step(*stmt) == SQLITE_ROW) {
                if (auto [ok, val] = TryGetColumnInt64(*stmt, 0); ok) id = val;
                if (auto [ok, val] = TryGetColumnInt64(*stmt, 1); ok) lastTxId = val;
            }
            // TODO (losty-db): error
        });

        return {id, lastTxId};
    }
    
    void ChainRepository::IndexBoostContent(const string& txHash)
    {
        // TODO (losty-db): calculate this before inserting tx data to db
        // Set transaction fee
        auto stmt = SetupSqlStatement(R"sql(
            update Transactions
            set Int1 =
              (
                (
                  select sum(oo.Value)
                  from TxInputs i -- TODO (losty-db): index
                  join TxOutputs oo -- TODO (losty-db): index
                    on oo.TxId = i.TxId
                    and oo.Number = i.Number
                  where i.SpentTxId = Transactions.RowId
                ) - (
                  select sum(o.Value)
                  from TxOutputs o -- TODO (losty-db): index
                  where TxId = Transactions.RowId
                )
              )
            where Transactions.RowId = (select RowId from Transactions where HashId = (select RowId from Registry where String = ?))
              and Transactions.Type in (208)
        )sql");
        TryBindStatementText(stmt, 1, txHash);
        TryStepStatement(stmt);
    }


    bool ChainRepository::ClearDatabase()
    {
        LogPrintf("Full reindexing database..\n");

        LogPrintf("Deleting database indexes..\n");
        m_database.DropIndexes();

        LogPrintf("Rollback to first block..\n");
        RollbackHeight(0);
        ClearBlockingList();

        m_database.CreateStructure();

        return true;
    }

    bool ChainRepository::Rollback(int height)
    {
        try
        {
            // Update transactions
            TryTransactionStep(__func__, [&]()
            {
                RestoreOldLast(height);
                RollbackBlockingList(height);
                RollbackHeight(height);
            });

            return true;
        }
        catch (std::exception& ex)
        {
            LogPrintf("Error: Rollback to height %d failed with message: %s\n", height, ex.what());
            return false;
        }
    }
    
    void ChainRepository::ClearOldLast(const int64_t& lastTxId)
    {
        auto stmt = SetupSqlStatement(R"sql(
            delete from Last
            where TxId = ?
        )sql");

        TryBindStatementInt64(stmt, 1, lastTxId);
        TryStepStatement(stmt);
    }

    void ChainRepository::RestoreOldLast(int height)
    {
        int64_t nTime0 = GetTimeMicros();

        // ----------------------------------------
        // Restore old Last transactions
        // TODO (losty-critical): validate!!!
        auto stmt1 = SetupSqlStatement(R"sql(
            insert into Last (TxId) -- TODO (losty-db): index
            select TxId from (
                select c.TxId, max(c.Height)Height
                from Chain c
                where c.Uid > 0 and not exists (select 1 from Last l where l.TxId = c.TxId)
                group by c.Uid
            )
        )sql");
        TryStepStatement(stmt1);

        int64_t nTime1 = GetTimeMicros();
        LogPrint(BCLog::BENCH, "        - RestoreOldLast (Transactions): %.2fms\n", 0.001 * (nTime1 - nTime0));

        // ----------------------------------------
        // Restore Last for deleting ratings
        auto stmt2 = SetupSqlStatement(R"sql(
            update Ratings indexed by Ratings_Type_Uid_Height_Value
                set Last=1
            from (
                select r1.Type, r1.Uid, max(r2.Height)Height
                from Ratings r1 indexed by Ratings_Type_Uid_Last_Height
                join Ratings r2 indexed by Ratings_Type_Uid_Last_Height on r2.Type = r1.Type and r2.Uid = r1.Uid and r2.Last = 0 and r2.Height < ?
                where r1.Height >= ?
                  and r1.Last = 1
                group by r1.Type, r1.Uid
            )r
            where Ratings.Type = r.Type
              and Ratings.Uid = r.Uid
              and Ratings.Height = r.Height
        )sql");
        TryBindStatementInt(stmt2, 1, height);
        TryBindStatementInt(stmt2, 2, height);
        TryStepStatement(stmt2);

        int64_t nTime2 = GetTimeMicros();
        LogPrint(BCLog::BENCH, "        - RestoreOldLast (Ratings): %.2fms\n", 0.001 * (nTime2 - nTime1));

        // ----------------------------------------
        // Restore Last for deleting balances
        auto stmt3 = SetupSqlStatement(R"sql(
            update Balances set

                Last = 1

            from (
                select

                    b1.AddressId
                    ,(
                        select max(b2.Height)
                        from Balances b2 indexed by Balances_AddressId_Last_Height
                        where b2.AddressId = b1.AddressId
                          and b2.Last = 0
                          and b2.Height < ?
                        limit 1
                    )Height

                from Balances b1 indexed by Balances_Height

                where b1.Height >= ?
                  and b1.Last = 1
                  and b1.AddressId > 0

                group by b1.AddressId
            )b
            where b.Height is not null
              and Balances.AddressId = b.AddressId
              and Balances.Height = b.Height
        )sql");
        TryBindStatementInt(stmt3, 1, height);
        TryBindStatementInt(stmt3, 2, height);
        TryStepStatement(stmt3);

        int64_t nTime3 = GetTimeMicros();
        LogPrint(BCLog::BENCH, "        - RestoreOldLast (Balances): %.2fms\n", 0.001 * (nTime3 - nTime2));
    }

    void ChainRepository::RollbackHeight(int height)
    {
        int64_t nTime0 = GetTimeMicros();

        auto stmt0 = SetupSqlStatement(R"sql(
            delete from Last
            where TxId in (select c.TxId from Chain c where c.Height > ?)
        )sql");
        TryBindStatementInt64(stmt0, 1, height);
        TryStepStatement(stmt0);
        // ----------------------------------------
        // Rollback general transaction information
        auto stmt1 = SetupSqlStatement(R"sql(
            delete from Chain
            WHERE Height >= ?
        )sql");
        TryBindStatementInt64(stmt1, 1, height);
        TryStepStatement(stmt1);

        int64_t nTime1 = GetTimeMicros();
        LogPrint(BCLog::BENCH, "        - RollbackHeight (Chain:Height = null): %.2fms\n", 0.001 * (nTime1 - nTime0));

        // TODO (losty-db): delete blocks from db?
        // auto stmt1 = SetupSqlStatement(R"sql(
        //     delete from Blocks
        //     where Height >= ?
        // )sql");
        // TryBindStatementInt(stmt1, 1, height);
        // TryStepStatement(stmt1);

        int64_t nTime2 = GetTimeMicros();
        // LogPrint(BCLog::BENCH, "        - RollbackHeight (Blocks:Height = null): %.2fms\n", 0.001 * (nTime2 - nTime1));

        // ----------------------------------------
        // Rollback spent transaction outputs

        // auto stmt2 = SetupSqlStatement(R"sql(
        //     UPDATE TxOutputs SET
        //         SpentHeight = null,
        //         SpentTxId = null
        //     WHERE SpentHeight >= ?
        // )sql");
        // TryBindStatementInt(stmt2, 1, height);
        // TryStepStatement(stmt2);

        int64_t nTime3 = GetTimeMicros();
        // LogPrint(BCLog::BENCH, "        - RollbackHeight (TxOutputs:SpentHeight = null): %.2fms\n", 0.001 * (nTime3 - nTime2));

        // ----------------------------------------
        // Rollback transaction outputs height

        // auto stmt3 = SetupSqlStatement(R"sql(
        //     UPDATE TxOutputs SET
        //         TxHeight = null
        //     WHERE TxHeight >= ?
        // )sql");
        // TryBindStatementInt(stmt3, 1, height);
        // TryStepStatement(stmt3);

        int64_t nTime4 = GetTimeMicros();
        // LogPrint(BCLog::BENCH, "        - RollbackHeight (TxOutputs:TxHeight = null): %.2fms\n", 0.001 * (nTime4 - nTime3));

        // ----------------------------------------
        // Remove ratings
        auto stmt4 = SetupSqlStatement(R"sql(
            delete from Ratings
            where Height >= ?
        )sql");
        TryBindStatementInt(stmt4, 1, height);
        TryStepStatement(stmt4);

        int64_t nTime5 = GetTimeMicros();
        LogPrint(BCLog::BENCH, "        - RollbackHeight (Ratings delete): %.2fms\n", 0.001 * (nTime5 - nTime4));

        // ----------------------------------------
        // Remove balances
        auto stmt5 = SetupSqlStatement(R"sql(
            delete from Balances
            where Height >= ?
        )sql");
        TryBindStatementInt(stmt5, 1, height);
        TryStepStatement(stmt5);

        int64_t nTime6 = GetTimeMicros();
        LogPrint(BCLog::BENCH, "        - RollbackHeight (Balances delete): %.2fms\n", 0.001 * (nTime6 - nTime5));
    }

    void ChainRepository::RollbackBlockingList(int height)
    {
        int64_t nTime0 = GetTimeMicros();

        auto delListStmt = SetupSqlStatement(R"sql(
            delete from BlockingLists where ROWID in
            (
                select bl.ROWID
                from Transactions b
                join Chain bc
                  on bc.TxId =  b.RowId
                    and bc.Height >= ?
                join Transactions us
                  on us.Type in (100, 170)
                    and us.RegId1 = b.RegId1
                join Chain usc
                  on usc.TxId = us.RowId
                join Last usl
                  on usl.TxId = us.RowId
                join Transactions ut
                  on ut.Type in (100, 170)
                    and ut.Int1 in (select b.RegId2 union select l.RegId from Lists l where l.TxId = b.RowId)
                join Chain utc
                  on utc.TxId = ut.RowId
                join Last utl
                  on utl.TxId = ut.RowId
                join BlockingLists bl on bl.IdSource = usc.Uid and bl.IdTarget = utc.Uid
                where b.Type in (305)
            )
        )sql");
        TryBindStatementInt(delListStmt, 1, height);
        TryStepStatement(delListStmt);
        
        int64_t nTime1 = GetTimeMicros();
        LogPrint(BCLog::BENCH, "        - RollbackList (Delete blocking list): %.2fms\n", 0.001 * (nTime1 - nTime0));

        auto insListStmt = SetupSqlStatement(R"sql(
            insert into BlockingLists
            (
                IdSource,
                IdTarget
            )
            select distinct
              usc.Uid,
              utc.Uid
            from Transactions b
            join Chain bc
              on bc.TxId = b.RowId
                and bc.Height >= ?
            join Transactions us
              on us.Type in (100, 170) and us.Int1 = b.Int1
            join Chain usc
              on usc.TxId = us.RowId
            join Last usl
                on usl.TxId = us.RowId
            join Transactions ut
              on ut.Type in (100, 170)
                --and ut.String1 = b.String2
                and ut.Int1 in (select b.RegId2 union select l.RegId from Lists l where l.TxId = b.RowId)
            join Chain utc
              on utc.TxId = ut.RowId
            join Last utl
              on utl.TxId = ut.RowId
            where b.Type in (306)
              and not exists (select 1 from BlockingLists bl where bl.IdSource = usc.Uid and bl.IdTarget = utc.Uid)
        )sql");
        TryBindStatementInt(insListStmt, 1, height);
        TryStepStatement(insListStmt);
        
        int64_t nTime2 = GetTimeMicros();
        LogPrint(BCLog::BENCH, "        - RollbackList (Insert blocking list): %.2fms\n", 0.001 * (nTime2 - nTime1));
    }

    void ChainRepository::ClearBlockingList()
    {
        int64_t nTime0 = GetTimeMicros();

        auto stmt = SetupSqlStatement(R"sql(
            delete from BlockingLists
        )sql");
        TryStepStatement(stmt);
        
        int64_t nTime1 = GetTimeMicros();
        LogPrint(BCLog::BENCH, "        - ClearBlockingList (Delete blocking list): %.2fms\n", 0.001 * (nTime1 - nTime0));
    }


} // namespace PocketDb
