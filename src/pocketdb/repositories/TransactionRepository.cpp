#include "TransactionRepository.h"

TransactionRepository::TransactionRepository(SQLiteDatabase &database)
        : BaseRepository(database) {
    SetupSqlStatements();
}

void TransactionRepository::SetupSqlStatements() {
    m_insert_stmt = SetupSqlStatement(m_insert_stmt, "INSERT INTO Transactions (TxType, TxId, TxTime, Block, Address, Int1, Int2, Int3, Int4, Int5, String1, String2, String3, String4, String5)  VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)");

    m_delete_stmt = SetupSqlStatement(m_delete_stmt, "DELETE FROM Transactions WHERE TxId = ?");
}

void TransactionRepository::Insert(const Transaction &transaction) {
    if (!m_database.m_db) {
        throw std::runtime_error(strprintf("SQLiteDatabase: database didn't opened: %s\n"));
    }

    if (!TryBindInsertStatement(m_insert_stmt, transaction)) {
        //TODO
        return;
    }

    int res = sqlite3_step(m_insert_stmt);
    if (res != SQLITE_ROW) {
        if (res != SQLITE_DONE) {
            LogPrintf("%s: Unable to execute statement: %s\n", __func__, sqlite3_errstr(res));
        }
    }

    sqlite3_clear_bindings(m_insert_stmt);
    sqlite3_reset(m_insert_stmt);
}

void TransactionRepository::Delete(string id) {
    if (!m_database.m_db) {
        throw std::runtime_error(strprintf("SQLiteDatabase: database didn't opened: %s\n"));
    }

    if (!TryBindStatementText(m_delete_stmt, 1, &id)) {
        //TODO
        return;
    }

    int res = sqlite3_step(m_delete_stmt);
    if (res != SQLITE_DONE) {
        LogPrintf("%s: Unable to execute statement: %s\n", __func__, sqlite3_errstr(res));
    }

    sqlite3_clear_bindings(m_delete_stmt);
    sqlite3_reset(m_delete_stmt);
}

void TransactionRepository::BulkInsert(const std::vector<Transaction>& transactions) {
    if (!m_database.m_db) {
        throw std::runtime_error(strprintf("SQLiteDatabase: database didn't opened: %s\n"));
    }

    if (!m_database.BeginTransaction()) {
        throw std::runtime_error(strprintf("SQLiteDatabase: can't begin transaction: %s\n"));
    }

    try {
        for (const auto& transaction : transactions) {
            if (!TryBindInsertStatement(m_insert_stmt, transaction)) {
                //TODO
                return;
            }

            int res = sqlite3_step(m_insert_stmt);
            if (res != SQLITE_ROW) {
                if (res != SQLITE_DONE) {
                    LogPrintf("%s: Unable to execute statement: %s\n", __func__, sqlite3_errstr(res));
                }
            }

            sqlite3_reset(m_insert_stmt);
        }

        if (!m_database.CommitTransaction()) {
            throw std::runtime_error(strprintf("SQLiteDatabase: can't commit transaction: %s\n"));
        }
    }
    catch (std::exception & ex) {
        m_database.AbortTransaction();
    }

    sqlite3_clear_bindings(m_insert_stmt);
    sqlite3_reset(m_insert_stmt);
}

bool TransactionRepository::TryBindInsertStatement(sqlite3_stmt *stmt, const Transaction &transaction) {
    if (TryBindStatementInt(stmt, 1, transaction.GetTxType())
        && TryBindStatementText(stmt, 2, transaction.GetTxId())
        && TryBindStatementInt (stmt, 3, transaction.GetTxTime())
        && TryBindStatementInt (stmt, 4, transaction.GetBlock())
        && TryBindStatementText(stmt, 5, transaction.GetAddress())
        && TryBindStatementInt (stmt, 6, transaction.GetInt1())
        && TryBindStatementInt (stmt, 7, transaction.GetInt2())
        && TryBindStatementInt (stmt, 8, transaction.GetInt3())
        && TryBindStatementInt (stmt, 9, transaction.GetInt4())
        && TryBindStatementInt (stmt, 10, transaction.GetInt5())
        && TryBindStatementText(stmt, 11, transaction.GetString1())
        && TryBindStatementText(stmt, 12, transaction.GetString2())
        && TryBindStatementText(stmt, 13, transaction.GetString3())
        && TryBindStatementText(stmt, 14, transaction.GetString4())
        && TryBindStatementText(stmt, 15, transaction.GetString5())) {
        return true;
    }

    //TODO reset bindings
    return false;
}
