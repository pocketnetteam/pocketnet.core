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
#include "pocketdb/models/base/SelectModels.hpp"

namespace PocketDb
{
    using std::runtime_error;

    using namespace PocketTx;

    class TransactionRepository : public BaseRepository
    {
    public:
        explicit TransactionRepository(SQLiteDatabase& db) : BaseRepository(db) {}

        void Init() override {}
        void Destroy() override {}

        // ============================================================================================================
        //  Base transaction operations
        bool InsertTransactions(PocketBlock& pocketBlock)
        {
            return TryTransactionStep([&]()
            {
                for (const auto& transaction : pocketBlock)
                {
                    auto stmt = SetupSqlStatement(R"sql(
                        INSERT OR FAIL INTO Transactions (
                            Type,
                            Hash,
                            Time,
                            AddressId,
                            Int1,
                            Int2,
                            Int3,
                            Int4
                        ) SELECT ?,?,?,?,?,?,?,?;
                    )sql");

                    auto result = TryBindStatementInt(stmt, 1, transaction->GetTypeInt());
                    result &= TryBindStatementText(stmt, 2, transaction->GetHash());
                    result &= TryBindStatementInt64(stmt, 3, transaction->GetTime());
                    result &= TryBindStatementInt64(stmt, 4, transaction->GetAccountAddressId());
                    result &= TryBindStatementInt64(stmt, 5, transaction->GetInt1());
                    result &= TryBindStatementInt64(stmt, 6, transaction->GetInt2());
                    result &= TryBindStatementInt64(stmt, 7, transaction->GetInt3());
                    result &= TryBindStatementInt64(stmt, 8, transaction->GetInt4());
                    if (!result)
                        throw runtime_error(strprintf("%s: can't insert in transaction (bind tx)\n", __func__));

                    // Try execute with clear last rowId
                    sqlite3_set_last_insert_rowid(m_database.m_db, 0);
                    if (!TryStepStatement(stmt))
                        throw runtime_error(strprintf("%s: can't insert in transaction (step tx)\n", __func__));

                    LogPrintf("  - Insert Tx %s\n", *transaction->GetHash());

                    // Also need insert payload of transaction
                    // But need get new rowId
                    // If last id equal 0 - insert ignored - or already exists or error -> paylod not inserted
                    auto newId = sqlite3_last_insert_rowid(m_database.m_db);
                    if (newId > 0 && transaction->HasPayload())
                    {
                        auto newIdPtr = make_shared<int64_t>(newId);
                        auto stmtPayload = SetupSqlStatement(R"sql(
                                INSERT OR FAIL INTO Payload (
                                    TxId,
                                    String1,
                                    String2,
                                    String3,
                                    String4,
                                    String5,
                                    String6,
                                    String7
                                ) SELECT
                                    ?,?,?,?,?,?,?,?
                                WHERE not exists (select 1 from Payload p where p.TxId = ?)
                            )sql"
                        );

                        // TODO (brangr): UNIQUE constraint failed: Payload.TxId
                        auto resultPayload = true;
                        resultPayload &= TryBindStatementInt64(stmtPayload, 1, newIdPtr);
                        resultPayload &= TryBindStatementText(stmtPayload, 2, transaction->GetPayload()->GetString1());
                        resultPayload &= TryBindStatementText(stmtPayload, 3, transaction->GetPayload()->GetString2());
                        resultPayload &= TryBindStatementText(stmtPayload, 4, transaction->GetPayload()->GetString3());
                        resultPayload &= TryBindStatementText(stmtPayload, 5, transaction->GetPayload()->GetString4());
                        resultPayload &= TryBindStatementText(stmtPayload, 6, transaction->GetPayload()->GetString5());
                        resultPayload &= TryBindStatementText(stmtPayload, 7, transaction->GetPayload()->GetString6());
                        resultPayload &= TryBindStatementText(stmtPayload, 8, transaction->GetPayload()->GetString7());
                        resultPayload &= TryBindStatementInt64(stmtPayload, 9, newIdPtr);
                        if (!resultPayload)
                            throw runtime_error(
                                strprintf("%s: can't insert in transaction (bind payload)\n", __func__));

                        if (!TryStepStatement(stmtPayload))
                            throw runtime_error(
                                strprintf("%s: can't insert in transaction (step payload): %s %d\n",
                                    __func__, *transaction->GetPayload()->GetTxHash(), *transaction->GetType()));
                    }
                }
            });
        }

        // ============================================================================================================
        //  Transaction output addresses
        bool InsertTransactionOutputAddresses(const vector<string>& addresses)
        {
            return TryTransactionStep([&]()
            {
                for (const auto& address : addresses)
                {
                    auto addressPtr = make_shared<string>(address);

                    // Save transaction output address
                    auto stmt = SetupSqlStatement(R"sql(
                        INSERT OR FAIL INTO Addresses (Address)
                        SELECT ?
                        WHERE NOT EXISTS (select 1 from Addresses a where a.Address = ?);
                    )sql");

                    auto result = true;
                    result &= TryBindStatementText(stmt, 1, addressPtr);
                    result &= TryBindStatementText(stmt, 2, addressPtr);
                    if (!result)
                        throw runtime_error(strprintf("%s: can't insert in transaction (bind addr)\n", __func__));

                    if (!TryStepStatement(stmt))
                        throw runtime_error(strprintf("%s: can't insert in transaction (step addr)\n", __func__));
                }
            });
        }

