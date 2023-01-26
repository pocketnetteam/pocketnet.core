// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include "pocketdb/repositories/ChainRepository.h"

namespace PocketDb
{
    void ChainRepository::IndexBlock(const string& blockHash, int height, vector<TransactionIndexingInfo>& txs)
    {
        SqlTransaction(__func__, [&]()
        {
            int64_t nTime1 = GetTimeMicros();

            IndexBlockData(blockHash);

            // Each transaction is processed individually
            for (const auto& txInfo : txs)
            {
                auto[id, lastTxId] = IndexSocial(txInfo);

                // Remove current last record for tx
                if (lastTxId)
                {
                    Sql(R"sql(
                        delete from Last
                        where TxId = ?
                    )sql")
                    .Bind(*lastTxId)
                    .Run();
                }

                // All transactions must have a blockHash & height relation
                InsertTransactionChainData(
                    blockHash,
                    txInfo.BlockNumber,
                    height,
                    txInfo.Hash,
                    id
                );
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
        int exists = 0;
        int last = 1;

        SqlTransaction(__func__, [&]()
        {
            Sql(R"sql(
                select
                    ifnull((
                        select
                            1
                        from
                            Chain c indexed by Chain_Height_BlockId
                        where
                            c.Height = ? and
                            c.BlockId = (
                                select
                                    r.RowId
                                from
                                    Registry r
                                where
                                    r.String = ?
                            )
                        limit 1
                    ), 0),
                    ifnull((
                        select
                            1
                        from
                            Chain c indexed by Chain_Height_BlockId
                        where
                            c.Height = ?
                        limit 1
                    ), 0)
            )sql")
            .Bind(height, blockHash, height + 1)
            .Select([&](Cursor& cursor) {
                cursor.Collect(exists, last);
            });
        });

        return { exists == 1, last == 0 };
    }

    void ChainRepository::IndexBlockData(const string& blockHash)
    {
        Sql(R"sql(
            insert or ignore into Registry (String)
            values (?)
        )sql")
        .Bind(blockHash)
        .Run();
    }

    void ChainRepository::InsertTransactionChainData(const string& blockHash, int blockNumber, int height, const string& txHash, const optional<int64_t>& id)
    {
        Sql(R"sql(
            with
                block as (
                    select
                        RowId
                    from
                        Registry
                    where
                        String = ?
                ),
                tx as (
                    select
                        t.RowId
                    from
                        vTx t
                    where
                        t.Hash = ?
                )
            insert or fail into Chain
                (TxId, BlockId, BlockNum, Height, Uid)
            select
                tx.RowId, block.RowId, ?, ?, ?
            from
                tx,
                block
            where
                not exists (
                    select
                        1
                    from
                        Chain
                    where
                        TxId = tx.RowId
                )
        )sql")
        .Bind(blockHash, txHash, blockNumber, height, id)
        .Run();

        if (id.has_value())
        {
            Sql(R"sql(
                insert into Last
                    (TxId)
                select
                    t.RowId
                from
                    vTx t
                where
                    t.Hash = ?
            )sql")
            .Bind(txHash)
            .Run();
        }
    }

    void ChainRepository::IndexBalances(int height)
    {
        // Generate new balance records
        Sql(R"sql(
            with
                height as (
                    select ? as value
                ),
                outs as (
                    select
                        o.TxId,
                        o.Number,
                        o.AddressId,
                        (+o.Value)val
                    from
                        height,
                        Chain c indexed by Chain_Height_BlockId
                        cross join TxOutputs o indexed by TxOutputs_TxId_Number_AddressId
                            on o.TxId = c.TxId
                    where
                        c.Height = height.value

                    union

                    select
                        o.TxId,
                        o.Number,
                        o.AddressId,
                        (-o.Value)val
                    from
                        height,
                        Chain ci indexed by Chain_Height_BlockId
                        cross join TxInputs i indexed by TxInputs_SpentTxId_TxId_Number
                            on i.SpentTxId = ci.TxId
                        cross join TxOutputs o indexed by TxOutputs_TxId_Number_AddressId
                            on o.TxId = i.TxId and o.Number = i.Number
                    where
                        ci.Height = height.value
                ),
                saldo as (
                    select
                        outs.AddressId,
                        sum(outs.val)Amount
                    from
                        outs
                    group by
                        outs.AddressId
                )

            replace into Balances (AddressId, Height, Value)
            select
                saldo.AddressId,
                height.value,
                ifnull(b.Value, 0) + saldo.Amount
            from
                height,
                saldo
                left join Balances b
                    on b.AddressId = saldo.AddressId
            where
                saldo.Amount != 0
        )sql")
        .Bind(height)
        .Run();
    }

    pair<optional<int64_t>, optional<int64_t>> ChainRepository::IndexSocial(const TransactionIndexingInfo& txInfo)
    {
        optional<int64_t> id;
        optional<int64_t> lastTxId;
        string sql;

        // Account and Content must have unique ID
        // Also all edited transactions must have Last=(0/1) field
        if (txInfo.IsAccount())
            sql = IndexAccount();
        else if (txInfo.IsAccountSetting())
            sql = IndexAccountSetting();
        else if (txInfo.IsContent())
            sql = IndexContent();
        else if (txInfo.IsComment())
            sql = IndexComment();
        else if (txInfo.IsBlocking())
            sql = IndexBlocking();
        else if (txInfo.IsSubscribe())
            sql = IndexSubscribe();
        else
            return {id, lastTxId};

        Sql(sql)
        .Bind(txInfo.Hash)
        .Select([&](Cursor& cursor) {
            cursor.Collect(id, lastTxId);
        });

        return {id, lastTxId};
    }

    string ChainRepository::IndexAccount()
    {
        return R"sql(
            with
                l as (
                    select
                        b.RowId
                    from
                        Transactions a -- primary key
                        join Transactions b indexed by Transactions_Type_RegId1_RegId2_RegId3
                            on b.Type in (100, 170) and 
                                b.RegId1 = a.RegId1
                        join Last l -- primary key
                            on l.TxId = b.RowId
                    where
                        a.Type in (100, 170) and
                        a.RowId = (
                            select t.RowId
                            from vTx t
                            where t.Hash = ?
                        )
                    limit 1
                )
            select
                ifnull(
                    (
                        select
                            c.Uid
                        from
                            Chain c, -- primary key
                            l
                        where
                            c.TxId = l.RowId
                    ),
                    ifnull(
                        -- new record
                        (
                            select
                                max(c.Uid) + 1
                            from
                                Chain c indexed by Chain_Uid_Height
                        ),
                        -- for first record
                        (
                            select 0
                        )
                    )
                ),
                (
                    select l.RowId
                    from l
                )
        )sql";
    }

    string ChainRepository::IndexAccountSetting()
    {
        return R"sql(
            with
                l as (
                    select
                        b.RowId
                    from
                        Transactions a -- primary key
                        join Transactions b indexed by Transactions_Type_RegId1_RegId2_RegId3
                            on b.Type in (103)
                            and b.RegId1 = a.RegId1
                        join Last l -- primary key
                            on l.TxId = b.RowId
                    where
                        a.Type in (103) and
                        a.RowId = (
                            select t.RowId
                            from vTx t
                            where t.Hash = ?
                        )
                    limit 1
                )
            select
                ifnull(
                    (
                        select
                            c.Uid
                        from
                            Chain c, -- primary key
                            l
                        where
                            c.TxId = l.RowId
                    ),
                    ifnull(
                        -- new record
                        (
                            select
                                max(c.Uid) + 1
                            from
                                Chain c indexed by Chain_Uid_Height
                        ),
                        -- for first record
                        (
                            select 0
                        )
                    )
                ),
                (
                    select l.RowId
                    from l
                )
        )sql";
    }

    string ChainRepository::IndexContent()
    {
        return R"sql(
            with
                l as (
                    select
                        b.RowId
                    from
                        Transactions a -- primary key
                        join Transactions b indexed by Transactions_Type_RegId1_RegId2_RegId3
                            on b.Type in (200, 201, 202, 209, 210, 207) and
                            b.RegId1 = a.RegId1 and
                            b.RegId2 = a.RegId2
                        join Last l -- primary key
                            on l.TxId = b.RowId
                    where
                        a.Type in (200, 201, 202, 209, 210, 207) and
                        a.RowId = (
                            select t.RowId
                            from vTx t
                            where t.Hash = ?
                        )
                    limit 1
                )
            select
                ifnull(
                    (
                        select
                            c.Uid
                        from
                            Chain c, -- primary key
                            l
                        where
                            c.TxId = l.RowId
                    ),
                    ifnull(
                        -- new record
                        (
                            select
                                max(c.Uid) + 1
                            from
                                Chain c indexed by Chain_Uid_Height
                        ),
                        -- for first record
                        (
                            select 0
                        )
                    )
                ),
                (
                    select l.RowId
                    from l
                )
        )sql";
    }

