// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 Bitcoin developers
// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETDB_CHAINREPOSITORY_HPP
#define POCKETDB_CHAINREPOSITORY_HPP

#include "pocketdb/helpers/TransactionHelper.hpp"
#include "pocketdb/repositories/BaseRepository.hpp"
#include "pocketdb/models/base/Rating.hpp"
#include "pocketdb/models/base/PocketTypes.hpp"
#include "pocketdb/models/base/ReturnDtoModels.hpp"

namespace PocketDb
{
    using std::runtime_error;

    using namespace PocketTx;

    class ChainRepository : public BaseRepository
    {
    public:
        explicit ChainRepository(SQLiteDatabase& db) : BaseRepository(db) {}

        void Init() override {}
        void Destroy() override {}

        // Update transactions set block hash & height
        // Also spent outputs
        void IndexBlock(const string& blockHash, int height, vector<TransactionIndexingInfo>& txs)
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
        }

        // Erase all calculated data great or equals block
        void RollbackBlock(int height)
        {
            // Update transactions
            auto transactionsStmt = SetupSqlStatement(R"sql(
                UPDATE Transactions SET
                    BlockHash = null,
                    BlockNum = null,
                    Height = null,
                    Id = null
                WHERE Height is not null and Height >= ?
            )sql");
            TryBindStatementInt(transactionsStmt, 1, height);
            TryTransactionStep(__func__, { transactionsStmt });

            // Update transaction outputs
            auto outputsStmt = SetupSqlStatement(R"sql(
                UPDATE TxOutputs SET
                    TxHeight = null,
                    SpentHeight = null,
                    SpentTxHash = null
                WHERE SpentHeight is not null and SpentHeight >= ?
            )sql");
            TryBindStatementInt(outputsStmt, 1, height);
            TryTransactionStep(__func__, { outputsStmt });

            // Remove ratings
            auto ratingsStmt = SetupSqlStatement(R"sql(
                DELETE FROM Ratings
                WHERE Height >= ?
            )sql");

            TryBindStatementInt(ratingsStmt, 1, height);
            TryTransactionStep(__func__, { ratingsStmt });
        }

    private:

        void UpdateTransactionHeight(const string& blockHash, int blockNumber, int height, const string& txHash)
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

            auto stmtOuts = SetupSqlStatement(R"sql(
                UPDATE TxOutputs SET
                    TxHeight = ?
                WHERE TxHash = ?
            )sql");
            TryBindStatementInt(stmtOuts, 1, height);
            TryBindStatementText(stmtOuts, 2, txHash);

