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
        explicit BlockRepository(SQLiteDatabase &db) : BaseRepository(db) {}

        void Init() override {}

        void Destroy() override {}

        bool RollbackTransactions(int height)
        {
            if (ShutdownRequested())
                return false;

            auto stmt = SetupSqlStatement(
                " update Transactions set"
                "   Block = null,"
                "   TxOut = null"
                " where Block >= ?"
                " ;"
            );

            if (!TryBindStatementInt(stmt, 1, height))
                return false;

            if (!TryStepStatement(stmt))
                return false;

            return true;
        }

        bool RollbackUtxo(int height)
        {
            if (ShutdownRequested())
                return false;

            auto stmt = SetupSqlStatement(
                " delete from Utxo"
                " where Block >= ?"
                " ;"
            );

            if (!TryBindStatementInt(stmt, 1, height))
                return false;

            if (!TryStepStatement(stmt))
                return false;

            return true;
        }

        bool RollbackRatings(int height)
        {
            if (ShutdownRequested())
                return false;

            auto stmt = SetupSqlStatement(
                " delete from Ratings"
                " where Block >= ?"
                " ;"
            );

            if (!TryBindStatementInt(stmt, 1, height))
                return false;

            if (!TryStepStatement(stmt))
                return false;

            return true;
        }

        bool BulkRollback(int height)
        {
            assert(m_database.m_db);
            if (ShutdownRequested())
                return false;

            if (!m_database.BeginTransaction())
                return false;

            try
            {
                if (!RollbackTransactions(height))
                    throw std::runtime_error(strprintf("%s: can't step rollback transactions %d\n", __func__, height));

                if (!RollbackUtxo(height))
                    throw std::runtime_error(strprintf("%s: can't step rollback utxos %d\n", __func__, height));

                if (!RollbackRatings(height))
                    throw std::runtime_error(strprintf("%s: can't step rollback ratings %d\n", __func__, height));

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

    };

} // namespace PocketDb

#endif // POCKETDB_BLOCKREPOSITORY_HPP

