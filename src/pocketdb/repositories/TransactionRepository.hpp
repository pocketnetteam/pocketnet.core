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

        void Insert(const Transaction *transaction)
        {
            if (!m_database.m_db)
            {
                throw std::runtime_error(strprintf("SQLiteDatabase: database didn't opened: %s\n"));
            }

            if (!TryBindInsertStatement(m_insert_stmt, transaction))
            {
                //TODO
                return;
            }

            int res = sqlite3_step(m_insert_stmt);
            if (res != SQLITE_ROW)
            {
                if (res != SQLITE_DONE)
                {
                    LogPrintf("%s: Unable to execute statement: %s\n", __func__, sqlite3_errstr(res));
                }
            }

            sqlite3_clear_bindings(m_insert_stmt);
            sqlite3_reset(m_insert_stmt);
        }

        void BulkInsert(const std::vector<Transaction *> &transactions)
        {
            if (!m_database.m_db)
            {
                throw std::runtime_error(strprintf("SQLiteDatabase: database didn't opened: %s\n"));
            }

            if (!m_database.BeginTransaction())
            {
                throw std::runtime_error(strprintf("SQLiteDatabase: can't begin transaction: %s\n"));
            }

            try
            {
                for (const auto &transaction : transactions)
                {
                    if (!TryBindInsertStatement(m_insert_stmt, transaction))
                    {
                        //TODO
                        return;
                    }

                    int res = sqlite3_step(m_insert_stmt);
                    if (res != SQLITE_ROW)
                    {
                        if (res != SQLITE_DONE)
                        {
                            LogPrintf("%s: Unable to execute statement: %s\n", __func__, sqlite3_errstr(res));
                        }
                    }

                    sqlite3_reset(m_insert_stmt);
                }

                if (!m_database.CommitTransaction())
                {
                    throw std::runtime_error(strprintf("SQLiteDatabase: can't commit transaction\n"));
                }
            } catch (std::exception &ex)
            {
                m_database.AbortTransaction();
            }

            sqlite3_clear_bindings(m_insert_stmt);
            sqlite3_reset(m_insert_stmt);
        }

        void Delete(const std::string& id)
        {
            if (!m_database.m_db)
            {
                throw std::runtime_error(strprintf("SQLiteDatabase: database didn't opened: %s\n"));
            }

            if (!TryBindStatementText(m_delete_stmt, 1, &id))
            {
                //TODO
                return;
            }

            int res = sqlite3_step(m_delete_stmt);
            if (res != SQLITE_DONE)
            {
                LogPrintf("%s: Unable to execute statement: %s\n", __func__, sqlite3_errstr(res));
            }

            sqlite3_clear_bindings(m_delete_stmt);
            sqlite3_reset(m_delete_stmt);
        }

    private:
        sqlite3_stmt *m_insert_stmt{nullptr};
        sqlite3_stmt *m_delete_stmt{nullptr};

        void SetupSqlStatements()
        {
            m_insert_stmt = SetupSqlStatement(
                m_insert_stmt,
                " INSERT INTO Transactions (TxType, TxId, TxTime, Address, Int1, Int2, Int3, Int4, Int5, String1, String2, String3, String4, String5)"
                " SELECT ?,?,?,?,?,?,?,?,?,?,?,?,?,?"
                " WHERE not exists (select 1 from Transactions t where t.TxId=?);");

            m_delete_stmt = SetupSqlStatement(m_delete_stmt, "DELETE FROM Transactions WHERE TxId = ?");
        }

        static bool TryBindInsertStatement(sqlite3_stmt *stmt, const Transaction *transaction)
        {
            if (TryBindStatementInt(stmt, 1, (int *) (transaction->GetTxType())) &&
                TryBindStatementText(stmt, 2, transaction->GetTxId()) &&
                TryBindStatementInt64(stmt, 3, transaction->GetTxTime()) &&
                TryBindStatementText(stmt, 4, transaction->GetAddress()) &&
                TryBindStatementInt64(stmt, 5, transaction->GetInt1()) &&
                TryBindStatementInt64(stmt, 6, transaction->GetInt2()) &&
                TryBindStatementInt64(stmt, 7, transaction->GetInt3()) &&
                TryBindStatementInt64(stmt, 8, transaction->GetInt4()) &&
                TryBindStatementInt64(stmt, 9, transaction->GetInt5()) &&
                TryBindStatementText(stmt, 10, transaction->GetString1()) &&
                TryBindStatementText(stmt, 11, transaction->GetString2()) &&
                TryBindStatementText(stmt, 12, transaction->GetString3()) &&
                TryBindStatementText(stmt, 13, transaction->GetString4()) &&
                TryBindStatementText(stmt, 14, transaction->GetString5()) &&
                TryBindStatementText(stmt, 15, transaction->GetTxId()))
            {
                return true;
            }

            //TODO reset bindings
            return false;
        }
    };

} // namespace PocketDb

#endif // POCKETDB_TRANSACTIONREPOSITORY_HPP

