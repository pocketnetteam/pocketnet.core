// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 Bitcoin developers
// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETDB_TRANSACTIONREPOSITORY_HPP
#define POCKETDB_TRANSACTIONREPOSITORY_HPP

#include "pocketdb/pocketnet.h"
#include "pocketdb/repositories/BaseRepository.hpp"
#include "pocketdb/models/base/Block.hpp"
#include "pocketdb/models/base/Transaction.hpp"

namespace PocketDb
{
    using namespace PocketTx;

    class TransactionRepository : public BaseRepository
    {
    public:
        explicit TransactionRepository(SQLiteDatabase &db) : BaseRepository(db)
        {
        }

        void Init() override
        {
            SetupSqlStatements();
        }

        void Destroy() override
        {
            FinalizeSqlStatement(m_insert_transaction_stmt);
            FinalizeSqlStatement(m_delete_transaction_stmt);
            FinalizeSqlStatement(m_insert_payload_stmt);
            FinalizeSqlStatement(m_delete_payload_stmt);
        }

        bool Insert(const shared_ptr<Transaction> &transaction)
        {
            if (ShutdownRequested())
                return false;

            assert(m_database.m_db);
            
            auto result = TryBindInsertTransactionStatement(m_insert_transaction_stmt, transaction) && TryStepStatement(m_insert_transaction_stmt);
            sqlite3_clear_bindings(m_insert_transaction_stmt);
            sqlite3_reset(m_insert_transaction_stmt);

            if (transaction->HasPayload())
            {
                result &= TryBindInsertPayloadStatement(m_insert_payload_stmt, transaction) && TryStepStatement(m_insert_payload_stmt);
                sqlite3_clear_bindings(m_insert_payload_stmt);
                sqlite3_reset(m_insert_payload_stmt);
            }

            return result;
        }

        bool BulkInsert(const std::vector<shared_ptr<Transaction>> &transactions)
        {
            if (ShutdownRequested())
                return false;

            assert(m_database.m_db);

            if (!m_database.BeginTransaction())
                return false;

            try
            {
                for (const auto &transaction : transactions)
                {
                    if (!Insert(transaction))
                        throw std::runtime_error(strprintf("%s: can't insert in transaction\n", __func__));
                }

                if (!m_database.CommitTransaction())
                    throw std::runtime_error(strprintf("%s: can't commit transaction\n", __func__));

            } catch (std::exception &ex)
            {
                m_database.AbortTransaction();
                return false;
            }

            return true;
        }

    private:
        sqlite3_stmt *m_insert_transaction_stmt{nullptr};
        sqlite3_stmt *m_delete_transaction_stmt{nullptr};

        sqlite3_stmt *m_insert_payload_stmt{nullptr};
        sqlite3_stmt *m_delete_payload_stmt{nullptr};

        void SetupSqlStatements()
        {
            m_insert_transaction_stmt = SetupSqlStatement(
                m_insert_transaction_stmt,
                " INSERT INTO Transactions ("
                "   TxType,"
                "   TxId,"
                "   Block,"
                "   TxOut,"
                "   TxTime,"
                "   Address,"
                "   Int1,"
                "   Int2,"
                "   Int3,"
                "   Int4,"
                "   Int5,"
                "   String1,"
                "   String2,"
                "   String3,"
                "   String4,"
                "   String5)"
                " SELECT ?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?"
                " WHERE not exists (select 1 from Transactions t where t.TxId = ?)"
                " ;");

            m_delete_transaction_stmt = SetupSqlStatement(
                m_delete_transaction_stmt,
                " DELETE FROM Transactions"
                " WHERE TxId = ?"
                " ;");

            // TODO (brangr): update stmt for transaction

            m_insert_payload_stmt = SetupSqlStatement(
                m_insert_payload_stmt,
                " INSERT INTO Payload ("
                " TxID,"
                " Data)"
                " SELECT ?,?"
                " WHERE not exists (select 1 from Payload p where p.TxId = ?)"
                " ;");

            m_delete_payload_stmt = SetupSqlStatement(
                m_delete_payload_stmt,
                " DELETE FROM PAYLOAD"
                " WHERE TxId = ?"
                " ;");
        }

        bool TryBindInsertTransactionStatement(sqlite3_stmt *stmt, const shared_ptr<Transaction> &transaction)
        {
            auto result = TryBindStatementInt(stmt, 1, transaction->GetTxTypeInt());
            result &= TryBindStatementText(stmt, 2, transaction->GetTxId());
            result &= TryBindStatementInt64(stmt, 3, transaction->GetBlock());
            result &= TryBindStatementInt64(stmt, 4, transaction->GetTxOut());
            result &= TryBindStatementInt64(stmt, 5, transaction->GetTxTime());
            result &= TryBindStatementText(stmt, 6, transaction->GetAddress());
            result &= TryBindStatementInt64(stmt, 7, transaction->GetInt1());
            result &= TryBindStatementInt64(stmt, 8, transaction->GetInt2());
            result &= TryBindStatementInt64(stmt, 9, transaction->GetInt3());
            result &= TryBindStatementInt64(stmt, 10, transaction->GetInt4());
            result &= TryBindStatementInt64(stmt, 11, transaction->GetInt5());
            result &= TryBindStatementText(stmt, 12, transaction->GetString1());
            result &= TryBindStatementText(stmt, 13, transaction->GetString2());
            result &= TryBindStatementText(stmt, 14, transaction->GetString3());
            result &= TryBindStatementText(stmt, 15, transaction->GetString4());
            result &= TryBindStatementText(stmt, 16, transaction->GetString5());
            result &= TryBindStatementText(stmt, 17, transaction->GetTxId());

            if (!result)
                sqlite3_clear_bindings(stmt);

            return result;
        }

        bool TryBindInsertPayloadStatement(sqlite3_stmt *stmt, const shared_ptr<Transaction> &transaction)
        {
            auto result = TryBindStatementText(stmt, 1, transaction->GetTxId());
            result &= TryBindStatementText(stmt, 2, transaction->GetPayloadStr());
            result &= TryBindStatementText(stmt, 3, transaction->GetTxId());

            if (!result)
                sqlite3_clear_bindings(stmt);

            return result;
        }

        bool TryStepStatement(sqlite3_stmt *stmt)
        {
            int res = sqlite3_step(stmt);
            if (res != SQLITE_ROW && res != SQLITE_DONE)
                LogPrintf("%s: Unable to execute statement: %s: %s\n",
                    __func__, sqlite3_sql(stmt), sqlite3_errstr(res));

            return !(res != SQLITE_ROW && res != SQLITE_DONE);
        }
    };

} // namespace PocketDb

#endif // POCKETDB_TRANSACTIONREPOSITORY_HPP

