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
#include "pocketdb/models/dto/ReturnDtoModels.hpp"

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
        bool UpdateHeight(string blockHash, int height, vector<TransactionIndexingInfo>& txs)
        {
            return TryTransactionStep([&]()
            {
                for (const auto& tx : txs)
                {
                    // All transactions must have a blockHash & height relation
                    UpdateTransactionHeight(blockHash, height, tx.Hash);

                    // The outputs are needed for the explorer
                    UpdateTransactionOutputs(blockHash, height, tx.Inputs);

                    // All users must have a unique digital ID
                    if (tx.Type == PocketTxType::ACCOUNT_USER
                        || tx.Type == PocketTxType::ACCOUNT_VIDEO_SERVER
                        || tx.Type == PocketTxType::ACCOUNT_MESSAGE_SERVER
                        || tx.Type == PocketTxType::CONTENT_POST
                        || tx.Type == PocketTxType::CONTENT_COMMENT
                        || tx.Type == PocketTxType::CONTENT_VIDEO
                        || tx.Type == PocketTxType::CONTENT_TRANSLATE)
                    {
                        UpdateShortId(tx.Hash);
                    }
                }
            });
        }

        // Rollback transaction-block link
        bool RollbackBlock(int height)
        {
            return TryTransactionStep([&]()
            {
                auto heightPtr = make_shared<int>(height);

                // Update transactions
                {
                    auto stmt = SetupSqlStatement(R"sql(
                        UPDATE Transactions SET
                            BlockHash = null,
                            Height = null,
                            Id = null
                        WHERE Height is not null and Height >= ?
                    )sql");

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
                        WHERE SpentHeight is not null and SpentHeight >= ?
                    )sql");

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
                        WHERE Height >= ?
                    )sql");

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
        bool InsertRatings(vector<Rating>& ratings)
        {
            return TryTransactionStep([&]()
            {
                for (const auto& rating : ratings)
                {
                    if (*rating.GetType() != RatingType::RATING_ACCOUNT_LIKERS)
                        InsertRating(rating);
                    else
                        InsertLiker(rating);
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
                throw runtime_error(
                    strprintf("%s: can't update transactions set height (step) Hash:%s Block:%s Height:%d\n",
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
        void UpdateShortId(string txHash)
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
                throw runtime_error(strprintf("%s: can't update transactions set Id (bind)\n", __func__));

            // Try execute
            if (!TryStepStatement(stmt))
                throw runtime_error(strprintf("%s: can't update transactions set Id (step) Hash:%s\n",
                    __func__, txHash));
        }
        void InsertRating(Rating rating)
        {
            // Build sql statement with auto select IDs
            auto stmt = SetupSqlStatement(R"sql(
                INSERT OR FAIL INTO Ratings (
                    Type,
                    Height,
                    Id,
                    Value
                ) SELECT ?,?,?,?
            )sql");

            // Bind arguments
            auto result = TryBindStatementInt(stmt, 1, rating.GetTypeInt());
            result &= TryBindStatementInt(stmt, 2, rating.GetHeight());
            result &= TryBindStatementInt64(stmt, 3, rating.GetId());
            result &= TryBindStatementInt64(stmt, 4, rating.GetValue());
            if (!result)
                throw runtime_error(strprintf("%s: can't insert in ratings (bind)\n", __func__));

            // Try execute
            if (!TryStepStatement(stmt))
                throw runtime_error(strprintf("%s: can't insert in ratings (step) Type:%d Height:%d Hash:%s\n",
                    __func__, *rating.GetTypeInt(), *rating.GetHeight(), *rating.GetId()));
        }
        void InsertLiker(Rating rating)
        {
            // Build sql statement with auto select IDs
            auto stmt = SetupSqlStatement(R"sql(
                INSERT OR FAIL INTO Ratings (
                    Type,
                    Height,
                    Id,
                    Value
                ) SELECT ?,?,?,?
                WHERE NOT EXISTS (select 1 from Ratings r where r.Type=? and r.Id=? and r.Value=?)
            )sql");

            // Bind arguments
            auto result = TryBindStatementInt(stmt, 1, rating.GetTypeInt());
            result &= TryBindStatementInt(stmt, 2, rating.GetHeight());
            result &= TryBindStatementInt64(stmt, 3, rating.GetId());
            result &= TryBindStatementInt64(stmt, 4, rating.GetValue());
            result &= TryBindStatementInt(stmt, 5, rating.GetTypeInt());
            result &= TryBindStatementInt64(stmt, 6, rating.GetId());
            result &= TryBindStatementInt64(stmt, 7, rating.GetValue());
            if (!result)
                throw runtime_error(strprintf("%s: can't insert in likers (bind)\n", __func__));

            // Try execute
            if (!TryStepStatement(stmt))
                throw runtime_error(strprintf("%s: can't insert in likers (step) Type:%d Height:%d Hash:%s Value:%s\n",
                    __func__, *rating.GetTypeInt(), *rating.GetHeight(), *rating.GetId(), *rating.GetValue()));
        }

    };

} // namespace PocketDb

#endif // POCKETDB_CHAINREPOSITORY_HPP

