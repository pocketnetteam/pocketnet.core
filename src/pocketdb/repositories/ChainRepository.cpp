// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include "pocketdb/repositories/ChainRepository.h"

namespace PocketDb
{
    void ChainRepository::IndexBlock(const string& blockHash, int height, vector<TransactionIndexingInfo>& txs)
    {
        TryTransactionStep(__func__, [&]()
        {
            int64_t nTime1 = GetTimeMicros();

            // Each transaction is processed individually
            for (const auto& txInfo : txs)
            {

                auto blockId = UpdateBlockData(blockHash, height);
                if (blockId == -1) {
                    // TODO (losty-db): error?
                }
                // All transactions must have a blockHash & height relation
                UpdateTransactionChainData(blockId, txInfo.BlockNumber, height, txInfo.TxId);

                // The outputs are needed for the explorer
                // TODO (aok) (v0.20.19+): replace with update inputs spent with TxInputs table over loop
                UpdateTransactionOutputs(txInfo, height);

                // Account and Content must have unique ID
                // Also all edited transactions must have Last=(0/1) field
                if (txInfo.IsAccount())
                    IndexAccount(txInfo.TxId);

                if (txInfo.IsAccountSetting())
                    IndexAccountSetting(txInfo.TxId);

                if (txInfo.IsContent())
                    IndexContent(txInfo.TxId);

                if (txInfo.IsComment())
                    IndexComment(txInfo.TxId);

                if (txInfo.IsBlocking())
                    IndexBlocking(txInfo.TxId);

                if (txInfo.IsSubscribe())
                    IndexSubscribe(txInfo.TxId);

                // Calculate and save fee for future selects
                if (txInfo.IsBoostContent())
                    IndexBoostContent(txInfo.TxId);
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
                ifnull((select 1 from Transactions where BlockHash = ? and Height = ? limit 1), 0)current,
                ifnull((select 1 from Transactions where Height = ? limit 1), 0)next
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

    int64_t ChainRepository::UpdateBlockData(const std::string& blockHash, int height)
    {
        auto sql = R"sql(
            INSERT OR IGNORE INTO Blocks (Hash, Height)
            VALUES (?, ?)
            RETURNING Id
        )sql";

        int64_t id = -1;
        TryTransactionStep(__func__, [&]()
        {
            auto stmt = SetupSqlStatement(sql);
            TryBindStatementText(stmt, 1, blockHash);
            TryBindStatementInt(stmt, 2, height);

            // TODO (losty-db): error
            if (sqlite3_step(*stmt) == SQLITE_ROW) {
                if (auto [ok, val] = TryGetColumnInt64(*stmt, 0); ok) id = val;
            }
        });

        auto stmt = SetupSqlStatement(sql);

    }

    void ChainRepository::UpdateTransactionChainData(const int64_t& blockId, int blockNumber, int height, const int64_t& txId)
    {
        auto stmt = SetupSqlStatement(R"sql(
            INSERT INTO Chain (TxId, BlockId, BlockNum)
            VALUES(?, ?, ?)
        )sql");
        TryBindStatementInt64(stmt, 1, txId);
        TryBindStatementInt64(stmt, 2, blockId);
        TryBindStatementInt(stmt, 3, blockNumber);
        TryStepStatement(stmt);


        // TODO (losty-db): can we remove duplicate of TxHeight in TxOutput?
        auto stmtOuts = SetupSqlStatement(R"sql(
            UPDATE TxOutputs SET
                TxHeight = ?
            WHERE TxId = ? and TxHeight is null
        )sql");
        TryBindStatementInt(stmtOuts, 1, height);
        TryBindStatementInt64(stmtOuts, 2, txId);
        TryStepStatement(stmtOuts);
    }

    void ChainRepository::UpdateTransactionOutputs(const TransactionIndexingInfo& txInfo, int height)
    {
        for (auto& input : txInfo.Inputs)
        {
            auto stmt = SetupSqlStatement(R"sql(
                UPDATE TxOutputs SET
                    SpentHeight = ?,
                    SpentTxId = ?
                WHERE TxId = ? and Number = ?
            )sql");

            TryBindStatementInt(stmt, 1, height);
            TryBindStatementInt64(stmt, 2, txInfo.TxId);
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
                from TxOutputs o indexed by TxOutputs_TxHeight_AddressId
                where  o.TxHeight = ?
                group by o.AddressId

                union

                select 'spent',
                       o.AddressId,
                       -sum(o.Value)Amount
                from TxOutputs o indexed by TxOutputs_SpentHeight_AddressId
                where o.SpentHeight = ?
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

    void ChainRepository::IndexAccount(const int64_t& txId)
    {
        // Get new ID or copy previous
        auto setIdStmt = SetupSqlStatement(R"sql(
            UPDATE Chain SET
                Id = ifnull(
                    -- copy self Id
                    (
                        select c.Id
                        from Transactions a -- TODO (losty-db): index
                        join Chain c
                            on c.TxId = a.Id
                            and c.Last = 1
                        where a.Type in (100,170)
                            and a.Int1 = Transactions.Int1
                        limit 1
                    ),
                    ifnull(
                        -- new record
                        (
                            select max( c.Id ) + 1
                            from Chain a indexed by Chain_Id
                        ),
                        0 -- for first record
                    )
                ),
                Last = 1
            WHERE TxId = ?
        )sql");
        TryBindStatementInt64(setIdStmt, 1, txId);
        TryStepStatement(setIdStmt);

        // Clear old last records for set new last
        ClearOldLast(txId);
    }

    void ChainRepository::IndexAccountSetting(const int64_t& txId)
    {
        // Get new ID or copy previous
        auto setIdStmt = SetupSqlStatement(R"sql(
            UPDATE Chain SET
                Id = ifnull(
                    -- copy self Id
                    (
                        select c.Id
                        from Transactions a -- TODO (losty-db): index
                        join Chain c
                            on c.TxId = a.Id
                            and c.Last = 1
                        where a.Type in (103)
                            and a.Last = 1
                            and a.Int1 = Transactions.Int1
                        limit 1
                    ),
                    ifnull(
                        -- new record
                        (
                            select max( c.Id ) + 1
                            from Chain c indexed by Chain_Id
                        ),
                        0 -- for first record
                    )
                ),
                Last = 1
            WHERE TxId = ?
        )sql");
        TryBindStatementInt64(setIdStmt, 1, txId);
        TryStepStatement(setIdStmt);

        // Clear old last records for set new last
        ClearOldLast(txId);
    }

    void ChainRepository::IndexContent(const int64_t& txId)
    {
        // Get new ID or copy previous
        auto setIdStmt = SetupSqlStatement(R"sql(
            UPDATE Chain SET
                Id = ifnull(
                    -- copy self Id
                    (
                        select c.Id
                        from Transactions t -- TODO (losty-db): index
                        join Chain c
                            on c.TxId = t.Id
                            and c.Last = 1
                        where t.Type in (200,201,202,209,210,207)
                            -- String2 = RootTxHash
                            and t.Int2 = Transactions.Int2
                        limit 1
                    ),
                    -- new record
                    ifnull(
                        (
                            select max( c.Id ) + 1
                            from Chain c indexed by Chain_Id
                        ),
                        0 -- for first record
                    )
                ),
                Last = 1
            WHERE TxId = ?
        )sql");
        TryBindStatementInt64(setIdStmt, 1, txId);
        TryStepStatement(setIdStmt);

        // Clear old last records for set new last
        ClearOldLast(txId);
    }

    void ChainRepository::IndexComment(const int64_t& txId)
    {
        // Get new ID or copy previous
        auto setIdStmt = SetupSqlStatement(R"sql(
            UPDATE Chain SET
                Id = ifnull(
                    -- copy self Id
                    (
                        select max( c.Id )
                        from Transactions t -- TODO (losty-db): index
                        join Chain c
                            on c.TxId = t.Id
                            and c.Last = 1
                            and c.BlockId > 0
                        where t.Type in (204,205,206)
                            -- String2 = RootTxHash
                            and t.Int2 = Transactions.Int2
                    ),
                    -- new record
                    ifnull(
                        (
                            select max( c.Id ) + 1
                            from Chian c indexed by Chain_Id
                        ),
                        0 -- for first record
                    )
                ),
                Last = 1
            WHERE TxId = ?
        )sql");
        TryBindStatementInt64(setIdStmt, 1, txId);
        TryStepStatement(setIdStmt);

        // Clear old last records for set new last
        ClearOldLast(txId);
    }

    void ChainRepository::IndexBlocking(const int64_t& txId)
    {
        // TODO (o1q): double check multiple locks
        // Set last=1 for new transaction
        auto setLastStmt = SetupSqlStatement(R"sql(
            UPDATE Chain SET
                Id = ifnull(
                    -- copy self Id
                    (
                        select c.Id
                        from Transactions a -- TODO (losty-db): index
                        join Chain c
                            on c.TxId = a.Id
                            and c.Last = 1
                            and c.BlockId > 0
                        where a.Type in (305, 306)
                            -- String1 = AddressHash
                            and a.Int1 = Transactions.Int1
                            -- String2 = AddressToHash
                            -- TODO (losty-db): is -1 ok?
                            and ifnull(a.Int2,-1) = ifnull(Transactions.Int2,-1)
                            and ifnull(a.Int3,-1) = ifnull(Transactions.Int3,-1)
                        limit 1
                    ),
                    ifnull(
                        -- new record
                        (
                            select max( c.Id ) + 1
                            from Chain c indexed by Chain_Id
                        ),
                        0 -- for first record
                    )
                ),
                Last = 1
            WHERE TxId = ?
        )sql");
        TryBindStatementInt64(setLastStmt, 1, txId);
        TryStepStatement(setLastStmt);

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
                and usc.BlockId > 0
            join Transactions ut -- TODO (losty-db): index
              on ut.Type in (100, 170)
                and ut.Int1 in (select b.Int2 union select cast(value as int) from json_each(b.Int3))
            join Chain utc -- TODO (losty-db): index
              on utc.TxId = ut.Id
                and utc.Last = 1
                and utc.BlockId > 0
            where b.Type in (305) and b.Id = ?
                and not exists (select 1 from BlockingLists bl where bl.IdSource = usc.Id and bl.IdTarget = utc.Id)
        )sql");
        TryBindStatementInt64(insListStmt, 1, txId);
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
                and usc.BlockId > 0
            join Transactions ut -- TODO (losty-db): index
              on ut.Type in (100, 170) and ut.Int1 = b.Int2
            join Chain utc -- TODO (losty-db): index
              on utc.TxId = ut.Id
                and utc.Last = 1
                and utc.Id = BlockingLists.IdTarget
                and utc.BlockId > 0
            where b.Type in (306) and b.Id = ?
            )
        )sql");
        TryBindStatementInt64(delListStmt, 1, txId);
        TryStepStatement(delListStmt);

        // Clear old last records for set new last
        ClearOldLast(txId);
    }

    void ChainRepository::IndexSubscribe(const int64_t& txId)
    {
        // Set last=1 for new transaction
        auto setLastStmt = SetupSqlStatement(R"sql(
            UPDATE Chain SET
                Id = ifnull(
                    -- copy self Id
                    (
                        select c.Id
                        from Transactions a -- TODO (losty-db): index
                        join Chain c -- TODO (losty-db): index
                            on c.TxId = a.Id
                            and c.Last = 1
                            and c.BlockId > 0
                        where a.Type in (302, 303, 304)
                            -- String1 = AddressHash
                            and a.Int1 = Transactions.Int1
                            -- String2 = AddressToHash
                            and a.Int2 = Transactions.Int2
                        limit 1
                    ),
                    ifnull(
                        -- new record
                        (
                            select max( c.Id ) + 1
                            from Chain c indexed by Chain_Id
                        ),
                        0 -- for first record
                    )
                ),
                Last = 1
            WHERE TxId = ?
        )sql");
        TryBindStatementInt64(setLastStmt, 1, txId);
        TryStepStatement(setLastStmt);

        // Clear old last records for set new last
        ClearOldLast(txId);
    }
    
    void ChainRepository::IndexBoostContent(const int64_t& txId)
    {
        // Set transaction fee
        auto stmt = SetupSqlStatement(R"sql(
            update Transactions
            set Int1 =
              (
                (
                  select sum(i.Value)
                  from TxOutputs i indexed by TxOutputs_SpentTxId
                  where i.SpentTxId = Transactions.Id
                ) - (
                  select sum(o.Value)
                  from TxOutputs o indexed by TxOutputs_TxId_AddressId_Value
                  where TxId = Transactions.Id
                )
              )
            where Transactions.Id = ?
              and Transactions.Type in (208)
        )sql");
        TryBindStatementInt64(stmt, 1, txId);
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
    
    void ChainRepository::ClearOldLast(const int64_t& txId)
    {
        auto stmt = SetupSqlStatement(R"sql(
            UPDATE Chain -- TODO (losty-db): indexing
            SET
                Last = 0
            FROM (
                select c.TxId, c.Id,
                from Chain c
                where c.TxId = ?
            ) as cInner
            WHERE   Chain.Id = cInner.Id
                and Chain.Last = 1
                and Chain.TxId != cInner.TxId
        )sql");

        TryBindStatementInt64(stmt, 1, txId);
        TryStepStatement(stmt);
    }

    void ChainRepository::RestoreOldLast(int height)
    {
        int64_t nTime0 = GetTimeMicros();

        // ----------------------------------------
        // Restore old Last transactions
        auto stmt1 = SetupSqlStatement(R"sql(
            update Chain -- TODO (losty-db): index
                set Last=1
            from (
                select c1.Id, max(b2.Id)BlockId -- TODO (losty-critical): is this ok??? Is max(blockId) the same as max(height)?
                from Chain c1
                join Blocks b1
                    on b1.Id = c1.BlockId
                    and b1.Height >= ?
                join Chain c2 on c2.Id = c1.Id and c2.Last = 0
                join Blocks b2 on b2.Id = c2.BlockId and b2.Height <= ?
                where c1.Last = 1
                group by c1.Id
            )c
            where Chain.Id = c.Id and Chain.BlockId = c.BlockId
        )sql");
        TryBindStatementInt(stmt1, 1, height);
        TryBindStatementInt(stmt1, 2, height);
        TryStepStatement(stmt1);

        int64_t nTime1 = GetTimeMicros();
        LogPrint(BCLog::BENCH, "        - RestoreOldLast (Transactions): %.2fms\n", 0.001 * (nTime1 - nTime0));

        // ----------------------------------------
        // Restore Last for deleting ratings
        auto stmt2 = SetupSqlStatement(R"sql(
            update Ratings indexed by Ratings_Type_Id_Height_Value
                set Last=1
            from (
                select r1.Type, r1.Id, max(r2.Height)Height
                from Ratings r1 indexed by Ratings_Type_Id_Last_Height
                join Ratings r2 indexed by Ratings_Type_Id_Last_Height on r2.Type = r1.Type and r2.Id = r1.Id and r2.Last = 0 and r2.Height < ?
                where r1.Height >= ?
                  and r1.Last = 1
                group by r1.Type, r1.Id
            )r
            where Ratings.Type = r.Type
              and Ratings.Id = r.Id
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

        // ----------------------------------------
        // Rollback general transaction information
        auto stmt0 = SetupSqlStatement(R"sql(
            delete from Chain c
            WHERE BlockId in (select Id from Blocks where Height >= ?)
        )sql");
        TryBindStatementInt(stmt0, 1, height);
        TryStepStatement(stmt0);

        int64_t nTime1 = GetTimeMicros();
        LogPrint(BCLog::BENCH, "        - RollbackHeight (Chain:Height = null): %.2fms\n", 0.001 * (nTime1 - nTime0));

        auto stmt1 = SetupSqlStatement(R"sql(
            delete from Blocks
            where Height >= ?
        )sql");
        TryBindStatementInt(stmt1, 1, height);
        TryStepStatement(stmt1);

        int64_t nTime2 = GetTimeMicros();
        LogPrint(BCLog::BENCH, "        - RollbackHeight (Blocks:Height = null): %.2fms\n", 0.001 * (nTime2 - nTime1));

        // ----------------------------------------
        // Rollback spent transaction outputs
        auto stmt2 = SetupSqlStatement(R"sql(
            UPDATE TxOutputs SET
                SpentHeight = null,
                SpentTxId = null
            WHERE SpentHeight >= ?
        )sql");
        TryBindStatementInt(stmt2, 1, height);
        TryStepStatement(stmt2);

        int64_t nTime3 = GetTimeMicros();
        LogPrint(BCLog::BENCH, "        - RollbackHeight (TxOutputs:SpentHeight = null): %.2fms\n", 0.001 * (nTime3 - nTime2));

        // ----------------------------------------
        // Rollback transaction outputs height
        auto stmt3 = SetupSqlStatement(R"sql(
            UPDATE TxOutputs SET
                TxHeight = null
            WHERE TxHeight >= ?
        )sql");
        TryBindStatementInt(stmt3, 1, height);
        TryStepStatement(stmt3);

        int64_t nTime4 = GetTimeMicros();
        LogPrint(BCLog::BENCH, "        - RollbackHeight (TxOutputs:TxHeight = null): %.2fms\n", 0.001 * (nTime4 - nTime3));

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
                  on bc.TxId =  b.Id
                join Blocks bcb
                  on bcb.Id = bc.BlockId
                    and bcb.Height >= ? -- TODO (losty-cirtical): very bad
                join Transactions us
                  on us.Type in (100, 170)
                    and us.String1 = b.String1
                join Chain usc
                  on usc.TxId = us.Id
                    and usc.Last = 1
                    and usc.BlockId > 0
                join Transactions ut
                  on ut.Type in (100, 170)
                    and ut.Int1 in (select b.Int2 union select cast(value as int) from json_each(b.String1))
                join Chain utc
                  on utc.Last = 1
                    and utc.BlockId > 0
                join BlockingLists bl on bl.IdSource = usc.Id and bl.IdTarget = utc.Id
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
              usc.Id,
              utc.Id
            from Transactions b
            join Chain bc
              on bc.TxId = b.Id
            join Blocks bcb
              on bcb.Id = bc.BlockId
                and bcb.Height >= ? -- TODO (losty-critical): very bad
            join Transactions us
              on us.Type in (100, 170) and us.Int1 = b.Int1
            join Chain usc
              on usc.TxId = us.Id
                and usc.Last = 1
            join Transactions ut
              on ut.Type in (100, 170) and ut.Last = 1
                --and ut.String1 = b.String2
                and ut.Int1 in (select b.Int2 union select cast (value as int) from json_each(b.String1))
            join Chain utc
              on utc.TxId = ut.Id
                and utc.Last = 1
            where b.Type in (306)
              and not exists (select 1 from BlockingLists bl where bl.IdSource = usc.Id and bl.IdTarget = utc.Id)
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
