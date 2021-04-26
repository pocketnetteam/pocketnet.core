#ifndef POCKETDB_BLOCKREPOSITORY_HPP
#define POCKETDB_BLOCKREPOSITORY_HPP

#include "pocketdb/pocketnet.h"
#include "pocketdb/repositories/BaseRepository.hpp"
#include "pocketdb/models/base/Utxo.hpp"

namespace PocketDb
{
    using namespace PocketTx;

    class BlockRepository : public BaseRepository
    {
    public:
        explicit BlockRepository(SQLiteDatabase &db) : BaseRepository(db)
        {
        }

        void Init() override
        {
            SetupSqlStatements();
        }

        bool Insert(const shared_ptr<Utxo> &utxo)
        {
            assert(m_database.m_db);
            auto result = true;

            if (TryBindInsertUtxoStatement(m_insert_utxo_stmt, utxo))
            {
                result &= TryStepStatement(m_insert_utxo_stmt);

                sqlite3_clear_bindings(m_insert_utxo_stmt);
                sqlite3_reset(m_insert_utxo_stmt);
            }

            return result;
        }

        bool BulkInsert(const std::vector<shared_ptr<Utxo>> &utxos)
        {
            assert(m_database.m_db);

            if (!m_database.BeginTransaction())
                return false;

            try
            {
                for (const auto &utxo : utxos)
                {
                    if (!Insert(utxo))
                        throw std::runtime_error(strprintf("%s: can't insert in Utxo\n", __func__));
                }

                if (!m_database.CommitTransaction())
                    throw std::runtime_error(strprintf("%s: can't commit Utxo\n", __func__));

            } catch (std::exception &ex)
            {
                m_database.AbortTransaction();
                return false;
            }

            return true;
        }

        bool BulkSpentUtxo(const std::vector<shared_ptr<Utxo>> &utxos)
        {
            // TODO (joni): update SpentBlock for utxo records
        }

    private:
        sqlite3_stmt *m_insert_utxo_stmt{nullptr};
        sqlite3_stmt *m_spent_utxo_stmt{nullptr};

        void SetupSqlStatements()
        {
            m_insert_utxo_stmt = SetupSqlStatement(
                m_insert_utxo_stmt,
                " INSERT INTO Utxo ("
                "   TxId,"
                "   Block,"
                "   TxOut,"
                "   TxTime,"
                "   Address,"
                "   BlockSpent,"
                "   Amount"
                " )"
                " SELECT ?,?,?,?,?,?,?"
                " WHERE not exists (select 1 from Transactions t where t.TxId = ? and t.TxOut = ?)"
                " ;");

            // TODO (joni): ...
        }

        bool TryBindInsertUtxoStatement(sqlite3_stmt *stmt, const shared_ptr<Utxo> &utxo)
        {
//            auto result = TryBindStatementInt(stmt, 1, transaction->GetTxTypeInt());
//            result &= TryBindStatementText(stmt, 2, transaction->GetTxId());
//            result &= TryBindStatementInt64(stmt, 3, transaction->GetBlock());
//            result &= TryBindStatementInt64(stmt, 4, transaction->GetTxOut());
//            result &= TryBindStatementInt64(stmt, 5, transaction->GetTxTime());
//            result &= TryBindStatementText(stmt, 6, transaction->GetAddress());
//            result &= TryBindStatementInt64(stmt, 7, transaction->GetInt1());
//            result &= TryBindStatementInt64(stmt, 8, transaction->GetInt2());
//            result &= TryBindStatementInt64(stmt, 9, transaction->GetInt3());
//            result &= TryBindStatementInt64(stmt, 10, transaction->GetInt4());
//            result &= TryBindStatementInt64(stmt, 11, transaction->GetInt5());
//            result &= TryBindStatementText(stmt, 12, transaction->GetString1());
//            result &= TryBindStatementText(stmt, 13, transaction->GetString2());
//            result &= TryBindStatementText(stmt, 14, transaction->GetString3());
//            result &= TryBindStatementText(stmt, 15, transaction->GetString4());
//            result &= TryBindStatementText(stmt, 16, transaction->GetString5());
//            result &= TryBindStatementText(stmt, 17, transaction->GetTxId());
//
//            if (!result)
//                sqlite3_clear_bindings(stmt);
//
//            return result;
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

#endif // POCKETDB_BLOCKREPOSITORY_HPP

