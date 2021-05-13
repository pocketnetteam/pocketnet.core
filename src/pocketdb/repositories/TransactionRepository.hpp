// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 Bitcoin developers
// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETDB_TRANSACTIONREPOSITORY_HPP
#define POCKETDB_TRANSACTIONREPOSITORY_HPP

#include "pocketdb/pocketnet.h"
#include "pocketdb/repositories/BaseRepository.hpp"
#include "pocketdb/models/base/Transaction.hpp"
#include "pocketdb/models/base/TransactionOutput.hpp"
#include "pocketdb/models/base/TransactionInput.hpp"
#include "pocketdb/models/base/Return.hpp"

namespace PocketDb
{
    using namespace PocketTx;

    class TransactionRepository : public BaseRepository
    {
    public:
        explicit TransactionRepository(SQLiteDatabase& db) : BaseRepository(db) {}

        void Init() override {}
        void Destroy() override {}

        // ================================================================================================================
        //  Base transaction operations
        // ================================================================================================================
        bool InsertTransactions(PocketBlock& pocketBlock)
        {
            return TryBulkStep([&]()
            {
                for (const auto& transaction : pocketBlock)
                {
                    auto stmt = SetupSqlStatement(R"sql(
                            INSERT OR IGNORE INTO Transactions (
                                Type,
                                Hash,
                                Time,
                                AddressId,
                                Int1,
                                Int2,
                                Int3,
                                Int4
                            ) SELECT ?,?,?,?,?,?,?,?;
                        )sql"
                    );

                    auto result = TryBindStatementInt(stmt, 1, transaction->GetTypeInt());
                    result &= TryBindStatementText(stmt, 2, transaction->GetHash());
                    result &= TryBindStatementInt64(stmt, 3, transaction->GetTime());
                    result &= TryBindStatementInt64(stmt, 4, transaction->GetAccountAddressId());
                    result &= TryBindStatementInt64(stmt, 5, transaction->GetInt1());
                    result &= TryBindStatementInt64(stmt, 6, transaction->GetInt2());
                    result &= TryBindStatementInt64(stmt, 7, transaction->GetInt3());
                    result &= TryBindStatementInt64(stmt, 8, transaction->GetInt4());
                    if (!result)
                        throw std::runtime_error(strprintf("%s: can't insert in transaction (bind tx)\n", __func__));

                    // Try execute with clear last rowId
                    sqlite3_set_last_insert_rowid(m_database.m_db, 0);
                    if (!TryStepStatement(stmt))
                        throw std::runtime_error(strprintf("%s: can't insert in transaction (step tx)\n", __func__));

                    // Also need insert payload of transaction
                    // But need get new rowId
                    // If last id equal 0 - insert ignored - or already exists or error -> paylod not inserted
                    auto newId = sqlite3_last_insert_rowid(m_database.m_db);
                    if (newId > 0 && transaction->HasPayload())
                    {
                        auto stmtPayload = SetupSqlStatement(R"sql(
                                INSERT OR IGNORE INTO Payload (
                                    TxId,
                                    String1,
                                    String2,
                                    String3,
                                    String4,
                                    String5,
                                    String6,
                                    String7
                                ) SELECT ?,?,?,?,?,?,?,?;
                            )sql"
                        );

                        auto resultPayload = TryBindStatementInt64(stmtPayload, 1, make_shared<int64_t>(newId));
                        resultPayload &= TryBindStatementText(stmtPayload, 2, transaction->GetPayload()->GetString1());
                        resultPayload &= TryBindStatementText(stmtPayload, 3, transaction->GetPayload()->GetString2());
                        resultPayload &= TryBindStatementText(stmtPayload, 4, transaction->GetPayload()->GetString3());
                        resultPayload &= TryBindStatementText(stmtPayload, 5, transaction->GetPayload()->GetString4());
                        resultPayload &= TryBindStatementText(stmtPayload, 6, transaction->GetPayload()->GetString5());
                        resultPayload &= TryBindStatementText(stmtPayload, 7, transaction->GetPayload()->GetString6());
                        resultPayload &= TryBindStatementText(stmtPayload, 8, transaction->GetPayload()->GetString7());
                        if (!resultPayload)
                            throw std::runtime_error(
                                strprintf("%s: can't insert in transaction (bind payload)\n", __func__));

                        if (!TryStepStatement(stmtPayload))
                            throw std::runtime_error(
                                strprintf("%s: can't insert in transaction (step payload)\n", __func__));
                    }
                }
            });
        }

        // ================================================================================================================
        //  Transaction outputs operations
        // ================================================================================================================
        bool InsertTransactionsOutputs(const vector<TransactionOutput>& outputs)
        {
            return TryBulkStep([&]()
            {
                for (const auto& output : outputs)
                {
                    // Save transaction output
                    auto stmt = SetupSqlStatement(R"sql(
                            INSERT OR FAIL INTO TxOutputs (
                                TxId,
                                Number,
                                Value
                            ) SELECT
                                (select t.Id from Transactions t where t.Hash=? limit 1),
                                (select a.Id from Addresses a where a.Address=? limit 1),
                                ?;
                        )sql"
                    );

                    auto result = TryBindStatementText(stmt, 1, output.GetTxHash());
                    result &= TryBindStatementInt64(stmt, 2, output.GetNumber());
                    result &= TryBindStatementInt64(stmt, 3, output.GetValue());
                    if (!result)
                        throw std::runtime_error(strprintf("%s: can't insert in transaction (bind out)\n", __func__));

                    // Try execute with clear last rowId
                    if (!TryStepStatement(stmt))
                        throw std::runtime_error(strprintf("%s: can't insert in transaction (step out)\n", __func__));

                    // Also need save destination addresses
                    // TODO (brangr): может быть вектор нужно сначала получить, а потом в цикл пихать?
                    for (const auto& dest : *(output.GetDestinations()))
                    {
                        auto stmtDest = SetupSqlStatement(R"sql(
                                INSERT OR IGNORE INTO TxOutputsDestinations (
                                    TxId,
                                    Number,
                                    AddressId
                                ) SELECT
                                    (select t.Id from Transactions t where t.Hash=? limit 1),
                                    ?,
                                    (select a.Id from Addresses a where a.Address=? limit 1);
                            )sql"
                        );

                        auto resultDest = TryBindStatementText(stmtDest, 1, output.GetTxHash());
                        resultDest &= TryBindStatementInt64(stmtDest, 2, output.GetNumber());
                        resultDest &= TryBindStatementText(stmtDest, 3, make_shared<string>(dest));
                        if (!resultDest)
                            throw std::runtime_error(
                                strprintf("%s: can't insert in transaction (bind outdest)\n", __func__));

                        // Try execute with clear last rowId
                        if (!TryStepStatement(stmtDest))
                            throw std::runtime_error(
                                strprintf("%s: can't insert in transaction (step outdest)\n", __func__));
                    }
                }
            });
        }

        // ================================================================================================================
        //  Transaction inputs operations - spent outputs
        // ================================================================================================================
        bool InsertTransactionsInputs(const vector<TransactionInput>& inputs)
        {
            return TryBulkStep([&]()
            {
                for (const auto& input : inputs)
                {
                    // Save transaction input
                    auto stmt = SetupSqlStatement(R"sql(
                            INSERT OR IGNORE INTO TxInputs (
                                TxId,
                                InputTxId,
                                InputTxNumber
                            ) SELECT
                                (select t.Id from Transactions t where t.Hash=? limit 1),
                                (select t.Id from Transactions t where t.Hash=? limit 1),
                                ?
                        )sql"
                    );

                    auto result = TryBindStatementText(stmt, 1, input.GetTxHash());
                    result &= TryBindStatementText(stmt, 2, input.GetInputTxHash());
                    result &= TryBindStatementInt64(stmt, 3, input.GetInputTxNumber());
                    if (!result)
                        throw std::runtime_error(strprintf("%s: can't insert in transaction (bind inp)\n", __func__));

                    // Try execute with clear last rowId
                    if (!TryStepStatement(stmt))
                        throw std::runtime_error(strprintf("%s: can't insert in transaction (step inp)\n", __func__));
                }
            });
        }


        // ================================================================================================================
        // 
        //  SELECTs
        // 
        // ================================================================================================================

        // Top addresses info
        tuple<bool, RetList<RetAddressInfo>> SelectTopAddresses(int count)
        {
            RetList<RetAddressInfo> result;

            assert(m_database.m_db);
            if (ShutdownRequested())
                return make_tuple(false, result);

            try
            {
                auto stmt = SetupSqlStatement(
                    " SELECT"
                    "   u.Address,"
                    "   SUM(u.Amount)Balance"
                    " FROM Utxo u"
                    " WHERE u.BlockSpent is null"
                    " GROUP BY u.Address"
                    " ORDER BY sum(u.Amount) desc"
                    " LIMIT ?"
                    " ;"
                );

                if (!TryBindStatementInt(stmt, 1, count))
                    return make_tuple(false, result);

                while (sqlite3_step(*stmt) == SQLITE_ROW)
                {
                    RetAddressInfo inf;
                    inf.Address = GetColumnString(*stmt, 0);
                    inf.Balance = GetColumnInt64(*stmt, 1);
                    result.Add(inf);
                }
            }
            catch (std::exception& ex)
            {
                return make_tuple(false, result);
            }

            return make_tuple(true, result);
        }

    private:

    };

} // namespace PocketDb

#endif // POCKETDB_TRANSACTIONREPOSITORY_HPP

