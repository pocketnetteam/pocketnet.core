// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 Bitcoin developers
// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETDB_TRANSACTIONREPOSITORY_HPP
#define POCKETDB_TRANSACTIONREPOSITORY_HPP

#include "pocketdb/repositories/BaseRepository.hpp"
#include "pocketdb/models/base/Transaction.hpp"
#include "pocketdb/models/base/TransactionOutput.hpp"
#include "pocketdb/models/base/Rating.hpp"
#include "pocketdb/models/base/SelectModels.hpp"
#include "pocketdb/models/dto/User.hpp"

namespace PocketDb
{
    using std::runtime_error;

    using namespace PocketTx;

    class TransactionRepository : public BaseRepository
    {
    public:
        explicit TransactionRepository(SQLiteDatabase& db) : BaseRepository(db) {}

        void Init() override {}
        void Destroy() override {}

        //  Base transaction operations
        bool InsertTransactions(PocketBlock& pocketBlock)
        {
            return TryTransactionStep([&]()
            {
                for (const auto& ptx : pocketBlock)
                {
                    // Insert general transaction
                    InsertTransactionModel(ptx);

                    // Outputs
                    InsertTransactionOutputs(ptx);

                    // Also need insert payload of transaction
                    // But need get new rowId
                    // If last id equal 0 - insert ignored - or already exists or error -> paylod not inserted
                    if (ptx->HasPayload())
                    {
                        InsertTransactionPayload(ptx);
                    }
                }
            });
        }


        // Top addresses info
        tuple<bool, SelectList<SelectAddressInfo>> SelectTopAddresses(int count)
        {
            SelectList<SelectAddressInfo> result;

            bool tryResult = TryTransactionStep([&]()
            {
                // TODO (brangr): implement select from outputs with join Chain
                auto stmt = SetupSqlStatement(R"sql(
                    SELECT u.Address, SUM(u.Amount)Balance
                    FROM
                )sql");

                auto countPtr = make_shared<int>(count);
                if (!TryBindStatementInt(stmt, 1, countPtr))
                    return false;

                while (sqlite3_step(*stmt) == SQLITE_ROW)
                {
                    SelectAddressInfo inf;
                    inf.Address = GetColumnString(*stmt, 0);
                    inf.Balance = GetColumnInt64(*stmt, 1);
                    result.Add(inf);
                }

                return true;
            });

            return make_tuple(tryResult, result);
        }

    private:

        void InsertTransactionOutputs(shared_ptr<Transaction> ptx)
        {
            for (const auto& output : ptx->Outputs())
            {
                // Build transaction output
                auto stmt = SetupSqlStatement(R"sql(
                    INSERT OR FAIL INTO TxOutputs (
                        TxHash,
                        Number,
                        AddressHash,
                        Value
                    ) SELECT ?,?,?,?
                    WHERE NOT EXISTS (select 1 from TxOutputs o where o.TxHash=? and o.Number=? and o.AddressHash=?)
                )sql");

                auto result = TryBindStatementText(stmt, 1, ptx->GetHash());
                result &= TryBindStatementInt64(stmt, 2, output->GetNumber());
                result &= TryBindStatementText(stmt, 3, output->GetAddressHash());
                result &= TryBindStatementInt64(stmt, 4, output->GetValue());
                result &= TryBindStatementText(stmt, 5, ptx->GetHash());
                result &= TryBindStatementInt64(stmt, 6, output->GetNumber());
                result &= TryBindStatementText(stmt, 7, output->GetAddressHash());
                if (!result)
                    throw runtime_error(strprintf("%s: can't insert in transaction (bind out)\n", __func__));

                // Try execute with clear last rowId
                if (!TryStepStatement(stmt))
                    throw runtime_error(strprintf("%s: can't insert in transaction (step out): %s\n",
                        __func__, *output->GetTxHash()));
            }

            LogPrint(BCLog::SYNC, "  - Insert Outputs %s : %d\n", *ptx->GetHash(), (int) ptx->Outputs().size());
        }

        void InsertTransactionPayload(shared_ptr<Transaction> ptx)
        {
            auto stmtPayload = SetupSqlStatement(R"sql(
                INSERT OR FAIL INTO Payload (
                    TxHash,
                    String1,
                    String2,
                    String3,
                    String4,
                    String5,
                    String6,
                    String7
                ) SELECT
                    ?,?,?,?,?,?,?,?
                WHERE not exists (select 1 from Payload p where p.TxHash = ?)
            )sql");

            auto resultPayload = true;
            resultPayload &= TryBindStatementText(stmtPayload, 1, ptx->GetHash());
            resultPayload &= TryBindStatementText(stmtPayload, 2, ptx->GetPayload()->GetString1());
            resultPayload &= TryBindStatementText(stmtPayload, 3, ptx->GetPayload()->GetString2());
            resultPayload &= TryBindStatementText(stmtPayload, 4, ptx->GetPayload()->GetString3());
            resultPayload &= TryBindStatementText(stmtPayload, 5, ptx->GetPayload()->GetString4());
            resultPayload &= TryBindStatementText(stmtPayload, 6, ptx->GetPayload()->GetString5());
            resultPayload &= TryBindStatementText(stmtPayload, 7, ptx->GetPayload()->GetString6());
            resultPayload &= TryBindStatementText(stmtPayload, 8, ptx->GetPayload()->GetString7());
            resultPayload &= TryBindStatementText(stmtPayload, 9, ptx->GetHash());
            if (!resultPayload)
                throw runtime_error(
                    strprintf("%s: can't insert in transaction (bind payload)\n", __func__));

            if (!TryStepStatement(stmtPayload))
                throw runtime_error(
                    strprintf("%s: can't insert in transaction (step payload): %s %d\n",
                        __func__, *ptx->GetPayload()->GetTxHash(), *ptx->GetType()));

            LogPrint(BCLog::SYNC, "  - Insert Payload %s\n", *ptx->GetHash());
        }

