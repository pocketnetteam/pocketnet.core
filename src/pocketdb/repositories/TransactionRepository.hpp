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
        explicit TransactionRepository(SQLiteDatabase &db) : BaseRepository(db) {}

        void Init() override {}

        void Destroy() override {}

        bool Insert(const shared_ptr<Transaction> &transaction)
        {
            if (ShutdownRequested())
                return false;

            auto stmt = SetupSqlStatement(
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
                " WHERE NOT EXISTS (select 1 from Transactions t where t.TxId = ?)"
                " ;"
            );

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
                return false;

            if (!TryStepStatement(stmt))
                return false;

            // Also need insert payload of transaction
            if (transaction->HasPayload())
            {
                auto stmtPayload = SetupSqlStatement(
                    " INSERT INTO Payload ("
                    " TxID,"
                    " Data)"
                    " SELECT ?,?"
                    " WHERE not exists (select 1 from Payload p where p.TxId = ?)"
                    " ;"
                );

                auto resultPayload = TryBindStatementText(stmtPayload, 1, transaction->GetTxId());
                resultPayload &= TryBindStatementText(stmtPayload, 2, transaction->GetPayloadStr());
                resultPayload &= TryBindStatementText(stmtPayload, 3, transaction->GetTxId());
                if (!resultPayload)
                    return false;

                if (!TryStepStatement(stmtPayload))
                    return false;
            }

            return result;
        }

        bool BulkInsert(const std::vector<shared_ptr<Transaction>> &transactions)
        {
            assert(m_database.m_db);
            if (ShutdownRequested())
                return false;

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

    };

} // namespace PocketDb

#endif // POCKETDB_TRANSACTIONREPOSITORY_HPP

