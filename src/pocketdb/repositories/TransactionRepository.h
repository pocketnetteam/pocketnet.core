#ifndef POCKETDB_TRANSACTIONREPOSITORY_H
#define POCKETDB_TRANSACTIONREPOSITORY_H

#include "BaseRepository.h"
#include "pocketdb/models/base/Transaction.hpp"

namespace PocketDb
{
    using namespace PocketTx;

    class TransactionRepository : public BaseRepository
    {
    public:
        TransactionRepository(SQLiteDatabase &db);

        void Init() override;

        void Insert(const Transaction *transaction);

        void BulkInsert(const std::vector<Transaction *> &transactions);

        void Delete(std::string id);

    private:
        sqlite3_stmt *m_insert_stmt{nullptr};
        sqlite3_stmt *m_delete_stmt{nullptr};

        void SetupSqlStatements();

        bool TryBindInsertStatement(sqlite3_stmt *stmt, const Transaction *transaction);
    };

} // namespace PocketDb

#endif // POCKETDB_TRANSACTIONREPOSITORY_H
