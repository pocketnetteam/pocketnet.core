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
        bool UpdateHeight(const string& blockHash, int height, vector<TransactionIndexingInfo>& txs)
        {
            for (const auto& tx : txs)
            {
                auto result = TryTransactionStep([&]()
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

                        int64_t nTime4 = GetTimeMicros();
                        LogPrint(BCLog::BENCH, "      - UpdateShortId: %.2fms\n", 0.001 * (nTime4 - nTime3));
                    }
                });

                if (!result)
                    return false;
            }

            return true;
        }

        // Erase all calculated data great or equals block
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

    private:

        void UpdateTransactionHeight(const string& blockHash, int height, const string& txHash)
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

        void UpdateTransactionOutputs(const string& txHash, int height, const map<string, int>& outputs)
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

        void UpdateShortId(const string& txHash)
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

    };

} // namespace PocketDb

#endif // POCKETDB_CHAINREPOSITORY_HPP

