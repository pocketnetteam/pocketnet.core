// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include "pocketdb/repositories/ChainRepository.h"

namespace PocketDb
{
    void ChainRepository::IndexBlock(const string& blockHash, int height, vector<TransactionIndexingInfo>& txs)
    {
        // TODO (brangr) (v0.21.1): optimizations
        TryTransactionStep(__func__, [&]()
        {
            int64_t nTime1 = GetTimeMicros();

            // Each transaction is processed individually
            for (const auto& txInfo : txs)
            {
                // All transactions must have a blockHash & height relation
                UpdateTransactionHeight(blockHash, txInfo.BlockNumber, height, txInfo.Hash);

                // The outputs are needed for the explorer
                // TODO (aok) (v0.20.19+): replace with update inputs spent with TxInputs table over loop
                UpdateTransactionOutputs(txInfo, height);

                // Account and Content must have unique ID
                // Also all edited transactions must have Last=(0/1) field
                if (txInfo.IsAccount())
                    IndexAccount(txInfo.Hash);

                if (txInfo.IsAccountSetting())
                    IndexAccountSetting(txInfo.Hash);

                if (txInfo.IsContent())
                    IndexContent(txInfo.Hash);

                if (txInfo.IsComment())
                    IndexComment(txInfo.Hash);

                if (txInfo.IsBlocking())
                    IndexBlocking(txInfo.Hash);

                if (txInfo.IsSubscribe())
                    IndexSubscribe(txInfo.Hash);

                // Calculate and save fee for future selects
                if (txInfo.IsBoostContent())
                    IndexBoostContent(txInfo.Hash);
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


    void ChainRepository::UpdateTransactionHeight(const string& blockHash, int blockNumber, int height, const string& txHash)
    {
        auto stmt = SetupSqlStatement(R"sql(
            UPDATE Transactions SET
                BlockHash = ?,
                BlockNum = ?,
                Height = ?
            WHERE Hash = ? and BlockHash is null
        )sql");
        TryBindStatementText(stmt, 1, blockHash);
        TryBindStatementInt(stmt, 2, blockNumber);
        TryBindStatementInt(stmt, 3, height);
        TryBindStatementText(stmt, 4, txHash);
        TryStepStatement(stmt);

        auto stmtOuts = SetupSqlStatement(R"sql(
            UPDATE TxOutputs SET
                TxHeight = ?
            WHERE TxHash = ? and TxHeight is null
        )sql");
        TryBindStatementInt(stmtOuts, 1, height);
        TryBindStatementText(stmtOuts, 2, txHash);
        TryStepStatement(stmtOuts);
    }

    void ChainRepository::UpdateTransactionOutputs(const TransactionIndexingInfo& txInfo, int height)
    {
        for (auto& input : txInfo.Inputs)
        {
            auto stmt = SetupSqlStatement(R"sql(
                UPDATE TxOutputs SET
                    SpentHeight = ?,
                    SpentTxHash = ?
                WHERE TxHash = ? and Number = ?
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
            insert into Balances (AddressHash, Last, Height, Value)
            select
                saldo.AddressHash,
                1,
                ?,
                sum(ifnull(saldo.Amount,0)) + ifnull(b.Value,0)
            from (

                select 'unspent',
                       o.AddressHash,
                       sum(o.Value)Amount
                from TxOutputs o indexed by TxOutputs_TxHeight_AddressHash
                where  o.TxHeight = ?
                group by o.AddressHash

                union

                select 'spent',
                       o.AddressHash,
                       -sum(o.Value)Amount
                from TxOutputs o indexed by TxOutputs_SpentHeight_AddressHash
                where o.SpentHeight = ?
                group by o.AddressHash

            ) saldo
            left join Balances b indexed by Balances_AddressHash_Last
                on b.AddressHash = saldo.AddressHash and b.Last = 1
            where saldo.AddressHash != ''
            group by saldo.AddressHash
        )sql");
        TryBindStatementInt(stmt, 1, height);
        TryBindStatementInt(stmt, 2, height);
        TryBindStatementInt(stmt, 3, height);
        TryStepStatement(stmt);

        // Remove old Last records
        auto stmtOld = SetupSqlStatement(R"sql(
            update Balances indexed by Balances_AddressHash_Last_Height
              set Last = 0
            where Balances.Last = 1
              and Balances.Height < ?
              and Balances.AddressHash in (
                select b.AddressHash
                from Balances b indexed by Balances_Height
                where b.Height = ?
              )
        )sql");
        TryBindStatementInt(stmtOld, 1, height);
        TryBindStatementInt(stmtOld, 2, height);
        TryStepStatement(stmtOld);
    }

    void ChainRepository::IndexAccount(const string& txHash)
    {
        // Get new ID or copy previous
        auto setIdStmt = SetupSqlStatement(R"sql(
            UPDATE Transactions SET
                Id = ifnull(
                    -- copy self Id
                    (
                        select a.Id
                        from Transactions a indexed by Transactions_Type_Last_String1_Height_Id
                        where a.Type in (100,170)
                            and a.Last = 1
                            and a.String1 = Transactions.String1
                            and a.Height is not null
                        limit 1
                    ),
                    ifnull(
                        -- new record
                        (
                            select max( a.Id ) + 1
                            from Transactions a indexed by Transactions_Id
                        ),
                        0 -- for first record
                    )
                ),
                Last = 1
            WHERE Hash = ?
        )sql");
        TryBindStatementText(setIdStmt, 1, txHash);
        TryStepStatement(setIdStmt);

        // Clear old last records for set new last
        ClearOldLast(txHash);
    }

    void ChainRepository::IndexAccountSetting(const string& txHash)
    {
        // Get new ID or copy previous
        auto setIdStmt = SetupSqlStatement(R"sql(
            UPDATE Transactions SET
                Id = ifnull(
                    -- copy self Id
                    (
                        select a.Id
                        from Transactions a indexed by Transactions_Type_Last_String1_Height_Id
                        where a.Type in (103)
                            and a.Last = 1
                            and a.String1 = Transactions.String1
                            and a.Height is not null
                        limit 1
                    ),
                    ifnull(
                        -- new record
                        (
                            select max( a.Id ) + 1
                            from Transactions a indexed by Transactions_Id
                        ),
                        0 -- for first record
                    )
                ),
                Last = 1
            WHERE Hash = ?
        )sql");
        TryBindStatementText(setIdStmt, 1, txHash);
        TryStepStatement(setIdStmt);

        // Clear old last records for set new last
        ClearOldLast(txHash);
    }

    void ChainRepository::IndexContent(const string& txHash)
    {
        // Get new ID or copy previous
        auto setIdStmt = SetupSqlStatement(R"sql(
            UPDATE Transactions SET
                Id = ifnull(
                    -- copy self Id
                    (
                        select c.Id
                        from Transactions c indexed by Transactions_Type_Last_String2_Height
                        where c.Type in (200,201,202,209,210,207)
                            and c.Last = 1
                            -- String2 = RootTxHash
                            and c.String2 = Transactions.String2
                            and c.Height is not null
                        limit 1
                    ),
                    -- new record
                    ifnull(
                        (
                            select max( c.Id ) + 1
                            from Transactions c indexed by Transactions_Id
                        ),
                        0 -- for first record
                    )
                ),
                Last = 1
            WHERE Hash = ?
        )sql");
        TryBindStatementText(setIdStmt, 1, txHash);
        TryStepStatement(setIdStmt);

        // Clear old last records for set new last
        ClearOldLast(txHash);
    }

    void ChainRepository::IndexComment(const string& txHash)
    {
        // Get new ID or copy previous
        auto setIdStmt = SetupSqlStatement(R"sql(
            UPDATE Transactions SET
                Id = ifnull(
                    -- copy self Id
                    (
                        select max( c.Id )
                        from Transactions c indexed by Transactions_Type_Last_String2_Height
                        where c.Type in (204,205,206)
                            and c.Last = 1
                            -- String2 = RootTxHash
                            and c.String2 = Transactions.String2
                            and c.Height is not null
                    ),
                    -- new record
                    ifnull(
                        (
                            select max( c.Id ) + 1
                            from Transactions c indexed by Transactions_Id
                        ),
                        0 -- for first record
                    )
                ),
                Last = 1
            WHERE Hash = ?
        )sql");
        TryBindStatementText(setIdStmt, 1, txHash);
        TryStepStatement(setIdStmt);

        // Clear old last records for set new last
        ClearOldLast(txHash);
    }

    void ChainRepository::IndexBlocking(const string& txHash)
    {
        // TODO (o1q): double check multiple locks
        // Set last=1 for new transaction
        auto setLastStmt = SetupSqlStatement(R"sql(
            UPDATE Transactions SET
                Id = ifnull(
                    -- copy self Id
                    (
                        select a.Id
                        from Transactions a indexed by Transactions_Type_Last_String1_String2_Height
                        where a.Type in (305, 306)
                            and a.Last = 1
                            -- String1 = AddressHash
                            and a.String1 = Transactions.String1
                            -- String2 = AddressToHash
                            and ifnull(a.String2,'') = ifnull(Transactions.String2,'')
                            and ifnull(a.String3,'') = ifnull(Transactions.String3,'')
                            and a.Height is not null
                        limit 1
                    ),
                    ifnull(
                        -- new record
                        (
                            select max( a.Id ) + 1
                            from Transactions a indexed by Transactions_Id
                        ),
                        0 -- for first record
                    )
                ),
                Last = 1
            WHERE Hash = ?
        )sql");
        TryBindStatementText(setLastStmt, 1, txHash);
        TryStepStatement(setLastStmt);

        auto insListStmt = SetupSqlStatement(R"sql(
            insert into BlockingLists (IdSource, IdTarget)
            select
              us.Id,
              ut.Id
            from Transactions b
            join Transactions us indexed by Transactions_Type_Last_String1_Height_Id
              on us.Type in (100, 170) and us.Last = 1 and us.String1 = b.String1 and us.Height > 0
            join Transactions ut indexed by Transactions_Type_Last_String1_Height_Id
              on ut.Type in (100, 170) and ut.Last = 1
                and ut.String1 in (select b.String2 union select value from json_each(b.String3))
                and ut.Height > 0
            where b.Type in (305) and b.Hash = ?
                and not exists (select 1 from BlockingLists bl where bl.IdSource = us.Id and bl.IdTarget = ut.Id)
        )sql");
        TryBindStatementText(insListStmt, 1, txHash);
        TryStepStatement(insListStmt);

        auto delListStmt = SetupSqlStatement(R"sql(
            delete from BlockingLists
            where exists
            (select
              1
            from Transactions b
            join Transactions us indexed by Transactions_Type_Last_String1_Height_Id
              on us.Type in (100, 170) and us.Last = 1 and us.String1 = b.String1 and us.Id = BlockingLists.IdSource and us.Height > 0
            join Transactions ut indexed by Transactions_Type_Last_String1_Height_Id
              on ut.Type in (100, 170) and ut.Last = 1 and ut.String1 = b.String2 and ut.Id = BlockingLists.IdTarget and ut.Height > 0
            where b.Type in (306) and b.Hash = ?
            )
        )sql");
        TryBindStatementText(delListStmt, 1, txHash);
        TryStepStatement(delListStmt);

        // Clear old last records for set new last
        ClearOldLast(txHash);
    }

    void ChainRepository::IndexSubscribe(const string& txHash)
    {
        // Set last=1 for new transaction
        auto setLastStmt = SetupSqlStatement(R"sql(
            UPDATE Transactions SET
                Id = ifnull(
                    -- copy self Id
                    (
                        select a.Id
                        from Transactions a indexed by Transactions_Type_Last_String1_String2_Height
                        where a.Type in (302, 303, 304)
                            and a.Last = 1
                            -- String1 = AddressHash
                            and a.String1 = Transactions.String1
                            -- String2 = AddressToHash
                            and a.String2 = Transactions.String2
                            and a.Height is not null
                        limit 1
                    ),
                    ifnull(
                        -- new record
                        (
                            select max( a.Id ) + 1
                            from Transactions a indexed by Transactions_Id
                        ),
                        0 -- for first record
                    )
                ),
                Last = 1
            WHERE Hash = ?
        )sql");
        TryBindStatementText(setLastStmt, 1, txHash);
        TryStepStatement(setLastStmt);

        // Clear old last records for set new last
        ClearOldLast(txHash);
    }
    
    void ChainRepository::IndexBoostContent(const string& txHash)
    {
        // Set transaction fee
        auto stmt = SetupSqlStatement(R"sql(
            update Transactions
            set Int1 =
              (
                (
                  select sum(i.Value)
                  from TxOutputs i indexed by TxOutputs_SpentTxHash
                  where i.SpentTxHash = Transactions.Hash
                ) - (
                  select sum(o.Value)
                  from TxOutputs o indexed by TxOutputs_TxHash_AddressHash_Value
                  where TxHash = Transactions.Hash
                )
              )
            where Transactions.Hash = ?
              and Transactions.Type in (208)
        )sql");
        TryBindStatementText(stmt, 1, txHash);
        TryStepStatement(stmt);
    }

    void ChainRepository::IndexModerationJury(const string& flagTxHash, int flagsDepth, int flagsMinCount)
    {
        TryTransactionStep(__func__, [&]()
        {
            auto stmt = SetupSqlStatement(R"sql(

                insert into Jury

                select

                    f.ROWID, /* Unique id of Flag record */
                    f.String3, /* Address of the content author */
                    f.Int1, /* Reason */
                    null /* Verdict */

                from Transactions f indexed by sqlite_autoindex_Transactions_1

                where f.Hash = ?

                -- Is there no active punishment listed on the account ?
                and not exists (
                    select 1
                    from Ban b indexed by Ban_AddressHash_Reason_Ending
                    where b.AddressHash = f.String3
                        and b.Reason = f.Int1
                        and b.Ending > f.Height
                )

                -- there is no active jury for the same reason
                and not exists (
                    select 1
                    from Jury j indexed by Jury_AddressHash_Reason_Verdict
                    where j.AddressHash = f.String3
                        and j.Reason = f.Int1
                        and j.Verdict is not null
                )

                -- if there are X flags of the same reason for X time
                and ? <= (
                    select count()
                    from Transactions ff indexed by Transactions_Type_Last_String2_Height
                    where ff.Type in (410)
                        and ff.Last = 0
                        and ff.String3 = f.String3
                        and ff.Height > ?
                        and ff.Hash != f.Hash
                )

            )sql");

            TryBindStatementText(stmt, 1, flagTxHash);
            TryBindStatementInt(stmt, 2, flagsMinCount);
            TryBindStatementInt(stmt, 3, flagsDepth);

            TryStepStatement(stmt);
        });
    }

    void ChainRepository::IndexModerationBan(const string& voteTxHash)
    {
        TryTransactionStep(__func__, [&]()
        {
            auto stmt_update_0 = SetupSqlStatement(R"sql(
              -- if there is at least one negative vote, then the defendant is acquitted
              update Jury indexed by sqlite_autoindex_Jury_1 set
                Verdict = 0
              where Jury.Verdict is null
                and Jury.FlagRowId = (
                  select f.ROWID
                  from Transactions v indexed by sqlite_autoindex_Transactions_1
                  cross join Transactions f indexed by sqlite_autoindex_Transactions_1
                    on f.Hash = v.String2
                  where v.Hash = ?
                    and exists (
                      select 1
                      from Transactions vv indexed by Transactions_Type_Last_String2_Height
                      where vv.Type in (420) -- Votes
                        and vv.Last = 0
                        and vv.String2 = f.Hash -- JuryId over FlagTxHash
                        and vv.Height > 0
                        and vv.Int1 = 0 -- Negative verdict
                    )
                )
            )sql");
            TryBindStatementText(stmt_update_0, 1, voteTxHash);
            TryStepStatement(stmt_update_0);
            
            auto stmt_update_1 = SetupSqlStatement(R"sql(
              -- if there are X positive votes, then the defendant is punished
              update Jury indexed by sqlite_autoindex_Jury_1 set
                Verdict = 1
              where Jury.Verdict is null
                and Jury.FlagRowId = (
                  select f.ROWID
                  from Transactions v indexed by sqlite_autoindex_Transactions_1
                  cross join Transactions f indexed by sqlite_autoindex_Transactions_1
                    on f.Hash = v.String2
                  where v.Hash = ?
                    and 8 <= (
                      select count()
                      from Transactions vv indexed by Transactions_Type_Last_String2_Height
                      where vv.Type in (420) -- Votes
                        and vv.Last = 0
                        and vv.String2 = f.Hash -- JuryId over FlagTxHash
                        and vv.Height > 0
                        and vv.Int1 = 1 -- Positive verdict
                    )
                )
            )sql");
            TryBindStatementText(stmt_update_1, 1, voteTxHash);
            TryStepStatement(stmt_update_1);
            
            auto stmt_ban = SetupSqlStatement(R"sql(
              -- If the defendant is punished, then we need to create a ban record
              insert into Ban

              select
                
                v.ROWID, /* Unique id of Flag record */
                j.AddressHash, /* Address of the content author */
                j.Reason, /* Reason */
                (
                  case (select count() from Ban b indexed by Ban_AddressHash_Reason_Ending where b.AddressHash = f.String3)
                    when 0 then 43200    -- 1 month
                    when 1 then 129600   -- 3 month
                    else 51840000        -- 100 years
                  end
                ) /* Ban period */

              from Transactions v indexed by sqlite_autoindex_Transactions_1

              join Transactions f indexed by sqlite_autoindex_Transactions_1
                on f.Hash = v.String2

              join Jury j indexed by sqlite_autoindex_Transactions_1
                on j.FlagRowId = f.ROWID and j.Verdict = 1

              where v.Hash = ?
                and not exists (
                  select 1
                  from Ban b indexed by Ban_AddressHash_Reason_Ending
                  where b.AddressHash = f.String3
                    and b.Ending > v.Height
                )
            )sql");
            TryBindStatementText(stmt_ban, 1, voteTxHash);
            TryStepStatement(stmt_ban);
        });
    }


    void ChainRepository::IndexBadges_Shark(int height, const BadgeSharkConditions& conditions)
    {
        TryTransactionStep(__func__, [&]()
        {
            auto stmt_delete = SetupSqlStatement(R"sql(
              delete from Badges
              where

                Badge in (?)

                and (
                  -- Likers over root comments must be above N
                  ifnull((select lc.Value from Ratings lc indexed by Ratings_Type_Id_Last_Value where lc.Type in (112) and lc.Last = 1 and lc.Id = Badges.AccountId),0) < ?

                  -- Sum liker must be above N
                  or ifnull((select sum(l.Value) from Ratings l where l.Type in (111,112,113) and l.Last = 1 and l.Id = Badges.AccountId),0) < ?

                  -- Account must be registered above N months
                  or ? - (select min(reg1.Height) from Transactions reg1 indexed by Transactions_Id where reg1.Id = Badges.AccountId) <= ?

                  -- Account must be active (not deleted)
                  or not exists (select 1 from Transactions u indexed by Transactions_Id_Last where u.Type = 100 and u.Last = 1 and u.Id = Badges.AccountId)
                )
            )sql");
            TryBindStatementInt(stmt_delete, 1, conditions.Number);
            TryBindStatementInt64(stmt_delete, 2, conditions.LikersComment);
            TryBindStatementInt64(stmt_delete, 3, conditions.LikersAll);
            TryBindStatementInt(stmt_delete, 4, height);
            TryBindStatementInt64(stmt_delete, 5, conditions.RegistrationDepth);
            TryStepStatement(stmt_delete);
            
            auto stmt_insert = SetupSqlStatement(R"sql(
              insert into Badges

              select

                lc.Id,
                ?

              from Ratings lc indexed by Ratings_Type_Id_Last_Value

              where

                not exists (select 1 from Badges b indexed by Badges_AccountId_Badge where b.AccountId = lc.Id and b.Badge = ?)

                -- The main filtering rule is performed by the main filter
                -- Likers over root comments must be above N
                and lc.Type = 112 and lc.Id > 0 and lc.Last = 1 and lc.Value >= ?
                
                -- Sum liker must be above N
                and ifnull((select sum(l.Value) from Ratings l indexed by Ratings_Type_Id_Last_Value where l.Type in (111,112,113) and l.Last = 1 and l.Id = lc.Id),0) >= ?

                -- Account must be registered above N months
                and ? - (select min(reg1.Height) from Transactions reg1 indexed by Transactions_Id where reg1.Id = lc.Id) > ?

                -- Account must be active
                and exists (select 1 from Transactions u indexed by Transactions_Id_Last where u.Type = 100 and u.Last = 1 and u.Id = lc.Id)
            )sql");
            TryBindStatementInt(stmt_insert, 1, conditions.Number);
            TryBindStatementInt(stmt_insert, 2, conditions.Number);
            TryBindStatementInt64(stmt_insert, 3, conditions.LikersComment);
            TryBindStatementInt64(stmt_insert, 3, conditions.LikersAll);
            TryBindStatementInt(stmt_insert, 4, height);
            TryBindStatementInt64(stmt_insert, 5, conditions.RegistrationDepth);
            TryStepStatement(stmt_insert);
        });
    }
    
    void ChainRepository::IndexBadges_Whale(int height, const BadgeWhaleConditions& conditions)
    {
        // TODO (aok): implement
    }
    
    void ChainRepository::IndexBadges_Moderator(int height, const BadgeModeratorConditions& conditions)
    {
        TryTransactionStep(__func__, [&]()
        {
            auto stmt_delete = SetupSqlStatement(R"sql(
              delete from Badges
              where

                Badge in (?)

                and (
                  -- Likers over root comments must be above N
                  ifnull((select lc.Value from Ratings lc indexed by Ratings_Type_Id_Last_Value where lc.Type in (112) and lc.Last = 1 and lc.Id = Badges.AccountId),0) < ?

                  -- Sum liker must be above N
                  or ifnull((select sum(l.Value) from Ratings l where l.Type in (111,112,113) and l.Last = 1 and l.Id = Badges.AccountId),0) < ?

                  -- Account must be registered above N months
                  or ? - (select min(reg1.Height) from Transactions reg1 indexed by Transactions_Id where reg1.Id = Badges.AccountId) <= ?

                  -- Account must be active (not deleted)
                  or not exists (select 1 from Transactions u indexed by Transactions_Id_Last where u.Type = 100 and u.Last = 1 and u.Id = Badges.AccountId)
                )
            )sql");
            TryBindStatementInt(stmt_delete, 1, conditions.Number);
            TryBindStatementInt64(stmt_delete, 2, conditions.LikersComment);
            TryBindStatementInt64(stmt_delete, 3, conditions.LikersAll);
            TryBindStatementInt(stmt_delete, 4, height);
            TryBindStatementInt64(stmt_delete, 5, conditions.RegistrationDepth);
            TryStepStatement(stmt_delete);
            
            auto stmt_insert = SetupSqlStatement(R"sql(
              insert into Badges

              select

                lc.Id,
                ?

              from Ratings lc indexed by Ratings_Type_Id_Last_Value

              where

                not exists (select 1 from Badges b indexed by Badges_AccountId_Badge where b.AccountId = lc.Id and b.Badge = ?)

                -- The main filtering rule is performed by the main filter
                -- Likers over root comments must be above N
                and lc.Type = 112 and lc.Id > 0 and lc.Last = 1 and lc.Value >= ?
                
                -- Sum liker must be above N
                and ifnull((select sum(l.Value) from Ratings l indexed by Ratings_Type_Id_Last_Value where l.Type in (111,112,113) and l.Last = 1 and l.Id = lc.Id),0) >= ?

                -- Account must be registered above N months
                and ? - (select min(reg1.Height) from Transactions reg1 indexed by Transactions_Id where reg1.Id = lc.Id) > ?

                -- Account must be active
                and exists (select 1 from Transactions u indexed by Transactions_Id_Last where u.Type = 100 and u.Last = 1 and u.Id = lc.Id)
            )sql");
            TryBindStatementInt(stmt_insert, 1, conditions.Number);
            TryBindStatementInt(stmt_insert, 2, conditions.Number);
            TryBindStatementInt64(stmt_insert, 3, conditions.LikersComment);
            TryBindStatementInt64(stmt_insert, 3, conditions.LikersAll);
            TryBindStatementInt64(stmt_insert, 4, height);
            TryBindStatementInt64(stmt_insert, 5, conditions.RegistrationDepth);
            TryStepStatement(stmt_insert);
        });
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
    
    void ChainRepository::ClearOldLast(const string& txHash)
    {
        auto stmt = SetupSqlStatement(R"sql(
            UPDATE Transactions indexed by Transactions_Id_Last SET
                Last = 0
            FROM (
                select t.Hash, t.id
                from Transactions t
                where   t.Hash = ?
            ) as tInner
            WHERE   Transactions.Id = tInner.Id
                and Transactions.Last = 1
                and Transactions.Hash != tInner.Hash
        )sql");

        TryBindStatementText(stmt, 1, txHash);
        TryStepStatement(stmt);
    }

    void ChainRepository::RestoreOldLast(int height)
    {
        int64_t nTime0 = GetTimeMicros();

        // ----------------------------------------
        // Restore old Last transactions
        auto stmt1 = SetupSqlStatement(R"sql(
            update Transactions indexed by Transactions_Height_Id
                set Last=1
            from (
                select t1.Id, max(t2.Height)Height
                from Transactions t1 indexed by Transactions_Last_Id_Height
                join Transactions t2 indexed by Transactions_Last_Id_Height on t2.Id = t1.Id and t2.Height < ? and t2.Last = 0
                where t1.Height >= ?
                  and t1.Last = 1
                group by t1.Id
            )t
            where Transactions.Id = t.Id and Transactions.Height = t.Height
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

                    b1.AddressHash
                    ,(
                        select max(b2.Height)
                        from Balances b2 indexed by Balances_AddressHash_Last_Height
                        where b2.AddressHash = b1.AddressHash
                          and b2.Last = 0
                          and b2.Height < ?
                        limit 1
                    )Height

                from Balances b1 indexed by Balances_Height

                where b1.Height >= ?
                  and b1.Last = 1
                  and b1.AddressHash != ''

                group by b1.AddressHash
            )b
            where b.Height is not null
              and Balances.AddressHash = b.AddressHash
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
            UPDATE Transactions SET
                BlockHash = null,
                BlockNum = null,
                Height = null,
                Id = null,
                Last = 0
            WHERE Height >= ?
        )sql");
        TryBindStatementInt(stmt0, 1, height);
        TryStepStatement(stmt0);

        int64_t nTime1 = GetTimeMicros();
        LogPrint(BCLog::BENCH, "        - RollbackHeight (Transactions:Height = null): %.2fms\n", 0.001 * (nTime1 - nTime0));

        // ----------------------------------------
        // Rollback spent transaction outputs
        auto stmt2 = SetupSqlStatement(R"sql(
            UPDATE TxOutputs SET
                SpentHeight = null,
                SpentTxHash = null
            WHERE SpentHeight >= ?
        )sql");
        TryBindStatementInt(stmt2, 1, height);
        TryStepStatement(stmt2);

        int64_t nTime2 = GetTimeMicros();
        LogPrint(BCLog::BENCH, "        - RollbackHeight (TxOutputs:SpentHeight = null): %.2fms\n", 0.001 * (nTime2 - nTime1));

        // ----------------------------------------
        // Rollback transaction outputs height
        auto stmt3 = SetupSqlStatement(R"sql(
            UPDATE TxOutputs SET
                TxHeight = null
            WHERE TxHeight >= ?
        )sql");
        TryBindStatementInt(stmt3, 1, height);
        TryStepStatement(stmt3);

        int64_t nTime3 = GetTimeMicros();
        LogPrint(BCLog::BENCH, "        - RollbackHeight (TxOutputs:TxHeight = null): %.2fms\n", 0.001 * (nTime3 - nTime2));

        // ----------------------------------------
        // Remove ratings
        auto stmt4 = SetupSqlStatement(R"sql(
            delete from Ratings
            where Height >= ?
        )sql");
        TryBindStatementInt(stmt4, 1, height);
        TryStepStatement(stmt4);

        int64_t nTime4 = GetTimeMicros();
        LogPrint(BCLog::BENCH, "        - RollbackHeight (Ratings delete): %.2fms\n", 0.001 * (nTime4 - nTime3));

        // ----------------------------------------
        // Remove balances
        auto stmt5 = SetupSqlStatement(R"sql(
            delete from Balances
            where Height >= ?
        )sql");
        TryBindStatementInt(stmt5, 1, height);
        TryStepStatement(stmt5);

        int64_t nTime5 = GetTimeMicros();
        LogPrint(BCLog::BENCH, "        - RollbackHeight (Balances delete): %.2fms\n", 0.001 * (nTime5 - nTime4));
    }

    void ChainRepository::RollbackBlockingList(int height)
    {
        int64_t nTime0 = GetTimeMicros();

        auto delListStmt = SetupSqlStatement(R"sql(
            delete from BlockingLists where ROWID in
            (
                select bl.ROWID
                from Transactions b indexed by Transactions_Type_Last_String1_Height_Id
                join Transactions us indexed by Transactions_Type_Last_String1_Height_Id
                  on us.Type in (100, 170) and us.Last = 1
                    and us.String1 = b.String1
                    and us.Height > 0
                join Transactions ut indexed by Transactions_Type_Last_String1_Height_Id
                  on ut.Type in (100, 170) and ut.Last = 1
                    and ut.String1 in (select b.String2 union select value from json_each(b.String3))
                    and ut.Height > 0
                join BlockingLists bl on bl.IdSource = us.Id and bl.IdTarget = ut.Id
                where b.Type in (305)
                  and b.Height >= ?
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
              us.Id,
              ut.Id
            from Transactions b indexed by Transactions_Type_Last_String1_Height_Id
            join Transactions us indexed by Transactions_Type_Last_String1_Height_Id
              on us.Type in (100, 170) and us.Last = 1 and us.String1 = b.String1 and us.Height > 0
            join Transactions ut indexed by Transactions_Type_Last_String1_Height_Id
              on ut.Type in (100, 170) and ut.Last = 1
                --and ut.String1 = b.String2
                and ut.String1 in (select b.String2 union select value from json_each(b.String3))
                and ut.Height > 0
            where b.Type in (306)
              and b.Height >= ?
              and not exists (select 1 from BlockingLists bl where bl.IdSource = us.Id and bl.IdTarget = ut.Id)
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
