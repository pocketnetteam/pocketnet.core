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

namespace PocketDb
{
    using namespace PocketTx;

    class TransactionRepository : public BaseRepository
    {
    public:
        explicit TransactionRepository(SQLiteDatabase &db) : BaseRepository(db) {}

        void Init() override {}
        void Destroy() override {}

        // ================================================================================================================
        //  Base transaction operations
        // ================================================================================================================
        bool InsertTransactions(const vector<shared_ptr<Transaction>> &transactions)
        {
            sqlite3* db = m_database.m_db;
            auto loop = [&, transactions, db] () {
                for (const auto &transaction : transactions)
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
                    sqlite3_set_last_insert_rowid(db, 0);
                    if (!TryStepStatement(stmt))
                        throw std::runtime_error(strprintf("%s: can't insert in transaction (step tx)\n", __func__));

                    // Also need insert payload of transaction
                    // But need get new rowId
                    // If last id equal 0 - insert ignored - or already exists or error -> paylod not inserted
                    auto newId = sqlite3_last_insert_rowid(db);
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
                            throw std::runtime_error(strprintf("%s: can't insert in transaction (bind payload)\n", __func__));

                        if (!TryStepStatement(stmtPayload))
                            throw std::runtime_error(strprintf("%s: can't insert in transaction (step payload)\n", __func__));
                    }
                }
            };

            return TryBulkStep(loop);
        }

        // ================================================================================================================
        //  Transaction outputs operations
        // ================================================================================================================
        bool InsertTransactionOutputs(const vector<TransactionOutput> &outputs)
        {

        }



    private:

    };

} // namespace PocketDb

#endif // POCKETDB_TRANSACTIONREPOSITORY_HPP

