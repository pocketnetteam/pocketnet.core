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
                        IndexAccount(txInfo.Hash, txInfo.Type);

                    if (txInfo.IsContent())
                        IndexContent(txInfo.Hash, txInfo.Type);

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

    void ChainRepository::RollbackBatch(int height)
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

    bool ChainRepository::ClearDatabase()
    {
        LogPrintf("Rollback to first block. This can take from a few minutes to several hours, do not turn off your computer.\n");
        
        m_database.DropIndexes();
        
        RollbackBatch(0);
        
        m_database.CreateStructure();
    }

    bool ChainRepository::RollbackBlock(int height, vector<TransactionIndexingInfo>& txs)
    {
        if (height == 0)
            return ClearDatabase();

        try
        {
            // Update transactions
            TryTransactionStep(__func__, [&]()
            {
                // Restore previous Last
                for (const auto& tx : txs)
                    RollbackLast(tx);

                RollbackBatch(height);
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

    void ChainRepository::IndexAccount(const string& txHash, PocketTxType txType)
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
                            from Transactions a indexed by Transactions_Type_Id
                            where a.Type = Transactions.Type
                                and a.Height is not null
                                and a.Id is not null
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
        IndexLast(txHash, {(int)txType});
    }

    void ChainRepository::IndexContent(const string& txHash, PocketTxType txType)
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
                            from Transactions c indexed by Transactions_Type_Id
                            where c.Type = Transactions.Type
                                and c.Height is not null
                                and c.Id is not null
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
        IndexLast(txHash, {(int)txType});
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
                            from Transactions c indexed by Transactions_Type_Id
                            where c.Type in (204, 205, 206)
                                and c.Height is not null
                                and c.Id is not null
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
        IndexLast(txHash, {204, 205, 206});
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
                            from Transactions a indexed by Transactions_Type_Id
                            where a.Type in (305, 306)
                                and a.Height is not null
                                and a.Id is not null
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
        IndexLast(txHash, {305, 306});
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
                            from Transactions a indexed by Transactions_Type_Id
                            where a.Type in (302, 303, 304)
                                and a.Height is not null
                                and a.Id is not null
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
        IndexLast(txHash, {302, 303, 304});
    }

    void ChainRepository::IndexLast(const string& txHash, const vector<int>& txTypes)
    {
        auto stmt = SetupSqlStatement(R"sql(
            UPDATE Transactions indexed by Transactions_LastById SET
                Last = 0
            FROM (
                select t.Hash, t.id
                from Transactions t
                where   t.Hash = ?
            ) as tInner
            WHERE   Transactions.Type = in ( )sql" + join(txTypes | transformed(static_cast<string(*)(int)>(to_string)), ",") + R"sql( )
                and Transactions.Last = 1
                and Transactions.Id = tInner.Id
                and Transactions.Height is not null
                and Transactions.Hash != tInner.Hash
        )sql");

        TryBindStatementText(stmt, 1, txHash);
        TryStepStatement(stmt);
    }

    void ChainRepository::RollbackLast(const TransactionIndexingInfo& txInfo)
    {
        vector<int> txTypes;
        switch (txInfo.Type)
        {
            case PocketTxType::ACCOUNT_USER:
            case PocketTxType::ACCOUNT_VIDEO_SERVER:
            case PocketTxType::ACCOUNT_MESSAGE_SERVER:
                txTypes = {PocketTxType::ACCOUNT_USER, PocketTxType::ACCOUNT_VIDEO_SERVER, PocketTxType::ACCOUNT_MESSAGE_SERVER};
                break;
            case PocketTxType::CONTENT_POST:
            case PocketTxType::CONTENT_VIDEO:
            case PocketTxType::CONTENT_TRANSLATE:
                txTypes = {PocketTxType::CONTENT_POST, PocketTxType::CONTENT_VIDEO, PocketTxType::CONTENT_TRANSLATE};
                break;
            case PocketTxType::CONTENT_COMMENT:
            case PocketTxType::CONTENT_COMMENT_EDIT:
            case PocketTxType::CONTENT_COMMENT_DELETE:
                txTypes = {PocketTxType::CONTENT_COMMENT, PocketTxType::CONTENT_COMMENT_EDIT, PocketTxType::CONTENT_COMMENT_DELETE};
                break;
            case PocketTxType::ACTION_BLOCKING:
            case PocketTxType::ACTION_BLOCKING_CANCEL:
                txTypes = {PocketTxType::ACTION_BLOCKING, PocketTxType::ACTION_BLOCKING_CANCEL};
                break;
            case PocketTxType::ACTION_SUBSCRIBE:
            case PocketTxType::ACTION_SUBSCRIBE_CANCEL:
            case PocketTxType::ACTION_SUBSCRIBE_PRIVATE:
                txTypes = {PocketTxType::ACTION_SUBSCRIBE, PocketTxType::ACTION_SUBSCRIBE_CANCEL, PocketTxType::ACTION_SUBSCRIBE_PRIVATE};
                break;
            default:
                return;
        }
        
        auto stmt = SetupSqlStatement(R"sql(
            update Transactions
                set Last = 1
            from (
                select t.Hash
                from Transactions t indexed by Transactions_LastById
                where t.Type in ( )sql" + join(txTypes | transformed(static_cast<string(*)(int)>(to_string)), ",") + R"sql( )
                    and t.Last = 0
                    and t.Id = (select t1.id from Transactions t1 where t1.Hash = ?)
                order by t.Height desc
                limit 1
            ) as tInner
            where Transactions.Hash = tInner.Hash
        )sql");

        TryBindStatementText(stmt, 1, txInfo.Hash);
        TryStepStatement(stmt);
    }

} // namespace PocketDb
