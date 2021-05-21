// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 Bitcoin developers
// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETDB_CHAINREPOSITORY_HPP
#define POCKETDB_CHAINREPOSITORY_HPP

#include "pocketdb/pocketnet.h"
#include "pocketdb/repositories/BaseRepository.hpp"
#include "pocketdb/models/base/Rating.hpp"
#include "pocketdb/models/base/SelectModels.hpp"

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
        bool UpdateHeight(string blockHash, int height, map<string, map<string, int>>& txs)
        {
            return TryTransactionStep([&]()
            {
                for (const auto& tx : txs)
                {
                    // All transactions must have a blockHash & height relation
                    UpdateTransactionHeight(blockHash, height, tx.first);

                    // The outputs are needed for the explorer
                    UpdateTransactionOutputs(blockHash, height, tx.second);

                    // All users must have a unique digital ID
                    UpdateUserId(tx.first);
                }
            });
        }

        // Rollback transaction-block link
        bool RollbackBlock(int height)
        {
            return TryTransactionStep([&]()
            {
                // Update transactions
                {
                    auto stmt = SetupSqlStatement(R"sql(
                        UPDATE Transactions SET
                            BlockHash = null,
                            Height = null
                        WHERE Height > ?
                    )sql");

                    auto heightPtr = make_shared<int>(height);
                    auto result = TryBindStatementInt(stmt, 1, heightPtr);
                    if (!result)
                        throw runtime_error(strprintf("%s: can't rollback chain (bind)\n", __func__));

                    if (!TryStepStatement(stmt))
                        throw runtime_error(strprintf("%s: can't rollback chain (step) Height > %d\n",
                            __func__, height));
                }

                // Update transaction outputs
                {
                    auto stmt = SetupSqlStatement(R"sql(
                        UPDATE TxOutputs SET
                            SpentHeight = null,
                            SpentTxHash = null
                        WHERE SpentHeight > ?
                    )sql");

                    auto heightPtr = make_shared<int>(height);
                    auto result = TryBindStatementInt(stmt, 1, heightPtr);
                    if (!result)
                        throw runtime_error(strprintf("%s: can't rollback chain (bind)\n", __func__));

                    if (!TryStepStatement(stmt))
                        throw runtime_error(strprintf("%s: can't rollback chain (step) Height > %d\n",
                            __func__, height));
                }

                // Remove ratings
                {
                    auto stmt = SetupSqlStatement(R"sql(
                        DELETE FROM Ratings
                        WHERE Height > ?
                    )sql");

                    auto heightPtr = make_shared<int>(height);
                    auto result = TryBindStatementInt(stmt, 1, heightPtr);
                    if (!result)
                        throw runtime_error(strprintf("%s: can't rollback ratings (bind)\n", __func__));

                    if (!TryStepStatement(stmt))
                        throw runtime_error(strprintf("%s: can't rollback ratings (step) Height > %d\n",
                            __func__, height));
                }
            });
        }

        // Accumulate new rating records
        bool InsertRatings(vector<shared_ptr<Rating>>& ratings)
        {
            return TryTransactionStep([&]()
            {
                for (const auto& rating : ratings)
                {
                    // Build sql statement with auto select IDs
                    auto stmt = SetupSqlStatement(R"sql(
                        INSERT OR FAIL INTO Ratings (
                            Type,
                            Height,
                            Hash,
                            Value
                        ) SELECT ?,?,?,?
                    )sql");

                    // Bind arguments
                    auto result = TryBindStatementInt(stmt, 1, rating->GetTypeInt());
                    result &= TryBindStatementInt(stmt, 2, rating->GetHeight());
                    result &= TryBindStatementText(stmt, 3, rating->GetHash());
                    result &= TryBindStatementInt64(stmt, 4, rating->GetValue());
                    if (!result)
                        throw runtime_error(strprintf("%s: can't insert in ratings (bind)\n", __func__));

                    // Try execute
                    if (!TryStepStatement(stmt))
                        throw runtime_error(strprintf("%s: can't insert in ratings (step) Type:%d Height:%d Hash:%s\n",
                            __func__, *rating->GetTypeInt(), *rating->GetHeight(), *rating->GetHash()));
                }
            });
        }

    private:

        void UpdateTransactionHeight(string blockHash, int height, string txHash)
        {
            auto stmt = SetupSqlStatement(R"sql(
                UPDATE Transactions SET
                    BlockHash=?,
                    Height=?
                WHERE Hash=?
            )sql");

            // Bind arguments
            auto blockHashPtr = make_shared<string>(blockHash);
            auto heightPtr = make_shared<int>(height);
            auto txHashPtr = make_shared<string>(txHash);

            auto result = TryBindStatementText(stmt, 1, blockHashPtr);
            result &= TryBindStatementInt(stmt, 2, heightPtr);
            result &= TryBindStatementText(stmt, 3, txHashPtr);
            if (!result)
                throw runtime_error(strprintf("%s: can't update transactions set height (bind)\n", __func__));

            // Try execute
            if (!TryStepStatement(stmt))
                throw runtime_error(strprintf("%s: can't update transactions set height (step) Hash:%s Block:%s Height:%d\n",
                    __func__, txHash, blockHash, height));
        }
        void UpdateTransactionOutputs(string txHash, int height, map<string, int> outputs)
        {
            for (auto& out : outputs)
            {
                auto stmt = SetupSqlStatement(R"sql(
                    UPDATE TxOutputs SET
                        SpentHeight=?,
                        SpentTxHash=?
                    WHERE TxHash=? and Number=?
                )sql");

                // Bind arguments
                auto spentHeightPtr = make_shared<int>(height);
                auto spentTxHashPtr = make_shared<string>(txHash);
                auto txHashPtr = make_shared<string>(out.first);
                auto numberPtr = make_shared<int>(out.second);

                auto result = TryBindStatementInt(stmt, 1, spentHeightPtr);
                result &= TryBindStatementText(stmt, 2, spentTxHashPtr);
                result &= TryBindStatementText(stmt, 3, txHashPtr);
                result &= TryBindStatementInt(stmt, 4, numberPtr);
                if (!result)
                    throw runtime_error(strprintf("%s: can't update txoutputs set height (bind)\n", __func__));

                // Try execute
                if (!TryStepStatement(stmt))
                    throw runtime_error(strprintf("%s: can't update txoutputs set height (step) Hash:%s Height:%d\n",
                        __func__, txHash, height));
            }
        }
        void UpdateUserId(string txHash)
        {
            auto stmt = SetupSqlStatement(R"sql(
                UPDATE Transactions SET
                    Id = ifnull(
                        (
                            select max( u.Id )
                            from Transactions u
                            where u.Type = Transactions.Type
                                and u.String1 = Transactions.String1
                                and u.Height is not null
                        ),
                        (
                            select count( 1 )
                            from Transactions u
                            where u.Type = Transactions.Type
                                and u.Height is not null
                                and u.Id is not null
                        )
                    )
                WHERE Hash=?
            )sql");

            // Bind arguments
            auto hashPtr = make_shared<string>(txHash);

            auto result = TryBindStatementText(stmt, 1, hashPtr);
            if (!result)
                throw runtime_error(strprintf("%s: can't update transactions set user Id (bind)\n", __func__));

            // Try execute
            if (!TryStepStatement(stmt))
                throw runtime_error(strprintf("%s: can't update transactions set user Id (step) Hash:%s\n",
                    __func__, txHash));
        }

    };

} // namespace PocketDb

#endif // POCKETDB_CHAINREPOSITORY_HPP