        void InsertTransactionModel(shared_ptr<Transaction> ptx)
        {
            switch (*ptx->GetType())
            {
                case ACCOUNT_USER:
                    InsertTransactionModelSince((User*) ptx.get());
                    break;
                case ACCOUNT_VIDEO_SERVER:
                    // TODO (brangr): imeplement
                    break;
                case ACCOUNT_MESSAGE_SERVER:
                    // TODO (brangr): implement
                    break;
                default:
                    InsertTransactionModelSince(ptx);
                    break;

                    // TODO (brangr): other types
            }

            LogPrint(BCLog::SYNC, "  - Insert Model body %s : %d\n", *ptx->GetHash(), *ptx->GetType());
        }

        void InsertTransactionModelSince(shared_ptr<Transaction> ptx)
        {
            auto stmt = SetupSqlStatement(R"sql(
                INSERT OR FAIL INTO Transactions (
                    Type,
                    Hash,
                    Time,
                    String1,
                    String2,
                    String3,
                    String4,
                    String5,
                    Int1
                ) SELECT ?,?,?,?,?,?,?,?,?
                WHERE not exists (select 1 from Transactions t where t.Hash=?)
            )sql");

            auto result = TryBindStatementInt(stmt, 1, ptx->GetTypeInt());
            result &= TryBindStatementText(stmt, 2, ptx->GetHash());
            result &= TryBindStatementInt64(stmt, 3, ptx->GetTime());
            result &= TryBindStatementText(stmt, 4, ptx->GetString1());
            result &= TryBindStatementText(stmt, 5, ptx->GetString2());
            result &= TryBindStatementText(stmt, 6, ptx->GetString3());
            result &= TryBindStatementText(stmt, 7, ptx->GetString4());
            result &= TryBindStatementText(stmt, 8, ptx->GetString5());
            result &= TryBindStatementInt64(stmt, 9, ptx->GetInt1());
            result &= TryBindStatementText(stmt, 10, ptx->GetHash());
            if (!result)
                throw runtime_error(strprintf("can't insert in transaction (bind tx) %s %d\n",
                    *ptx->GetHash(), *ptx->GetType()));

            if (!TryStepStatement(stmt))
                throw runtime_error(strprintf("%s: can't insert in transaction (step tx) %s %d\n",
                    __func__, *ptx->GetHash(), *ptx->GetType()));
        }

        void InsertTransactionModelSince(User* ptx)
        {
            auto stmt = SetupSqlStatement(R"sql(
                INSERT OR FAIL INTO Transactions (
                    Type,
                    Hash,
                    Time,
                    String1,
                    String2,
                    Int1
                ) SELECT
                    ?,?,?,?,
                    case
                        when (select count(1) from Transactions t where t.Type=? and t.String1=?)>0
                        then (select max(t.String2) from Transactions t where t.Type=? and t.String1=?)
                        else ?
                    end,
                    ?
                WHERE not exists (select 1 from Transactions t where t.Hash=?)
            )sql");

            auto result = TryBindStatementInt(stmt, 1, ptx->GetTypeInt());
            result &= TryBindStatementText(stmt, 2, ptx->GetHash());
            result &= TryBindStatementInt64(stmt, 3, ptx->GetTime());
            result &= TryBindStatementText(stmt, 4, ptx->GetAddress());
            result &= TryBindStatementInt(stmt, 5, ptx->GetTypeInt());
            result &= TryBindStatementText(stmt, 6, ptx->GetAddress());
            result &= TryBindStatementInt(stmt, 7, ptx->GetTypeInt());
            result &= TryBindStatementText(stmt, 8, ptx->GetAddress());
            result &= TryBindStatementText(stmt, 9, ptx->GetReferrerAddress());
            result &= TryBindStatementInt64(stmt, 10, ptx->GetRegistration());
            result &= TryBindStatementText(stmt, 11, ptx->GetHash());
            if (!result)
                throw runtime_error(strprintf("can't insert in transaction (bind tx) %s %d\n",
                    *ptx->GetHash(), *ptx->GetType()));

            if (!TryStepStatement(stmt))
                throw runtime_error(strprintf("%s: can't insert in transaction (step tx) %s %d\n",
                    __func__, *ptx->GetHash(), *ptx->GetType()));
        }

    };

} // namespace PocketDb

#endif // POCKETDB_TRANSACTIONREPOSITORY_HPP

