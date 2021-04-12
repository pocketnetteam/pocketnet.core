#ifndef TESTDEMO_TRANSACTIONREPOSITORY_H
#define TESTDEMO_TRANSACTIONREPOSITORY_H

#include "BaseRepository.h"
#include "pocketdb/models/Transaction.h"

class TransactionRepository : public BaseRepository {
public:
    explicit TransactionRepository(SQLiteDatabase &database);

    void Insert(const Transaction& transaction);

    void BulkInsert(const std::vector<Transaction>& transactions);

    void Delete(string id);

private:
    sqlite3_stmt* m_insert_stmt{nullptr};
    sqlite3_stmt* m_delete_stmt{nullptr};

    void SetupSqlStatements();

    bool TryBindInsertStatement(sqlite3_stmt* stmt, const Transaction& transaction);
};


#endif //TESTDEMO_TRANSACTIONREPOSITORY_H