    string ChainRepository::IndexComment()
    {
        return R"sql(
            with
                l as (
                    select
                        b.RowId
                    from
                        Transactions a -- primary key
                        join Transactions b indexed by Transactions_Type_RegId1_RegId2_RegId3
                            on b.Type in (204, 205, 206) and
                            b.RegId1 = a.RegId1 and
                            b.RegId2 = a.RegId2
                        join Last l -- primary key
                            on l.TxId = b.RowId
                    where
                        a.Type in (204, 205, 206) and
                        a.RowId = (
                            select t.RowId
                            from vTx t
                            where t.Hash = ?
                        )
                    limit 1
                )
            select
                ifnull(
                    (
                        select
                            c.Uid
                        from
                            Chain c, -- primary key
                            l
                        where
                            c.TxId = l.RowId
                    ),
                    ifnull(
                        -- new record
                        (
                            select
                                max(c.Uid) + 1
                            from
                                Chain c indexed by Chain_Uid_Height
                        ),
                        -- for first record
                        (
                            select 0
                        )
                    )
                ),
                (
                    select l.RowId
                    from l
                )
        )sql";
    }

    string ChainRepository::IndexBlocking()
    {
        return R"sql(
            with
                l as (
                    select
                        b.RowId
                    from
                        Transactions a -- primary key
                        join Transactions b indexed by Transactions_Type_RegId1_RegId2_RegId3
                            on b.Type in (305, 306) and
                            b.RegId1 = a.RegId1 and
                            ifnull(b.RegId2, -1) = ifnull(a.RegId2, -1) and
                            ifnull(b.RegId3, -1) = ifnull(a.RegId3, -1)
                        join Last l -- primary key
                            on l.TxId = b.RowId
                    where
                        a.Type in (305, 306) and
                        a.RowId = (
                            select t.RowId
                            from vTx t
                            where t.Hash = ?
                        )
                    limit 1
                )
            select
                ifnull(
                    (
                        select
                            c.Uid
                        from
                            Chain c, -- primary key
                            l
                        where
                            c.TxId = l.RowId
                    ),
                    ifnull(
                        -- new record
                        (
                            select
                                max(c.Uid) + 1
                            from
                                Chain c indexed by Chain_Uid_Height
                        ),
                        -- for first record
                        (
                            select 0
                        )
                    )
                ),
                (
                    select l.RowId
                    from l
                )
        )sql";

        // TODO (optimization): bad bad bad!!!
        // SqlTransaction(__func__, [&]()
        // {
        //     auto insListStmt = Sql(R"sql(
        //         insert into BlockingLists (IdSource, IdTarget)
        //         select
        //         usc.Id,
        //         utc.Id
        //         from Transactions b -- TODO (optimization): index
        //         join Transactions us -- TODO (optimization): index
        //         on us.Type in (100, 170) and us.Int1 = b.Int1
        //         join Chain usc -- TODO (optimization): index
        //         on usc.TxId = us.Id
        //             and usc.Last = 1
        //         join Transactions ut -- TODO (optimization): index
        //         on ut.Type in (100, 170)
        //             and ut.Int1 in (select b.Int2 union select cast(value as int) from json_each(b.Int3))
        //         join Chain utc -- TODO (optimization): index
        //         on utc.TxId = ut.Id
        //             and utc.Last = 1
        //         where b.Type in (305) and b.Id = (select RowId from Transactions where HashId = (select RowId from Registry where String = ?))
        //             and not exists (select 1 from BlockingLists bl where bl.IdSource = usc.Id and bl.IdTarget = utc.Id)
        //     )sql");
        //     insListStmt->Bind(txHash);
        //     TryStepStatement(insListStmt);

        //     auto delListStmt = Sql(R"sql(
        //         delete from BlockingLists
        //         where exists
        //         (select
        //         1
        //         from Transactions b -- TODO (optimization): index
        //         join Transactions us -- TODO (optimization): index
        //         on us.Type in (100, 170) and us.Int1 = b.Int1
        //         join Chain usc -- TODO (optimization): index
        //         on usc.TxId = us.Id
        //             and usc.Last = 1
        //             and usc.Id = BlockingLists.IdSource
        //         join Transactions ut -- TODO (optimization): index
        //         on ut.Type in (100, 170) and ut.Int1 = b.Int2
        //         join Chain utc -- TODO (optimization): index
        //         on utc.TxId = ut.Id
        //             and utc.Last = 1
        //             and utc.Id = BlockingLists.IdTarget
        //         where b.Type in (306) and b.Id = (select RowId from Transactions where HashId = (select RowId from Registry where String = ?))
        //         )
        //     )sql");
        //     delListStmt->Bind(txHash);
        //     TryStepStatement(delListStmt);
        // });
    }

    string ChainRepository::IndexSubscribe()
    {
        return R"sql(
            with
                l as (
                    select
                        b.RowId
                    from
                        Transactions a -- primary key
                        join Transactions b indexed by Transactions_Type_RegId1_RegId2_RegId3
                            on b.Type in (302, 303, 304) and
                            b.RegId1 = a.RegId1 and
                            b.RegId2 = a.RegId2
                        join Last l
                            on l.TxId = b.RowId
                    where
                        a.Type in (302, 303, 304) and
                        a.RowId = (
                            select t.RowId
                            from vTx t
                            where t.Hash = ?
                        )
                    limit 1
                )
            select
                ifnull(
                    (
                        select
                            c.Uid
                        from
                            Chain c, -- primary key
                            l
                        where
                            c.TxId = l.RowId
                    ),
                    ifnull(
                        -- new record
                        (
                            select
                                max(c.Uid) + 1
                            from
                                Chain c indexed by Chain_Uid_Height
                        ),
                        -- for first record
                        (
                            select 0
                        )
                    )
                ),
                (
                    select l.RowId
                    from l
                )
        )sql";
    }


    bool ChainRepository::ClearDatabase()
    {
        LogPrintf("Deleting database indexes..\n");

        try
        {
            // Update transactions
            SqlTransaction(__func__, [&]()
            {
                m_database.DropIndexes();

                Sql(R"sql( delete from Last )sql").Run();
                Sql(R"sql( delete from Ratings )sql").Run();
                Sql(R"sql( delete from Balances )sql").Run();
                Sql(R"sql( delete from Chain )sql").Run();

                // TODO (aok) : bad
                // ClearBlockingList();

                m_database.CreateStructure();
            });

            return true;
        }
        catch (exception& ex)
        {
            LogPrintf("Error: Clear database failed with message: %s\n", ex.what());
            return false;
        }
    }

    bool ChainRepository::Rollback(int height)
    {
        try
        {
            // Update transactions
            SqlTransaction(__func__, [&]()
            {
                RestoreLast(height);
                RestoreRatings(height);
                RestoreBalances(height);
                RestoreChain(height);

                // TODO (aok) : bad
                // RollbackBlockingList(height);
            });

            return true;
        }
        catch (exception& ex)
        {
            LogPrintf("Error: Rollback to height %d failed with message: %s\n", height, ex.what());
            return false;
        }
    }
    
    void ChainRepository::RestoreLast(int height)
    {
        // Delete current last records
        Sql(R"sql(
            delete from Last
            where
                TxId in (
                    select
                        c.TxId
                    from
                        Chain c indexed by Chain_Height_Uid
                    where
                        c.Height >= ?
                )
        )sql")
        .Bind(height)
        .Run();

        // Restore old Last transactions
        Sql(R"sql(
            with
                height as (
                    select ? as value
                ),
                prev as (
                    select
                        c.Uid, max(cc.Height)maxHeight
                    from
                        height,
                        Chain c indexed by Chain_Height_Uid
                        cross join Last l -- primary key
                            on l.TxId = c.TxId
                        cross join Chain cc indexed by Chain_Uid_Height
                            on cc.Uid = c.Uid and cc.Height < height.value
                    where
                        c.Height >= height.value
                    group by c.Uid
                )
            insert into
                Last (TxId)
            select
                cp.TxId
            from
                prev
                cross join Chain cp indexed by Chain_Uid_Height
                    on cp.Uid = prev.Uid and
                    cp.Height = prev.maxHeight
        )sql")
        .Bind(height)
        .Run();
    }

    void ChainRepository::RestoreRatings(int height)
    {
        // Restore Last for deleting ratings
        Sql(R"sql(
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
        )sql")
        .Bind(height, height)
        .Run();

        // Remove ratings
        Sql(R"sql(
            delete from Ratings indexed by Ratings_Height_Last
            where Height >= ?
        )sql")
        .Bind(height)
        .Run();
    }

    void ChainRepository::RestoreBalances(int height)
    {
        // Rollback balances
        Sql(R"sql(
            with
                height as (
                    select ? as value
                ),
                outs as (
                    select
                        o.AddressId,
                        (+o.Value)val
                    from
                        height,
                        Chain c indexed by Chain_Height_BlockId
                        cross join TxOutputs o indexed by TxOutputs_TxId_Number_AddressId
                            on o.TxId = c.TxId
                    where
                        c.Height >= height.value

                    union

                    select
                        o.AddressId,
                        (-o.Value)val
                    from
                        height,
                        Chain ci indexed by Chain_Height_BlockId
                        cross join TxInputs i indexed by TxInputs_SpentTxId_TxId_Number
                            on i.SpentTxId = ci.TxId
                        cross join TxOutputs o indexed by TxOutputs_TxId_Number_AddressId
                            on o.TxId = i.TxId and o.Number = i.Number
                    where
                        ci.Height >= height.value
                ),
                saldo as (
                    select
                        outs.AddressId,
                        sum(outs.val)Amount
                    from
                        outs
                    group by
                        outs.AddressId
                )

            replace into Balances (AddressId, Value)
            select
                saldo.AddressId,
                ifnull(b.Value, 0) - saldo.Amount
            from
                saldo
                left join Balances b
                    on b.AddressId = saldo.AddressId
            where
                saldo.Amount != 0
        )sql")
        .Bind(height)
        .Run();
    }

    void ChainRepository::RestoreChain(int height)
    {
        Sql(R"sql(
            delete from Chain indexed by Chain_Height_BlockId
            where Height >= ?
        )sql")
        .Bind(height)
        .Run();
    }

    // void ChainRepository::RollbackBlockingList(int height)
    // {
    //     int64_t nTime0 = GetTimeMicros();

    //     auto& delListStmt = Sql(R"sql(
    //         delete from BlockingLists where ROWID in
    //         (
    //             select bl.ROWID
    //             from Transactions b
    //             join Chain bc
    //               on bc.TxId =  b.RowId
    //                 and bc.Height >= ?
    //             join Transactions us
    //               on us.Type in (100, 170)
    //                 and us.RegId1 = b.RegId1
    //             join Chain usc
    //               on usc.TxId = us.RowId
    //             join Last usl
    //               on usl.TxId = us.RowId
    //             join Transactions ut
    //               on ut.Type in (100, 170)
    //                 and ut.Int1 in (select b.RegId2 union select l.RegId from Lists l where l.TxId = b.RowId)
    //             join Chain utc
    //               on utc.TxId = ut.RowId
    //             join Last utl
    //               on utl.TxId = ut.RowId
    //             join BlockingLists bl on bl.IdSource = usc.Uid and bl.IdTarget = utc.Uid
    //             where b.Type in (305)
    //         )
    //     )sql");
    //     delListStmt->Bind(height);
    //     delListStmt->Step();
        
    //     int64_t nTime1 = GetTimeMicros();
    //     LogPrint(BCLog::BENCH, "        - RollbackList (Delete blocking list): %.2fms\n", 0.001 * (nTime1 - nTime0));

    //     auto& insListStmt = Sql(R"sql(
    //         insert into BlockingLists
    //         (
    //             IdSource,
    //             IdTarget
    //         )
    //         select distinct
    //           usc.Uid,
    //           utc.Uid
    //         from Transactions b
    //         join Chain bc
    //           on bc.TxId = b.RowId
    //             and bc.Height >= ?
    //         join Transactions us
    //           on us.Type in (100, 170) and us.Int1 = b.Int1
    //         join Chain usc
    //           on usc.TxId = us.RowId
    //         join Last usl
    //             on usl.TxId = us.RowId
    //         join Transactions ut
    //           on ut.Type in (100, 170)
    //             --and ut.String1 = b.String2
    //             and ut.Int1 in (select b.RegId2 union select l.RegId from Lists l where l.TxId = b.RowId)
    //         join Chain utc
    //           on utc.TxId = ut.RowId
    //         join Last utl
    //           on utl.TxId = ut.RowId
    //         where b.Type in (306)
    //           and not exists (select 1 from BlockingLists bl where bl.IdSource = usc.Uid and bl.IdTarget = utc.Uid)
    //     )sql");
    //     insListStmt->Bind(height);
    //     insListStmt->Step();
        
    //     int64_t nTime2 = GetTimeMicros();
    //     LogPrint(BCLog::BENCH, "        - RollbackList (Insert blocking list): %.2fms\n", 0.001 * (nTime2 - nTime1));
    // }

    // void ChainRepository::ClearBlockingList()
    // {
    //     int64_t nTime0 = GetTimeMicros();

    //     auto& stmt = Sql(R"sql(
    //         delete from BlockingLists
    //     )sql");
    //     stmt.Step();
        
    //     int64_t nTime1 = GetTimeMicros();
    //     LogPrint(BCLog::BENCH, "        - ClearBlockingList (Delete blocking list): %.2fms\n", 0.001 * (nTime1 - nTime0));
    // }


} // namespace PocketDb