        // ============================================================================================================
        //  Transaction outputs operations
        bool InsertTransactionOutputs(const vector<TransactionOutput>& outputs)
        {
            return TryTransactionStep([&]()
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
                                ?,
                                ?
                        )sql"
                    );

                    auto result = TryBindStatementText(stmt, 1, output.GetTxHash());
                    result &= TryBindStatementInt64(stmt, 2, output.GetNumber());
                    result &= TryBindStatementInt64(stmt, 3, output.GetValue());
                    if (!result)
                        throw runtime_error(strprintf("%s: can't insert in transaction (bind out)\n", __func__));

                    // Try execute with clear last rowId
                    if (!TryStepStatement(stmt))
                        throw runtime_error(strprintf("%s: can't insert in transaction (step out): %s\n",
                            __func__, *output.GetTxHash()));

                    // Also need save destination addresses
                    for (const auto& dest : *(output.GetDestinations()))
                    {
                        auto destPtr = make_shared<string>(dest);
                        auto stmtDest = SetupSqlStatement(R"sql(
                                INSERT OR FAIL INTO TxOutputsDestinations (
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
                        resultDest &= TryBindStatementText(stmtDest, 3, destPtr);
                        if (!resultDest)
                            throw runtime_error(
                                strprintf("%s: can't insert in transaction (bind outdest)\n", __func__));

                        // Try execute with clear last rowId
                        if (!TryStepStatement(stmtDest))
                            throw runtime_error(strprintf("%s: can't insert in transaction (step outdest): %s\n",
                                __func__, dest));
                    }
                }
            });
        }

        // ============================================================================================================
        //  Transaction inputs operations - spent outputs
        bool InsertTransactionInputs(const vector<TransactionInput>& inputs)
        {
            return TryTransactionStep([&]()
            {
                for (const auto& input : inputs)
                {
                    // Build sql statement with auto select IDs
                    auto stmt = SetupSqlStatement(R"sql(
                            INSERT OR FAIL INTO TxInputs (
                                TxId,
                                InputTxId,
                                InputTxNumber
                            ) SELECT
                                (select t.Id from Transactions t where t.Hash=? limit 1),
                                (select t.Id from Transactions t where t.Hash=? limit 1),
                                ?
                        )sql"
                    );

                    // Bind arguments
                    auto result = TryBindStatementText(stmt, 1, input.GetTxHash());
                    result &= TryBindStatementText(stmt, 2, input.GetInputTxHash());
                    result &= TryBindStatementInt64(stmt, 3, input.GetInputTxNumber());
                    if (!result)
                        throw runtime_error(strprintf("%s: can't insert in transaction (bind inp)\n", __func__));

                    // Try execute
                    if (!TryStepStatement(stmt))
                        throw runtime_error(strprintf("%s: can't insert in transaction (step inp)\n", __func__));
                }
            });
        }

        // ============================================================================================================
        //  SELECTs

        // Mapping TxHash (string) -> TxId (int) and Address (string) -> AddressId (int)
        // type = 0 for transactions
        // type = 1 for addresses
        bool MapHashToId(int type, map<string, int>& lst)
        {
            if (ShutdownRequested())
                return false;

            // Build bind string for select - ?,?,?
            string values;
            for (int i = 0; i < lst.size(); i++)
                values += "?,";

            // Trim last comma
            values.erase(values.find_last_not_of(",") + 1);

            // Build sql statement
            string sql;
            switch (type)
            {
                case 0:
                    sql = "SELECT t.Hash, t.Id FROM Transactions t WHERE t.Hash in (" + values + ");";
                    break;
                case 1:
                    sql = "SELECT a.Address, a.Id FROM Addresses a WHERE a.Address in (" + values + ");";
                    break;
                default:
                    return false;
            }
            auto stmt = SetupSqlStatement(sql);

            // Bind "where" arguments
            int i = 1;
            for (const auto& m : lst)
            {
                auto mFirstPtr = make_shared<string>(m.first);
                if (!TryBindStatementText(stmt, i, mFirstPtr))
                    return false;
                i += 1;
            }

            // Execute
            return TryTransactionStep([&]()
            {
                while (sqlite3_step(*stmt) == SQLITE_ROW)
                {
                    lst[GetColumnString(*stmt, 0)] = GetColumnInt(*stmt, 1);
                }
            });
        }

        // Top addresses info
        tuple<bool, SelectList<SelectAddressInfo>> SelectTopAddresses(int count)
        {
            SelectList<SelectAddressInfo> result;

            if (ShutdownRequested())
                return make_tuple(false, result);

            bool tryResult = TryTransactionStep([&]()
            {
                auto countPtr = make_shared<int>(count);
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

                if (!TryBindStatementInt(stmt, 1, countPtr))
                    return make_tuple(false, result);

                while (sqlite3_step(*stmt) == SQLITE_ROW)
                {
                    SelectAddressInfo inf;
                    inf.Address = GetColumnString(*stmt, 0);
                    inf.Balance = GetColumnInt64(*stmt, 1);
                    result.Add(inf);
                }
            });

            return make_tuple(tryResult, result);
        }

    private:

    };

} // namespace PocketDb

#endif // POCKETDB_TRANSACTIONREPOSITORY_HPP

