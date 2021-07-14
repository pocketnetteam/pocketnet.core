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
                int64_t nTime1 = GetTimeMicros();

                // All transactions must have a blockHash & height relation
                UpdateTransactionHeight(blockHash, height, txInfo.Hash);

                int64_t nTime2 = GetTimeMicros();
                LogPrint(BCLog::BENCH, "      - UpdateTransactionHeight: %.2fms _ %s\n",
                    0.001 * (nTime2 - nTime1), txInfo.Hash);

                // The outputs are needed for the explorer
                UpdateTransactionOutputs(txInfo.Hash, height, txInfo.Inputs);

                int64_t nTime3 = GetTimeMicros();
                LogPrint(BCLog::BENCH, "      - UpdateTransactionOutputs: %.2fms _ %s\n",
                    0.001 * (nTime3 - nTime2), txInfo.Hash);

                // Account and Content must have unique ID
                // Also all edited transactions must have Last=(0/1) field
                {
                    if (txInfo.IsAccount())
                    {
                        IndexAccount(txInfo.Hash);

                        int64_t nTime4 = GetTimeMicros();
                        LogPrint(BCLog::BENCH, "      - SetAccountId: %.2fms _ %s\n",
                            0.001 * (nTime4 - nTime3), txInfo.Hash);
                    }

                    if (txInfo.IsContent())
                    {
                        IndexContent(txInfo.Hash);

                        int64_t nTime4 = GetTimeMicros();
                        LogPrint(BCLog::BENCH, "      - SetContentId: %.2fms _ %s\n",
                            0.001 * (nTime4 - nTime3), txInfo.Hash);
                    }

                    if (txInfo.IsBlocking())
                    {
                        IndexBlocking(txInfo.Hash);

                        int64_t nTime4 = GetTimeMicros();
                        LogPrint(BCLog::BENCH, "      - SetBlockingId: %.2fms _ %s\n",
                            0.001 * (nTime4 - nTime3), txInfo.Hash);
                    }

                    if (txInfo.IsSubscribe())
                    {
                        IndexSubscribe(txInfo.Hash);

                        int64_t nTime4 = GetTimeMicros();
                        LogPrint(BCLog::BENCH, "      - SetSubscribeId: %.2fms _ %s\n",
                            0.001 * (nTime4 - nTime3), txInfo.Hash);
                    }
                }
            }
        }

        // Erase all calculated data great or equals block
        void RollbackBlock(int height)
        {
            int64_t nTime1 = GetTimeMicros();

            // Update transactions
            auto transactionsStmt = SetupSqlStatement(R"sql(
                UPDATE Transactions SET
                    BlockHash = null,
                    Height = null,
                    Id = null
                WHERE Height is not null and Height >= ?
            )sql");
            TryBindStatementInt(transactionsStmt, 1, height);
            TryTransactionStep({ transactionsStmt });

            int64_t nTime2 = GetTimeMicros();
            LogPrint(BCLog::BENCH, "      - RollbackBlock (Chain): %.2fms _ %d\n",
                0.001 * (nTime2 - nTime1), height);

            // Update transaction outputs
            auto outputsStmt = SetupSqlStatement(R"sql(
                UPDATE TxOutputs SET
                    SpentHeight = null,
                    SpentTxHash = null
                WHERE SpentHeight is not null and SpentHeight >= ?
            )sql");
            TryBindStatementInt(outputsStmt, 1, height);
            TryTransactionStep({ outputsStmt });

            int64_t nTime3 = GetTimeMicros();
            LogPrint(BCLog::BENCH, "      - RollbackBlock (Outputs): %.2fms _ %d\n",
                0.001 * (nTime3 - nTime2), height);

            // Remove ratings
            auto ratingsStmt = SetupSqlStatement(R"sql(
                DELETE FROM Ratings
                WHERE Height >= ?
            )sql");

            TryBindStatementInt(ratingsStmt, 1, height);
            TryTransactionStep({ ratingsStmt });

            int64_t nTime4 = GetTimeMicros();
            LogPrint(BCLog::BENCH, "      - RollbackBlock (Ratings): %.2fms _ %d\n",
                0.001 * (nTime4 - nTime3), height);
        }

    private:

        void UpdateTransactionHeight(const string& blockHash, int height, const string& txHash)
        {
            auto stmt = SetupSqlStatement(R"sql(
                UPDATE Transactions SET
                    BlockHash=?,
                    Height=?
                WHERE Hash=?
            )sql");

            TryBindStatementText(stmt, 1, blockHash);
            TryBindStatementInt(stmt, 2, height);
            TryBindStatementText(stmt, 3, txHash);

            TryTransactionStep({ stmt });
        }

        void UpdateTransactionOutputs(const string& txHash, int height, const map<string, int>& outputs)
        {
            auto sql = R"sql(
                UPDATE TxOutputs SET
                    SpentHeight=?,
                    SpentTxHash=?
                WHERE TxHash=? and Number=?
            )sql";

            TryTransactionStep([&]()
            {
                for (auto& out : outputs)
                {
                    auto stmt = SetupSqlStatement(sql);

                    TryBindStatementInt(stmt, 1, height);
                    TryBindStatementText(stmt, 2, txHash);
                    TryBindStatementText(stmt, 3, out.first);
                    TryBindStatementInt(stmt, 4, out.second);

                    TryStepStatement(stmt);
                }
            });
        }

        void IndexAccount(const string& txHash)
        {
            // Clear old last records for set new last
            auto clearLastStmt = SetupSqlStatement(R"sql(
                UPDATE Transactions SET
                    Last = 0
                FROM (
                    select a.Hash, a.Type, a.AddressHash
                    from vAccounts a
                    where a.Hash = ?
                ) as account
                WHERE   Transactions.Hash != account.Hash
                    and Transactions.Type = account.Type
                    and Transactions.String1 = account.AddressHash
                    and Transactions.Last = 1
            )sql");
            TryBindStatementText(clearLastStmt, 1, txHash);

            // Get new ID or copy previous
            auto setIdStmt = SetupSqlStatement(R"sql(
                UPDATE Transactions SET
                    Id = ifnull(
                        -- copy self Id
                        (
                            select max( a.Id )
                            from vAccounts a
                            where a.Type = Transactions.Type
                                and a.AddressHash = Transactions.String1
                        ),
                        ifnull(
                            -- new record
                            (
                                select max( a.Id ) + 1
                                from vAccounts a
                                where a.Type = Transactions.Type
                                    and a.Id is not null
                            ),
                            0 -- for first record
                        )
                    ),
                    Last = 1
                WHERE Hash = ?
            )sql");
            TryBindStatementText(setIdStmt, 1, txHash);

            // Execute all
            TryTransactionStep({ clearLastStmt, setIdStmt });
        }

        void IndexContent(const string& txHash)
        {
            // Clear old last records for set new last
            auto clearLastStmt = SetupSqlStatement(R"sql(
                UPDATE Transactions SET
                    Last = 0
                FROM (
                    select c.Hash, c.Type, c.RootTxHash
                    from vContents c
                    where c.Hash = ?
                        and c.RootTxHash is not null
                ) as content
                WHERE   Transactions.Type = content.Type
                    and Transactions.String2 = content.RootTxHash
                    and Transactions.Hash != content.Hash
                    and Transactions.Last = 1
            )sql");
            TryBindStatementText(clearLastStmt, 1, txHash);

            // Get new ID or copy previous
            auto setIdStmt = SetupSqlStatement(R"sql(
                UPDATE Transactions SET
                    Id = ifnull(
                        -- copy self Id
                        (
                            select max( c.Id )
                            from vContents c
                            where c.Type = Transactions.Type
                                and c.RootTxHash = Transactions.String2
                                and c.Height <= Transactions.Height
                        ),
                        -- new record
                        ifnull(
                            (
                                select max( c.Id ) + 1
                                from vContents c
                                where c.Type = Transactions.Type
                            ),
                            0 -- for first record
                        )
                    ),
                    Last = 1
                WHERE Hash = ?
            )sql");
            TryBindStatementText(setIdStmt, 1, txHash);

            TryTransactionStep({ clearLastStmt, setIdStmt });
        }

        void IndexBlocking(const string& txHash)
        {
            // Clear old last records for set new last
            auto clearLastStmt = SetupSqlStatement(R"sql(
                UPDATE Transactions SET
                    Last = 0
                FROM (
                    select b.Hash, b.Type, b.AddressHash, b.AddressToHash
                    from vBlockings b
                    where b.Hash = ?
                ) as blocking
                WHERE   Transactions.Hash != blocking.Hash
                    and Transactions.Type = blocking.Type
                    and Transactions.String1 = blocking.AddressHash
                    and Transactions.String2 = blocking.AddressToHash
                    and Transactions.Last = 1

            )sql");
            TryBindStatementText(clearLastStmt, 1, txHash);

            // Get new ID or copy previous
            auto setLastStmt = SetupSqlStatement(R"sql(
                UPDATE Transactions SET
                    Last = 1
                WHERE Hash = ?
            )sql");
            TryBindStatementText(setLastStmt, 1, txHash);

            TryTransactionStep({ clearLastStmt, setLastStmt });
        }

        void IndexSubscribe(const string& txHash)
        {
            // Clear old last records for set new last
            auto clearLastStmt = SetupSqlStatement(R"sql(
                UPDATE Transactions SET
                    Last = 0
                FROM (
                    select s.Hash, s.Type, s.AddressHash, s.AddressToHash
                    from vSubscribes s
                    where s.Hash = ?
                ) as subscribe
                WHERE   Transactions.Hash != subscribe.Hash
                    and Transactions.Type = subscribe.Type
                    and Transactions.String1 = subscribe.AddressHash
                    and Transactions.String2 = subscribe.AddressToHash
                    and Transactions.Last = 1

            )sql");
            TryBindStatementText(clearLastStmt, 1, txHash);

            // Get new ID or copy previous
            auto setLastStmt = SetupSqlStatement(R"sql(
                UPDATE Transactions SET
                    Last = 1
                WHERE Hash = ?
            )sql");
            TryBindStatementText(setLastStmt, 1, txHash);

            TryTransactionStep({ clearLastStmt, setLastStmt });
        }

    };

} // namespace PocketDb

#endif // POCKETDB_CHAINREPOSITORY_HPP

