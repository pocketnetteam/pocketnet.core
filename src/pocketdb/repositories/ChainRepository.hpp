// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 Bitcoin developers
// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETDB_CHAINREPOSITORY_HPP
#define POCKETDB_CHAINREPOSITORY_HPP

#include <util.h>

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
        void UpdateHeight(const string& blockHash, int height, vector<TransactionIndexingInfo>& txs)
        {
            for (const auto& tx : txs)
            {
                int64_t nTime1 = GetTimeMicros();

                // All transactions must have a blockHash & height relation
                UpdateTransactionHeight(blockHash, height, tx.Hash);

                int64_t nTime2 = GetTimeMicros();
                LogPrint(BCLog::BENCH, "      - UpdateTransactionHeight: %.2fms\n", 0.001 * (nTime2 - nTime1));

                // The outputs are needed for the explorer
                UpdateTransactionOutputs(blockHash, height, tx.Inputs);

                int64_t nTime3 = GetTimeMicros();
                LogPrint(BCLog::BENCH, "      - UpdateTransactionOutputs: %.2fms\n", 0.001 * (nTime3 - nTime2));

                // All accounts must have a unique digital ID
                if (PocketHelpers::IsAccount(tx))
                {
                    SetAccountId(tx.Hash);

                    int64_t nTime4 = GetTimeMicros();
                    LogPrint(BCLog::BENCH, "      - SetAccountId: %.2fms\n", 0.001 * (nTime4 - nTime3));
                }

                // All contents must have a unique digital ID
                if (PocketHelpers::IsContent(tx))
                {
                    SetContentId(tx.Hash);

                    int64_t nTime4 = GetTimeMicros();
                    LogPrint(BCLog::BENCH, "      - SetContentId: %.2fms\n", 0.001 * (nTime4 - nTime3));
                }
            }
        }

        // Erase all calculated data great or equals block
        void RollbackBlock(int height)
        {
            int64_t nTime1 = GetTimeMicros();

            // Update transactions
            TryTransactionStep([&]()
            {
                auto stmt = SetupSqlStatement(R"sql(
                    UPDATE Transactions SET
                        BlockHash = null,
                        Height = null,
                        Id = null
                    WHERE Height is not null and Height >= ?
                )sql");

                TryBindStatementInt(stmt, 1, height);
                TryStepStatement(stmt);
            });

            int64_t nTime2 = GetTimeMicros();
            LogPrint(BCLog::BENCH, "      - RollbackBlock (Chain): %.2fms\n", 0.001 * (nTime2 - nTime1));

            // Update transaction outputs
            TryTransactionStep([&]()
            {
                auto stmt = SetupSqlStatement(R"sql(
                    UPDATE TxOutputs SET
                        SpentHeight = null,
                        SpentTxHash = null
                    WHERE SpentHeight is not null and SpentHeight >= ?
                )sql");

                TryBindStatementInt(stmt, 1, height);
                TryStepStatement(stmt);
            });

            int64_t nTime3 = GetTimeMicros();
            LogPrint(BCLog::BENCH, "      - RollbackBlock (Outputs): %.2fms\n", 0.001 * (nTime3 - nTime2));

            // Remove ratings
            TryTransactionStep([&]()
            {
                auto stmt = SetupSqlStatement(R"sql(
                    DELETE FROM Ratings
                    WHERE Height >= ?
                )sql");

                TryBindStatementInt(stmt, 1, height);
                TryStepStatement(stmt);
            });

            int64_t nTime4 = GetTimeMicros();
            LogPrint(BCLog::BENCH, "      - RollbackBlock (Ratings): %.2fms\n", 0.001 * (nTime4 - nTime3));
        }

    private:

        void UpdateTransactionHeight(const string& blockHash, int height, const string& txHash)
        {
            auto sql = R"sql(
                UPDATE Transactions SET
                    BlockHash=?,
                    Height=?
                WHERE Hash=?
            )sql";

            TryTransactionStep([&]()
            {
                auto stmt = SetupSqlStatement(sql);

                TryBindStatementText(stmt, 1, blockHash);
                TryBindStatementInt(stmt, 2, height);
                TryBindStatementText(stmt, 3, txHash);

                TryStepStatement(stmt);
            });
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

        void SetAccountId(const string& txHash)
        {
            // Clear old last records for set new last
            auto clearLastStmt = SetupSqlStatement(R"sql(
                UPDATE Transactions SET
                    Last = 0
                WHERE String1 = (select a.AddressHash from vAccounts a where a.Hash = ?)
            )sql");
            TryBindStatementText(clearLastStmt, 1, txHash);

            // Get new ID or copy previous
            auto setIdStmt = SetupSqlStatement(R"sql(
                UPDATE Transactions SET
                    Id = ifnull(
                        -- copy self Id
                        (
                            select max( u.Id )
                            from Transactions u
                            where u.Type = Transactions.Type
                                and u.String1 = Transactions.String1
                                and u.Height is not null
                        ),
                        ifnull(
                            -- new record
                            (
                                select max( u.Id ) + 1
                                from Transactions u
                                where u.Type = Transactions.Type
                                    and u.Height is not null
                                    and u.Id is not null
                            ),
                            0 -- for first record
                        )
                    ),
                    Last = 1
                WHERE Hash = ?
            )sql");
            TryBindStatementText(setIdStmt, 1, txHash);
            
            TryStepStatements({ clearLastStmt, setIdStmt });
        }

        void SetContentId(const string& txHash)
        {
            auto sql = R"sql(
                UPDATE Transactions SET
                    Id = case
                        -- String2 (RootTxHash) empty - new content record
                        when Transactions.String2 is null then
                            ifnull(
                                (select max(c.Id) + 1
                                 from vContents c
                                 where c.Type = Transactions.Type
                                   and c.Id is not null)
                            , 0)
                        -- String2 (RootTxHash) not empty - edited content record
                        else
                            ifnull(
                                (select max(c.Id)
                                 from vContents c
                                 where c.Type = Transactions.Type
                                   and c.Hash = Transactions.String2
                                   and c.Id is not null)
                            , 0)
                    end
                WHERE Hash = ?
            )sql";

            TryTransactionStep([&]()
            {
                auto stmt = SetupSqlStatement(sql);
                TryBindStatementText(stmt, 1, txHash);
                TryStepStatement(stmt);
            });
        }

    };

} // namespace PocketDb

#endif // POCKETDB_CHAINREPOSITORY_HPP

