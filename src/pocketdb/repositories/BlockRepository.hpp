// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 Bitcoin developers
// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETDB_BLOCKREPOSITORY_HPP
#define POCKETDB_BLOCKREPOSITORY_HPP

#include "pocketdb/pocketnet.h"
#include "pocketdb/repositories/BaseRepository.hpp"

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
            m_rollback_stmt = SetupSqlStatement(
                m_rollback_stmt,
                " update Transactions set"
                "   Block = null,"
                "   TxOut = null"
                " where Block >= ?;"
                " "
                " delete from Utxo where Block >= ?;"
                " delete from Ratings where Block >= ?;"
            );
        }

        void Destroy() override
        {
            FinalizeSqlStatement(m_rollback_stmt);
        }

        bool BulkRollback(int height)
        {
            assert(m_database.m_db);

            if (!m_database.BeginTransaction())
                return false;

            try
            {
                if (!TryBindRollbackStatement(height) || !TryStepStatement(m_rollback_stmt))
                    throw std::runtime_error(strprintf("%s: can't step rollback block %d\n", __func__, height));
                sqlite3_clear_bindings(m_rollback_stmt);
                sqlite3_reset(m_rollback_stmt);

                if (!m_database.CommitTransaction())
                    throw std::runtime_error(strprintf("%s: can't commit rollback block\n", __func__));

            } catch (std::exception &ex)
            {
                m_database.AbortTransaction();
                return false;
            }

            return true;
        }

    private:
        sqlite3_stmt *m_rollback_stmt{nullptr};

        bool TryBindRollbackStatement(int height)
        {
            auto result = TryBindStatementInt(m_rollback_stmt, 1, height);
            result &= TryBindStatementInt(m_rollback_stmt, 2, height);
            result &= TryBindStatementInt(m_rollback_stmt, 3, height);

            if (!result)
                sqlite3_clear_bindings(m_rollback_stmt);

            return result;
        }
    };

} // namespace PocketDb

#endif // POCKETDB_BLOCKREPOSITORY_HPP

