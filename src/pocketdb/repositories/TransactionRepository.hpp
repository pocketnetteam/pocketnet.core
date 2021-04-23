#ifndef POCKETDB_TRANSACTIONREPOSITORY_HPP
#define POCKETDB_TRANSACTIONREPOSITORY_HPP

#include "pocketdb/pocketnet.h"
#include "pocketdb/repositories/BaseRepository.hpp"
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

        bool Insert(const shared_ptr<Transaction>& transaction)
        {
            assert(m_database.m_db);

            if (transaction)
            {
                // First set transaction
                auto result = TryBindInsertTransactionStatement(m_insert_transaction_stmt, transaction);
                if (result)
                    result = TryStepStatement(m_insert_transaction_stmt);

                sqlite3_clear_bindings(m_insert_transaction_stmt);
                sqlite3_reset(m_insert_transaction_stmt);

                // Second set payload if transaction inserted
                if (result && transaction->HasPayload())
                {
                    result = TryBindInsertPayloadStatement(m_insert_payload_stmt, transaction);
                    if (result)
                        result = TryStepStatement(m_insert_payload_stmt);

                    sqlite3_clear_bindings(m_insert_payload_stmt);
                    sqlite3_reset(m_insert_payload_stmt);
                }

                return result;
            }

            return true;
        }

        bool BulkInsert(const std::vector<shared_ptr<Transaction>> &transactions)
        {
            assert(m_database.m_db);

            if (!m_database.BeginTransaction())
                return false;

            try
            {
                for (const auto &transaction : transactions)
                {
                    if (!Insert(transaction))
                        throw std::runtime_error(strprintf("SQLiteDatabase: can't insert in transaction\n"));
                }

                if (!m_database.CommitTransaction())
                    throw std::runtime_error(strprintf("SQLiteDatabase: can't commit transaction\n"));

            } catch (std::exception &ex)
            {
                m_database.AbortTransaction();
                return false;
            }

            return true;
        }

        void Delete(const shared_ptr<std::string> &id)
        {
            if (!m_database.m_db)
            {
                throw std::runtime_error(strprintf("SQLiteDatabase: database didn't opened\n"));
            }

            if (!TryBindStatementText(m_delete_transaction_stmt, 1, id))
            {
                //TODO
                return;
            }

            int res = sqlite3_step(m_delete_transaction_stmt);
            if (res != SQLITE_DONE)
            {
                LogPrintf("%s: Unable to execute statement: %s\n", __func__, sqlite3_errstr(res));
            }

            sqlite3_clear_bindings(m_delete_transaction_stmt);
            sqlite3_reset(m_delete_transaction_stmt);
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
                " SELECT ?,?,?,?,?,?,?,?,?,?,?,?,?,?,?"
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

        static bool TryBindInsertTransactionStatement(sqlite3_stmt *stmt, const shared_ptr<Transaction>& transaction)
        {
            if (TryBindStatementInt(stmt, 1, transaction->GetTxTypeInt()) &&
                TryBindStatementText(stmt, 2, transaction->GetTxId()) &&
                TryBindStatementInt64(stmt, 3, transaction->GetBlock()) &&
                TryBindStatementInt64(stmt, 4, transaction->GetTxTime()) &&
                TryBindStatementText(stmt, 5, transaction->GetAddress()) &&
                TryBindStatementInt64(stmt, 6, transaction->GetInt1()) &&
                TryBindStatementInt64(stmt, 7, transaction->GetInt2()) &&
                TryBindStatementInt64(stmt, 8, transaction->GetInt3()) &&
                TryBindStatementInt64(stmt, 9, transaction->GetInt4()) &&
                TryBindStatementInt64(stmt, 10, transaction->GetInt5()) &&
                TryBindStatementText(stmt, 11, transaction->GetString1()) &&
                TryBindStatementText(stmt, 12, transaction->GetString2()) &&
                TryBindStatementText(stmt, 13, transaction->GetString3()) &&
                TryBindStatementText(stmt, 14, transaction->GetString4()) &&
                TryBindStatementText(stmt, 15, transaction->GetString5()) &&
                TryBindStatementText(stmt, 16, transaction->GetTxId()))
            {
                return true;
            }

            //TODO reset bindings
            return false;
        }

        static bool TryBindInsertPayloadStatement(sqlite3_stmt *stmt, const shared_ptr<Transaction>& transaction)
        {
            if (TryBindStatementText(stmt, 1, transaction->GetTxId()) &&
                TryBindStatementText(stmt, 2, transaction->GetPayload()) &&
                TryBindStatementText(stmt, 3, transaction->GetTxId()))
            {
                return true;
            }

            return false;
        }

        static bool TryStepStatement(sqlite3_stmt *stmt)
        {
            int res = sqlite3_step(stmt);
            if (res != SQLITE_ROW && res != SQLITE_DONE)
                LogPrintf("%s: Unable to execute statement: %s: %s\n", __func__, sqlite3_sql(stmt),
                    sqlite3_errstr(res));

            return !(res != SQLITE_ROW && res != SQLITE_DONE);
        }
    };

} // namespace PocketDb

#endif // POCKETDB_TRANSACTIONREPOSITORY_HPP

