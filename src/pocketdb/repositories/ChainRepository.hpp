// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 Bitcoin developers
// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETDB_CHAINREPOSITORY_HPP
#define POCKETDB_CHAINREPOSITORY_HPP

#include <util.h>

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
        bool UpdateHeight(const string& blockHash, int height, vector<TransactionIndexingInfo>& txs)
        {
            for (const auto& tx : txs)
            {
                int64_t nTime1 = GetTimeMicros();

                // All transactions must have a blockHash & height relation
                if (!UpdateTransactionHeight(blockHash, height, tx.Hash))
                {
                    LogPrintf("UpdateHeight::UpdateTransactionHeight failed for block:%s height:%d tx:%s\n",
                        blockHash, height, tx.Hash);
                    return false;
                }

                int64_t nTime2 = GetTimeMicros();
                LogPrint(BCLog::BENCH, "      - UpdateTransactionHeight: %.2fms\n", 0.001 * (nTime2 - nTime1));

                // The outputs are needed for the explorer
                if (!UpdateTransactionOutputs(blockHash, height, tx.Inputs))
                {
                    LogPrintf("UpdateHeight::UpdateTransactionOutputs failed for block:%s height:%d tx:%s\n",
                        blockHash, height, tx.Hash);
                    return false;
                }

                int64_t nTime3 = GetTimeMicros();
                LogPrint(BCLog::BENCH, "      - UpdateTransactionOutputs: %.2fms\n", 0.001 * (nTime3 - nTime2));

                // All accounts must have a unique digital ID
                if (tx.Type == PocketTxType::ACCOUNT_USER ||
                    tx.Type == PocketTxType::ACCOUNT_VIDEO_SERVER ||
                    tx.Type == PocketTxType::ACCOUNT_MESSAGE_SERVER)
                {
                    if (!SetAccountId(tx.Hash))
                    {
                        LogPrintf("UpdateHeight::SetAccountId failed for block:%s height:%d tx:%s\n",
                            blockHash, height, tx.Hash);
                        return false;
                    }

                    int64_t nTime4 = GetTimeMicros();
                    LogPrint(BCLog::BENCH, "      - SetAccountId: %.2fms\n", 0.001 * (nTime4 - nTime3));
                }

                // All contents must have a unique digital ID
                if (tx.Type == PocketTxType::CONTENT_POST ||
                    tx.Type == PocketTxType::CONTENT_COMMENT ||
                    tx.Type == PocketTxType::CONTENT_VIDEO ||
                    tx.Type == PocketTxType::CONTENT_TRANSLATE)
                {
                    if (!SetContentId(tx.Hash))
                    {
                        LogPrintf("UpdateHeight::SetContentId failed for block:%s height:%d tx:%s\n",
                            blockHash, height, tx.Hash);
                        return false;
                    }

                    int64_t nTime4 = GetTimeMicros();
                    LogPrint(BCLog::BENCH, "      - SetContentId: %.2fms\n", 0.001 * (nTime4 - nTime3));
                }
            }

            return true;
        }

        // Erase all calculated data great or equals block
        bool RollbackBlock(int height)
        {
            auto heightPtr = make_shared<int>(height);

            int64_t nTime1 = GetTimeMicros();

            // Update transactions
            if (!TryTransactionStep([&]()
            {
                auto stmt = SetupSqlStatement(R"sql(
                    UPDATE Transactions SET
                        BlockHash = null,
                        Height = null,
                        Id = null
                    WHERE Height is not null and Height >= ?
                )sql");

                return TryBindStatementInt(stmt, 1, heightPtr) && TryStepStatement(stmt);
            }))
                return false;

            int64_t nTime2 = GetTimeMicros();
            LogPrint(BCLog::BENCH, "      - RollbackBlock (Chain): %.2fms\n", 0.001 * (nTime2 - nTime1));

            // Update transaction outputs
            if (!TryTransactionStep([&]()
            {
                auto stmt = SetupSqlStatement(R"sql(
                    UPDATE TxOutputs SET
                        SpentHeight = null,
                        SpentTxHash = null
                    WHERE SpentHeight is not null and SpentHeight >= ?
                )sql");

                return TryBindStatementInt(stmt, 1, heightPtr) && TryStepStatement(stmt);
            }))
                return false;

            int64_t nTime3 = GetTimeMicros();
            LogPrint(BCLog::BENCH, "      - RollbackBlock (Outputs): %.2fms\n", 0.001 * (nTime3 - nTime2));

            // Remove ratings
            if (!TryTransactionStep([&]()
            {
                auto stmt = SetupSqlStatement(R"sql(
                    DELETE FROM Ratings
                    WHERE Height >= ?
                )sql");

                return TryBindStatementInt(stmt, 1, heightPtr) && TryStepStatement(stmt);
            }))
                return false;

            int64_t nTime4 = GetTimeMicros();
            LogPrint(BCLog::BENCH, "      - RollbackBlock (Ratings): %.2fms\n", 0.001 * (nTime4 - nTime3));

            return true;
        }

    private:

        bool UpdateTransactionHeight(const string& blockHash, int height, const string& txHash)
        {
            return TryTransactionStep([&]()
            {
                auto stmt = SetupSqlStatement(R"sql(
                    UPDATE Transactions SET
                        BlockHash=?,
                        Height=?
                    WHERE Hash=?
                )sql");

                // TODO (joni): replace with TryBind
                auto blockHashPtr = make_shared<string>(blockHash);
                auto heightPtr = make_shared<int>(height);
                auto txHashPtr = make_shared<string>(txHash);

                auto result = TryBindStatementText(stmt, 1, blockHashPtr);
                result &= TryBindStatementInt(stmt, 2, heightPtr);
                result &= TryBindStatementText(stmt, 3, txHashPtr);
                if (!result)
                    return false;

                return TryStepStatement(stmt);
            });
        }

        bool UpdateTransactionOutputs(const string& txHash, int height, const map<string, int>& outputs)
        {
            return TryTransactionStep([&]()
            {
                for (auto& out : outputs)
                {
                    auto stmt = SetupSqlStatement(R"sql(
                        UPDATE TxOutputs SET
                            SpentHeight=?,
                            SpentTxHash=?
                        WHERE TxHash=? and Number=?
                    )sql");

                    // TODO (joni): replace with TryBind
                    auto spentHeightPtr = make_shared<int>(height);
                    auto spentTxHashPtr = make_shared<string>(txHash);
                    auto txHashPtr = make_shared<string>(out.first);
                    auto numberPtr = make_shared<int>(out.second);

                    auto result = TryBindStatementInt(stmt, 1, spentHeightPtr);
                    result &= TryBindStatementText(stmt, 2, spentTxHashPtr);
                    result &= TryBindStatementText(stmt, 3, txHashPtr);
                    result &= TryBindStatementInt(stmt, 4, numberPtr);
                    if (!result)
                        return false;

                    if (!TryStepStatement(stmt))
                        return false;
                }

                return true;
            });
        }

        bool SetAccountId(const string& txHash)
        {
            return TryTransactionStep([&]()
            {
                auto stmt = SetupSqlStatement(R"sql(
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
                        )
                    WHERE Hash = ?
                )sql");

                // TODO (joni): replace with TryBind
                auto hashPtr = make_shared<string>(txHash);
                auto result = TryBindStatementText(stmt, 1, hashPtr);
                if (!result)
                    return false;

                return TryStepStatement(stmt);
            });
        }

        bool SetContentId(const string& txHash)
        {
            return TryTransactionStep([&]()
            {
                auto stmt = SetupSqlStatement(R"sql(
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
                )sql");

                // TODO (joni): replace with TryBind
                auto hashPtr = make_shared<string>(txHash);
                auto result = TryBindStatementText(stmt, 1, hashPtr);
                if (!result)
                    return false;

                return TryStepStatement(stmt);
            });
        }

    };

} // namespace PocketDb

#endif // POCKETDB_CHAINREPOSITORY_HPP

