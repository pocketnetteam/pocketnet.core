// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include "pocketdb/repositories/ChainRepository.h"

namespace PocketDb
{
    void ChainRepository::IndexBlock(const string& blockHash, int height, vector<TransactionIndexingInfo>& txs)
    {
        TryTransactionStep(__func__, [&]()
        {
            for (const auto& txInfo : txs)
            {
                // All transactions must have a blockHash & height relation
                UpdateTransactionHeight(blockHash, txInfo.BlockNumber, height, txInfo.Hash);

                // The outputs are needed for the explorer
                UpdateTransactionOutputs(txInfo, height);

                // Account and Content must have unique ID
                // Also all edited transactions must have Last=(0/1) field
                {
                    if (txInfo.IsAccount())
                        IndexAccount(txInfo.Hash);

                    if (txInfo.IsContent())
                        IndexContent(txInfo.Hash);

                    if (txInfo.IsComment())
                        IndexComment(txInfo.Hash);

                    if (txInfo.IsBlocking())
                        IndexBlocking(txInfo.Hash);

                    if (txInfo.IsSubscribe())
                        IndexSubscribe(txInfo.Hash);
                }
            }
        });
    }

    bool ChainRepository::ClearDatabase()
    {
        LogPrintf("Full reindexing database. This can take several hours.\n");

        LogPrintf("Deleting database indexes..\n");
        m_database.DropIndexes();
        
        LogPrintf("Rollback to first block..\n");
        RollbackHeight(0);
        
        m_database.CreateStructure();

        return true;
    }

    bool ChainRepository::Rollback(int height)
    {
        if (height == 0)
            return ClearDatabase();

        try
        {
            // Update transactions
            TryTransactionStep(__func__, [&]()
            {
                RestoreOldLast(height);
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

    void ChainRepository::UpdateTransactionHeight(const string& blockHash, int blockNumber, int height, const string& txHash)
    {
        auto stmt = SetupSqlStatement(R"sql(
            UPDATE Transactions SET
                BlockHash = ?,
                BlockNum = ?,
                Height = ?
            WHERE Hash = ?
        )sql");
        TryBindStatementText(stmt, 1, blockHash);
        TryBindStatementInt(stmt, 2, blockNumber);
        TryBindStatementInt(stmt, 3, height);
        TryBindStatementText(stmt, 4, txHash);
        TryStepStatement(stmt);

        auto stmtOuts = SetupSqlStatement(R"sql(
            UPDATE TxOutputs SET
                TxHeight = ?
            WHERE TxHash = ?
        )sql");
        TryBindStatementInt(stmtOuts, 1, height);
        TryBindStatementText(stmtOuts, 2, txHash);
        TryStepStatement(stmtOuts);

        LogPrint(BCLog::BENCH, "      - TryTransactionStep (UpdateTransactionHeight %d): %s\n", height, txHash);
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

    void ChainRepository::IndexAccount(const string& txHash)
    {
        // Get new ID or copy previous
        auto setIdStmt = SetupSqlStatement(R"sql(
            UPDATE Transactions SET
                Id = ifnull(
                    -- copy self Id
                    (
                        select a.Id
                        from Transactions a indexed by Transactions_LastAccount
                        where a.Type = Transactions.Type
                            and a.Last = 1
                            -- String1 = AddressHash
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
                        from Transactions c indexed by Transactions_LastContent
                        where c.Type = Transactions.Type
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
                        from Transactions c indexed by Transactions_LastContent
                        where c.Type in (204, 205, 206)
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
        // Set last=1 for new transaction
        auto setLastStmt = SetupSqlStatement(R"sql(
            UPDATE Transactions SET
                Id = ifnull(
                    -- copy self Id
                    (
                        select a.Id
                        from Transactions a indexed by Transactions_LastAction
                        where a.Type in (305, 306)
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

    void ChainRepository::IndexSubscribe(const string& txHash)
    {
        // Set last=1 for new transaction
        auto setLastStmt = SetupSqlStatement(R"sql(
            UPDATE Transactions SET
                Id = ifnull(
                    -- copy self Id
                    (
                        select a.Id
                        from Transactions a indexed by Transactions_LastAction
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
        // Restore old Last transactions
        auto stmt = SetupSqlStatement(R"sql(
            update Transactions set Last=1
            from (
                select t1.Id, max(t2.Height)Height
                from Transactions t1 indexed by Transactions_Last_Id_Height
                join Transactions t2 on t2.Id = t1.Id and t2.Height < ? and t2.Last = 0
                where t1.Height >= ?
                  and t1.Last = 1
                group by t1.Id
            )t
            where Transactions.Id = t.Id and Transactions.Height = t.Height
        )sql");
        TryBindStatementInt(stmt, 1, height);
        TryStepStatement(stmt);
    }

    void ChainRepository::RollbackHeight(int height)
    {
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

        // Rollback spent transaction outputs
        auto stmt1 = SetupSqlStatement(R"sql(
            UPDATE TxOutputs SET
                SpentHeight = null,
                SpentTxHash = null
            WHERE SpentHeight >= ?
        )sql");
        TryBindStatementInt(stmt1, 1, height);
        TryStepStatement(stmt1);

        // Rollback transaction outputs height
        auto stmt11 = SetupSqlStatement(R"sql(
            UPDATE TxOutputs SET
                TxHeight = null
            WHERE TxHeight >= ?
        )sql");
        TryBindStatementInt(stmt11, 1, height);
        TryStepStatement(stmt11);

        // Remove ratings
        auto stmt2 = SetupSqlStatement(R"sql(
            DELETE FROM Ratings
            WHERE Height >= ?
        )sql");
        TryBindStatementInt(stmt2, 1, height);
        TryStepStatement(stmt2);
    }


} // namespace PocketDb