            TryTransactionStep(__func__, { stmt, stmtOuts });
        }

        void UpdateTransactionOutputs(const TransactionIndexingInfo& txInfo, int height)
        {
            auto sql = R"sql(
                UPDATE TxOutputs SET
                    SpentHeight = ?,
                    SpentTxHash = ?
                WHERE TxHash = ? and Number = ?
            )sql";

            TryTransactionStep(__func__, [&]()
            {
                for (auto& input : txInfo.Inputs)
                {
                    auto stmt = SetupSqlStatement(sql);

                    TryBindStatementInt(stmt, 1, height);
                    TryBindStatementText(stmt, 2, txInfo.Hash);
                    TryBindStatementText(stmt, 3, input.first);
                    TryBindStatementInt(stmt, 4, input.second);

                    TryStepStatement(stmt);
                }
            });
        }

        void IndexAccount(const string& txHash)
        {
            // Get new ID or copy previous
            auto setIdStmt = SetupSqlStatement(R"sql(
                UPDATE Transactions SET
                    Id = ifnull(
                        -- copy self Id
                        (
                            select a.Id
                            from vAccounts a
                            where a.Type = Transactions.Type
                                and a.Last = 1
                                and a.Height is not null
                                and a.AddressHash = Transactions.String1
                            limit 1
                        ),
                        ifnull(
                            -- new record
                            (
                                select max( a.Id ) + 1
                                from vAccounts a
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

            // Clear old last records for set new last
            auto clearLastStmt = SetupSqlStatement(R"sql(
                UPDATE Transactions SET
                    Last = 0
                FROM (
                    select a.Hash, a.Type, a.AddressHash
                    from vAccounts a
                    where   a.Height is not null
                        and a.Hash = ?
                ) as account
                WHERE   Transactions.Hash != account.Hash
                    and Transactions.Type = account.Type
                    and Transactions.String1 = account.AddressHash
                    and Transactions.Height is not null
                    and Transactions.Last = 1
            )sql");
            TryBindStatementText(clearLastStmt, 1, txHash);

            // Execute all
            TryTransactionStep(__func__, { setIdStmt, clearLastStmt });
        }

        void IndexContent(const string& txHash)
        {
            // Get new ID or copy previous
            auto setIdStmt = SetupSqlStatement(R"sql(
                UPDATE Transactions SET
                    Id = ifnull(
                        -- copy self Id
                        (
                            select c.Id
                            from vContents c
                            where c.Type = Transactions.Type
                                and c.RootTxHash = Transactions.String2
                                and c.Height is not null
                                and c.Last = 1
                            limit 1
                        ),
                        -- new record
                        ifnull(
                            (
                                select max( c.Id ) + 1
                                from vContents c
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

            // Clear old last records for set new last
            auto clearLastStmt = SetupSqlStatement(R"sql(
                UPDATE Transactions SET
                    Last = 0
                FROM (
                    select c.Hash, c.Type, c.RootTxHash
                    from vContents c
                    where c.Height is not null
                        and c.Hash = ?
                ) as content
                WHERE   Transactions.Hash != content.Hash
                    and Transactions.Type = content.Type
                    and Transactions.String2 = content.RootTxHash
                    and Transactions.Height is not null
                    and Transactions.Last = 1
            )sql");
            TryBindStatementText(clearLastStmt, 1, txHash);

            TryTransactionStep(__func__, { setIdStmt, clearLastStmt });
        }

        void IndexComment(const string& txHash)
        {
            // Get new ID or copy previous
            auto setIdStmt = SetupSqlStatement(R"sql(
                UPDATE Transactions SET
                    Id = ifnull(
                        -- copy self Id
                        (
                            select max( c.Id )
                            from vComments c
                            where c.RootTxHash = Transactions.String2
                                and c.Height is not null
                                and c.Height <= Transactions.Height
                        ),
                        -- new record
                        ifnull(
                            (
                                select max( c.Id ) + 1
                                from vComments c
                                where c.Height is not null
                            ),
                            0 -- for first record
                        )
                    ),
                    Last = 1
                WHERE Hash = ?
            )sql");
            TryBindStatementText(setIdStmt, 1, txHash);

            // Clear old last records for set new last
            auto clearLastStmt = SetupSqlStatement(R"sql(
                UPDATE Transactions SET
                    Last = 0
                FROM (
                    select c.Hash, c.RootTxHash
                    from vComments c
                    where c.Height is not null
                        and c.Hash = ?
                ) as content
                WHERE   Transactions.Hash != content.Hash
                    and Transactions.Type in (204, 205, 206)
                    and Transactions.String2 = content.RootTxHash
                    and Transactions.Height is not null
                    and Transactions.Last = 1
            )sql");
            TryBindStatementText(clearLastStmt, 1, txHash);

            TryTransactionStep(__func__, { setIdStmt, clearLastStmt });
        }

        void IndexBlocking(const string& txHash)
        {
            // Set last=1 for new transaction
            auto setLastStmt = SetupSqlStatement(R"sql(
                UPDATE Transactions SET
                    Last = 1
                WHERE Hash = ?
            )sql");
            TryBindStatementText(setLastStmt, 1, txHash);

            // Clear old last records for set new last
            auto clearLastStmt = SetupSqlStatement(R"sql(
                UPDATE Transactions SET
                    Last = 0
                FROM (
                    select b.Hash, b.AddressHash, b.AddressToHash
                    from vBlockings b
                    where   b.Hash = ?
                        and b.Height is not null
                ) as blocking
                WHERE   Transactions.Hash != blocking.Hash
                    and Transactions.Type in (305, 306)
                    and Transactions.String1 = blocking.AddressHash
                    and Transactions.String2 = blocking.AddressToHash
                    and Transactions.Height is not null
                    and Transactions.Last = 1

            )sql");
            TryBindStatementText(clearLastStmt, 1, txHash);

            TryTransactionStep(__func__, { setLastStmt, clearLastStmt });
        }

        void IndexSubscribe(const string& txHash)
        {
            // Set last=1 for new transaction
            auto setLastStmt = SetupSqlStatement(R"sql(
                UPDATE Transactions SET
                    Last = 1
                WHERE Hash = ?
            )sql");
            TryBindStatementText(setLastStmt, 1, txHash);

            // Clear old last records for set new last
            auto clearLastStmt = SetupSqlStatement(R"sql(
                UPDATE Transactions SET
                    Last = 0
                FROM (
                    select s.Hash, s.AddressHash, s.AddressToHash
                    from vSubscribes s
                    where   s.Hash = ?
                        and s.Height is not null
                ) as subscribe
                WHERE   Transactions.Hash != subscribe.Hash
                    and Transactions.Type in (302, 303, 304)
                    and Transactions.String1 = subscribe.AddressHash
                    and Transactions.String2 = subscribe.AddressToHash
                    and Transactions.Height is not null
                    and Transactions.Last = 1

            )sql");
            TryBindStatementText(clearLastStmt, 1, txHash);

            TryTransactionStep(__func__, { setLastStmt, clearLastStmt });
        }

    };

} // namespace PocketDb

#endif // POCKETDB_CHAINREPOSITORY_HPP

