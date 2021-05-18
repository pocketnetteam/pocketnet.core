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

        // Link transactions with block hash & height
        bool InsertBlock(string blockHash, int height, vector<string>& txs)
        {
            return TryTransactionStep([&]()
            {
                for (const auto& txHash : txs)
                {
                    // Build sql statement with auto select IDs
                    auto stmt = SetupSqlStatement(R"sql(
                        INSERT OR FAIL INTO Chain (
                            TxHash,
                            BlockHash,
                            Height
                        ) SELECT ?,?,?
                    )sql");

                    // Bind arguments
                    auto txHashPtr = make_shared<string>(txHash);
                    auto blockHashPtr = make_shared<string>(blockHash);
                    auto heightPtr = make_shared<int>(height);

                    auto result = TryBindStatementText(stmt, 1, txHashPtr);
                    result &= TryBindStatementText(stmt, 2, blockHashPtr);
                    result &= TryBindStatementInt(stmt, 3, heightPtr);
                    if (!result)
                        throw runtime_error(strprintf("%s: can't insert in chain (bind)\n", __func__));

                    // Try execute
                    if (!TryStepStatement(stmt))
                        throw runtime_error(strprintf("%s: can't insert in chain (step) Hash:%s Block:%s Height:%d\n",
                            __func__, txHash, blockHash, height));
                }
            });
        }

        // Rollback transaction-block link
        bool RollbackBlock(int height)
        {
            return TryTransactionStep([&]()
            {
                // Remove blocks from chain
                {
                    auto stmt = SetupSqlStatement(R"sql(
                        DELETE FROM Chain
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


    };

} // namespace PocketDb

#endif // POCKETDB_CHAINREPOSITORY_HPP

