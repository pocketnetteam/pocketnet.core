// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 Bitcoin developers
// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETDB_TRANSACTIONREPOSITORY_HPP
#define POCKETDB_TRANSACTIONREPOSITORY_HPP

#include "pocketdb/pocketnet.h"
#include "pocketdb/repositories/BaseRepository.hpp"
#include "pocketdb/models/base/Transaction.hpp"
#include "pocketdb/models/base/TransactionOutput.hpp"
#include "pocketdb/models/base/TransactionInput.hpp"
#include "pocketdb/models/base/SelectModels.hpp"

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

        // ============================================================================================================
        //  Base transaction operations
        bool InsertTransactions(PocketBlock& pocketBlock)
        {
            return TryTransactionStep([&]()
            {
                for (const auto& ptx : pocketBlock)
                {
                    // First insert addresses from Transaction Outputs
                    InsertTransactionOutputAddresses(ptx);

                    // Insert general transaction
                    InsertTransactionModel(ptx);

                    if (ptx->GetId() && ptx->GetId() > 0)
                    {
                        // Outputs & inputs
                        InsertTransactionOutputs(ptx);
                        InsertTransactionInputs(ptx);

                        // Also need insert payload of transaction
                        // But need get new rowId
                        // If last id equal 0 - insert ignored - or already exists or error -> paylod not inserted
                        if (ptx->HasPayload())
                        {
                            InsertTransactionPayload(ptx);
                        }
                    }

                    LogPrint(BCLog::SYNC, "  | -------------------------------------------------\n");
                }
            });
        }

        // ============================================================================================================
        //  SELECTs

        // Top addresses info
        tuple<bool, SelectList<SelectAddressInfo>> SelectTopAddresses(int count)
        {
            SelectList<SelectAddressInfo> result;

            if (ShutdownRequested())
                return make_tuple(false, result);

            bool tryResult = TryTransactionStep([&]()
            {
                auto countPtr = make_shared<int>(count);
                auto stmt = SetupSqlStatement(
                    " SELECT"
                    "   u.Address,"
                    "   SUM(u.Amount)Balance"
                    " FROM Utxo u"
                    " WHERE u.BlockSpent is null"
                    " GROUP BY u.Address"
                    " ORDER BY sum(u.Amount) desc"
                    " LIMIT ?"
                    " ;"
                );

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

        void InsertTransactionOutputAddresses(shared_ptr<Transaction> ptx)
        {
            vector<string> addresses;
            for (auto& pout : ptx->Outputs())
            {
                for (auto& address : pout->Destinations())
                {
                    if (std::find(std::begin(addresses), std::end(addresses), address) ==
                        std::end(addresses))
                        addresses.push_back(address);
                }
            }

            for (const auto& address : addresses)
            {
                auto addressPtr = make_shared<string>(address);

                // Save transaction output address
                auto stmt = SetupSqlStatement(R"sql(
                        INSERT OR FAIL INTO Addresses (Address)
                        SELECT ?
                        WHERE NOT EXISTS (select 1 from Addresses a where a.Address = ?);
                    )sql");

                auto result = true;
                result &= TryBindStatementText(stmt, 1, addressPtr);
                result &= TryBindStatementText(stmt, 2, addressPtr);
                if (!result)
                    throw runtime_error(strprintf("%s: can't insert in transaction (bind addr)\n", __func__));

                if (!TryStepStatement(stmt))
                    throw runtime_error(strprintf("%s: can't insert in transaction (step addr)\n", __func__));
            }

            LogPrint(BCLog::SYNC, "  | Insert Addresses %s : %d\n", *ptx->GetHash(), (int) addresses.size());
        }
        void InsertTransactionOutputs(shared_ptr<Transaction> ptx)
        {
            for (const auto& output : ptx->Outputs())
            {
                // Save transaction output
                auto stmt = SetupSqlStatement(R"sql(
                    INSERT OR FAIL INTO TxOutputs (
                        TxId,
                        Number,
                        Value
                    ) SELECT ?,?,?
                    WHERE NOT EXISTS (select 1 from TxOutputs o where o.TxId=? and o.Number=?)
                )sql");

                auto result = TryBindStatementInt64(stmt, 1, ptx->GetId());
                result &= TryBindStatementInt64(stmt, 2, output->GetNumber());
                result &= TryBindStatementInt64(stmt, 3, output->GetValue());
                result &= TryBindStatementInt64(stmt, 4, ptx->GetId());
                result &= TryBindStatementInt64(stmt, 5, output->GetNumber());
                if (!result)
                    throw runtime_error(strprintf("%s: can't insert in transaction (bind out)\n", __func__));

                // Try execute with clear last rowId
                if (!TryStepStatement(stmt))
                    throw runtime_error(strprintf("%s: can't insert in transaction (step out): %s\n",
                        __func__, *output->GetTxHash()));

                // Also need save destination addresses
                for (const auto& dest : output->Destinations())
                {
                    auto destPtr = make_shared<string>(dest);
                    auto stmtDest = SetupSqlStatement(R"sql(
                        INSERT OR FAIL INTO TxOutputsDestinations (
                            TxId,
                            Number,
                            AddressId
                        ) SELECT
                            ?,
                            ?,
                            (select a.Id from Addresses a where a.Address=? limit 1)
                        WHERE NOT EXISTS (select 1 from TxOutputsDestinations od where od.TxId=? and od.Number=?)
                    )sql");

                    auto resultDest = TryBindStatementInt64(stmtDest, 1, ptx->GetId());
                    resultDest &= TryBindStatementInt64(stmtDest, 2, output->GetNumber());
                    resultDest &= TryBindStatementText(stmtDest, 3, destPtr);
                    resultDest &= TryBindStatementInt64(stmtDest, 4, ptx->GetId());
                    resultDest &= TryBindStatementInt64(stmtDest, 5, output->GetNumber());
                    if (!resultDest)
                        throw runtime_error(
                            strprintf("%s: can't insert in transaction (bind outdest)\n", __func__));

                    // Try execute with clear last rowId
                    if (!TryStepStatement(stmtDest))
                        throw runtime_error(strprintf("%s: can't insert in transaction (step outdest): %s\n",
                            __func__, dest));
                }
            }

            LogPrint(BCLog::SYNC, "  | Insert Outputs %s : %d\n", *ptx->GetHash(), (int) ptx->Outputs().size());
        }
        void InsertTransactionInputs(shared_ptr<Transaction> ptx)
        {
            for (const auto& input : ptx->Inputs())
            {
                // Build sql statement with auto select IDs
                auto stmt = SetupSqlStatement(R"sql(
                    INSERT OR FAIL INTO TxInputs (
                        TxId,
                        InputTxId,
                        InputTxNumber
                    ) SELECT
                        ?,
                        (select t.Id from Transactions t where t.Hash=?),
                        ?
                    WHERE NOT EXISTS (
                        select 1
                        from TxInputs i
                        where i.TxId=?
                            and i.InputTxId=(select t.Id from Transactions t where t.Hash=?)
                            and i.InputTxNumber=?
                    )
                )sql");

                // Bind arguments
                auto result = TryBindStatementInt64(stmt, 1, ptx->GetId());
                result &= TryBindStatementText(stmt, 2, input->GetInputTxHash());
                result &= TryBindStatementInt64(stmt, 3, input->GetInputTxNumber());
                result &= TryBindStatementInt64(stmt, 4, ptx->GetId());
                result &= TryBindStatementText(stmt, 5, input->GetInputTxHash());
                result &= TryBindStatementInt64(stmt, 6, input->GetInputTxNumber());
                if (!result)
                    throw runtime_error(strprintf("%s: can't insert in transaction (bind inp)\n", __func__));

                // Try execute
                if (!TryStepStatement(stmt))
                    throw runtime_error(strprintf("%s: can't insert in transaction (step inp) TxId:%d InputTxId:%s\n",
                        __func__, *ptx->GetId(), *input->GetInputTxHash()));
            }

            LogPrint(BCLog::SYNC, "  | Insert Inputs %s : %d\n", *ptx->GetHash(), (int) ptx->Inputs().size());
        }
        void InsertTransactionPayload(shared_ptr<Transaction> ptx)
        {
            auto stmtPayload = SetupSqlStatement(R"sql(
                INSERT OR FAIL INTO Payload (
                    TxId,
                    String1,
                    String2,
                    String3,
                    String4,
                    String5,
                    String6,
                    String7
                ) SELECT
                    ?,?,?,?,?,?,?,?
                WHERE not exists (select 1 from Payload p where p.TxId = ?)
            )sql");

            auto resultPayload = true;
            resultPayload &= TryBindStatementInt64(stmtPayload, 1, ptx->GetId());
            resultPayload &= TryBindStatementText(stmtPayload, 2, ptx->GetPayload()->GetString1());
            resultPayload &= TryBindStatementText(stmtPayload, 3, ptx->GetPayload()->GetString2());
            resultPayload &= TryBindStatementText(stmtPayload, 4, ptx->GetPayload()->GetString3());
            resultPayload &= TryBindStatementText(stmtPayload, 5, ptx->GetPayload()->GetString4());
            resultPayload &= TryBindStatementText(stmtPayload, 6, ptx->GetPayload()->GetString5());
            resultPayload &= TryBindStatementText(stmtPayload, 7, ptx->GetPayload()->GetString6());
            resultPayload &= TryBindStatementText(stmtPayload, 8, ptx->GetPayload()->GetString7());
            resultPayload &= TryBindStatementInt64(stmtPayload, 9, ptx->GetId());
            if (!resultPayload)
                throw runtime_error(
                    strprintf("%s: can't insert in transaction (bind payload)\n", __func__));

            if (!TryStepStatement(stmtPayload))
                throw runtime_error(
                    strprintf("%s: can't insert in transaction (step payload): %s %d\n",
                        __func__, *ptx->GetPayload()->GetTxHash(), *ptx->GetType()));

            LogPrint(BCLog::SYNC, "  | Insert Payload %s\n", *ptx->GetHash());
        }
        void InsertTransactionModel(shared_ptr<Transaction> ptx)
        {
            shared_ptr<sqlite3_stmt*> stmt = nullptr;

            // For different tx types, we define different behaviors of anonymous fields
            switch (*ptx->GetType())
            {
                case TX_COINBASE:
                case TX_COINSTAKE:
                case TX_DEFAULT:
                    stmt = BuildModelStatement(ptx);
                    break;
                case ACCOUNT_USER:
                    stmt = BuildModelStatement((User*) ptx.get());
                    break;
                case CONTENT_POST:
                    stmt = BuildModelStatement((Post*) ptx.get());
                    break;
                case CONTENT_COMMENT:
                    stmt = BuildModelStatement((Comment*) ptx.get());
                    break;
                case ACTION_SCORE_POST:
                    stmt = BuildModelStatement((ScorePost*) ptx.get());
                    break;
                case ACTION_SCORE_COMMENT:
                    stmt = BuildModelStatement((ScoreComment*) ptx.get());
                    break;
                case ACTION_SUBSCRIBE:
                    stmt = BuildModelStatement((Subscribe*) ptx.get());
                    break;
                case ACTION_SUBSCRIBE_PRIVATE:
                    stmt = BuildModelStatement((SubscribePrivate*) ptx.get());
                    break;
                case ACTION_SUBSCRIBE_CANCEL:
                    stmt = BuildModelStatement((SubscribeCancel*) ptx.get());
                    break;
                case ACTION_BLOCKING:
                    stmt = BuildModelStatement((Blocking*) ptx.get());
                    break;
                case ACTION_BLOCKING_CANCEL:
                    stmt = BuildModelStatement((BlockingCancel*) ptx.get());
                    break;
                case ACTION_COMPLAIN:
                    stmt = BuildModelStatement((Complain*) ptx.get());
                    break;

                case CONTENT_VIDEO:
                case CONTENT_TRANSLATE:
                case CONTENT_SERVERPING:
                case ACCOUNT_VIDEO_SERVER:
                case ACCOUNT_MESSAGE_SERVER:
                    throw runtime_error(strprintf("Not supported tx type %s %d\n", *ptx->GetHash(), *ptx->GetType()));
                default:
                    throw runtime_error(strprintf("Unknown tx type %s %d\n", *ptx->GetHash(), *ptx->GetType()));
            }

            if (!stmt)
                throw runtime_error(strprintf("Unknown error tx type %s %d\n", *ptx->GetHash(), *ptx->GetType()));

            sqlite3_set_last_insert_rowid(m_database.m_db, 0);
            if (!TryStepStatement(stmt))
                throw runtime_error(strprintf("%s: can't insert in transaction (step tx)\n", __func__));

            ptx->SetId(sqlite3_last_insert_rowid(m_database.m_db));

            LogPrint(BCLog::SYNC, "  | Insert Model body %s : %d\n", *ptx->GetHash(), *ptx->GetId());
        }

        shared_ptr<sqlite3_stmt*> BuildModelStatement(shared_ptr<Transaction> ptx)
        {
            auto stmt = SetupSqlStatement(R"sql(
                INSERT OR FAIL INTO Transactions (
                    Type,
                    Hash,
                    Time
                ) SELECT ?,?,?
                WHERE not exists (select 1 from Transactions t where t.Hash=?)
            )sql");

            auto result = TryBindStatementInt(stmt, 1, ptx->GetTypeInt());
            result &= TryBindStatementText(stmt, 2, ptx->GetHash());
            result &= TryBindStatementInt64(stmt, 3, ptx->GetTime());
            result &= TryBindStatementText(stmt, 4, ptx->GetHash());

            if (!result)
                throw runtime_error(strprintf("BuildModelStatement %s %d\n", *ptx->GetHash(), *ptx->GetType()));

            return stmt;
        }

        shared_ptr<sqlite3_stmt*> BuildModelStatement(User* ptx)
        {
            auto stmt = SetupSqlStatement(R"sql(
                INSERT OR FAIL INTO Transactions (
                    Type,
                    Hash,
                    Time,
                    AddressId,
                    Int1,
                    Int2
                ) SELECT ?,?,?,
                    (select a.Id from Addresses a where a.Address=?),
                    ifnull((
                        select t.Int2 from Transactions t where t.Id=(
                            select min(t1.Id) from Transactions t1 where t1.AddressId=(
                                select a.Id from Addresses a where a.Address=?
                            )
                        )
                    ),?),
                    (select a.Id from Addresses a where a.Address=?)
                WHERE not exists (select 1 from Transactions t where t.Hash=?)
            )sql");

            auto result = TryBindStatementInt(stmt, 1, ptx->GetTypeInt());
            result &= TryBindStatementText(stmt, 2, ptx->GetHash());
            result &= TryBindStatementInt64(stmt, 3, ptx->GetTime());
            result &= TryBindStatementText(stmt, 4, ptx->GetAccountAddress());
            result &= TryBindStatementText(stmt, 5, ptx->GetAccountAddress());
            result &= TryBindStatementInt64(stmt, 6, ptx->GetRegistration());
            result &= TryBindStatementText(stmt, 7, ptx->GetReferrerAddress());
            result &= TryBindStatementText(stmt, 8, ptx->GetHash());

            if (!result)
                throw runtime_error(strprintf("BuildModelStatement %s %d\n", *ptx->GetHash(), *ptx->GetType()));

            return stmt;
        }

        shared_ptr<sqlite3_stmt*> BuildModelStatement(Post* ptx)
        {
            auto stmt = SetupSqlStatement(R"sql(
                INSERT OR FAIL INTO Transactions (
                    Type,
                    Hash,
                    Time,
                    AddressId,
                    Int1,
                    Int2
                ) SELECT ?,?,?,
                    (select a.Id from Addresses a where a.Address=?),
                    (select t.Id from Transactions t where t.Hash=?),
                    (select t.Id from Transactions t where t.Hash=?)
                WHERE not exists (select 1 from Transactions t where t.Hash=?)
            )sql");

            auto result = TryBindStatementInt(stmt, 1, ptx->GetTypeInt());
            result &= TryBindStatementText(stmt, 2, ptx->GetHash());
            result &= TryBindStatementInt64(stmt, 3, ptx->GetTime());
            result &= TryBindStatementText(stmt, 4, ptx->GetAccountAddress());
            result &= TryBindStatementText(stmt, 5, ptx->GetRootTxHash());
            result &= TryBindStatementText(stmt, 6, ptx->GetRelayTxHash());
            result &= TryBindStatementText(stmt, 7, ptx->GetHash());

            if (!result)
                throw runtime_error(strprintf("BuildModelStatement %s %d\n", *ptx->GetHash(), *ptx->GetType()));

            return stmt;
        }

        shared_ptr<sqlite3_stmt*> BuildModelStatement(Comment* ptx)
        {
            auto stmt = SetupSqlStatement(R"sql(
                INSERT OR FAIL INTO Transactions (
                    Type,
                    Hash,
                    Time,
                    AddressId,
                    Int1,
                    Int2,
                    Int3,
                    Int4
                ) SELECT ?,?,?,
                    (select a.Id from Addresses a where a.Address=?),
                    (select t.Id from Transactions t where t.Hash=?),
                    (select t.Id from Transactions t where t.Hash=?),
                    (select t.Id from Transactions t where t.Hash=?),
                    (select t.Id from Transactions t where t.Hash=?)
                WHERE not exists (select 1 from Transactions t where t.Hash=?)
            )sql");

            auto result = TryBindStatementInt(stmt, 1, ptx->GetTypeInt());
            result &= TryBindStatementText(stmt, 2, ptx->GetHash());
            result &= TryBindStatementInt64(stmt, 3, ptx->GetTime());
            result &= TryBindStatementText(stmt, 4, ptx->GetAccountAddress());
            result &= TryBindStatementText(stmt, 5, ptx->GetRootTxHash());
            result &= TryBindStatementText(stmt, 6, ptx->GetPostTxHash());
            result &= TryBindStatementText(stmt, 7, ptx->GetParentTxHash());
            result &= TryBindStatementText(stmt, 8, ptx->GetAnswerTxHash());
            result &= TryBindStatementText(stmt, 9, ptx->GetHash());

            if (!result)
                throw runtime_error(strprintf("BuildModelStatement %s %d\n", *ptx->GetHash(), *ptx->GetType()));

            return stmt;
        }

        shared_ptr<sqlite3_stmt*> BuildModelStatement(Blocking* ptx)
        {
            auto stmt = SetupSqlStatement(R"sql(
                INSERT OR FAIL INTO Transactions (
                    Type,
                    Hash,
                    Time,
                    AddressId,
                    Int1
                ) SELECT ?,?,?,
                    (select a.Id from Addresses a where a.Address=?),
                    (select a.Id from Addresses a where a.Address=?)
                WHERE not exists (select 1 from Transactions t where t.Hash=?)
            )sql");

            auto result = TryBindStatementInt(stmt, 1, ptx->GetTypeInt());
            result &= TryBindStatementText(stmt, 2, ptx->GetHash());
            result &= TryBindStatementInt64(stmt, 3, ptx->GetTime());
            result &= TryBindStatementText(stmt, 4, ptx->GetAccountAddress());
            result &= TryBindStatementText(stmt, 5, ptx->GetAddressTo());
            result &= TryBindStatementText(stmt, 6, ptx->GetHash());

            if (!result)
                throw runtime_error(strprintf("BuildModelStatement %s %d\n", *ptx->GetHash(), *ptx->GetType()));

            return stmt;
        }

        shared_ptr<sqlite3_stmt*> BuildModelStatement(BlockingCancel* ptx)
        {
            auto stmt = SetupSqlStatement(R"sql(
                INSERT OR FAIL INTO Transactions (
                    Type,
                    Hash,
                    Time,
                    AddressId,
                    Int1
                ) SELECT ?,?,?,
                    (select a.Id from Addresses a where a.Address=?),
                    (select a.Id from Addresses a where a.Address=?)
                WHERE not exists (select 1 from Transactions t where t.Hash=?)
            )sql");

            auto result = TryBindStatementInt(stmt, 1, ptx->GetTypeInt());
            result &= TryBindStatementText(stmt, 2, ptx->GetHash());
            result &= TryBindStatementInt64(stmt, 3, ptx->GetTime());
            result &= TryBindStatementText(stmt, 4, ptx->GetAccountAddress());
            result &= TryBindStatementText(stmt, 5, ptx->GetAddressTo());
            result &= TryBindStatementText(stmt, 6, ptx->GetHash());

            if (!result)
                throw runtime_error(strprintf("BuildModelStatement %s %d\n", *ptx->GetHash(), *ptx->GetType()));

            return stmt;
        }

        shared_ptr<sqlite3_stmt*> BuildModelStatement(Complain* ptx)
        {
            auto stmt = SetupSqlStatement(R"sql(
                INSERT OR FAIL INTO Transactions (
                    Type,
                    Hash,
                    Time,
                    AddressId,
                    Int1,
                    Int2
                ) SELECT ?,?,?,
                    (select a.Id from Addresses a where a.Address=?),
                    (select t.Id from Transactions t where t.Hash=?),
                    ?
                WHERE not exists (select 1 from Transactions t where t.Hash=?)
            )sql");

            auto result = TryBindStatementInt(stmt, 1, ptx->GetTypeInt());
            result &= TryBindStatementText(stmt, 2, ptx->GetHash());
            result &= TryBindStatementInt64(stmt, 3, ptx->GetTime());
            result &= TryBindStatementText(stmt, 4, ptx->GetAccountAddress());
            result &= TryBindStatementText(stmt, 5, ptx->GetPostTxHash());
            result &= TryBindStatementInt64(stmt, 6, ptx->GetReason());
            result &= TryBindStatementText(stmt, 7, ptx->GetHash());

            if (!result)
                throw runtime_error(strprintf("BuildModelStatement %s %d\n", *ptx->GetHash(), *ptx->GetType()));

            return stmt;
        }

        shared_ptr<sqlite3_stmt*> BuildModelStatement(ScorePost* ptx)
        {
            auto stmt = SetupSqlStatement(R"sql(
                INSERT OR FAIL INTO Transactions (
                    Type,
                    Hash,
                    Time,
                    AddressId,
                    Int1,
                    Int2
                ) SELECT ?,?,?,
                    (select a.Id from Addresses a where a.Address=?),
                    (select t.Id from Transactions t where t.Hash=?),
                    ?
                WHERE not exists (select 1 from Transactions t where t.Hash=?)
            )sql");

            auto result = TryBindStatementInt(stmt, 1, ptx->GetTypeInt());
            result &= TryBindStatementText(stmt, 2, ptx->GetHash());
            result &= TryBindStatementInt64(stmt, 3, ptx->GetTime());
            result &= TryBindStatementText(stmt, 4, ptx->GetAccountAddress());
            result &= TryBindStatementText(stmt, 5, ptx->GetPostTxHash());
            result &= TryBindStatementInt64(stmt, 6, ptx->GetValue());
            result &= TryBindStatementText(stmt, 7, ptx->GetHash());

            if (!result)
                throw runtime_error(strprintf("BuildModelStatement %s %d\n", *ptx->GetHash(), *ptx->GetType()));

            return stmt;
        }

        shared_ptr<sqlite3_stmt*> BuildModelStatement(ScoreComment* ptx)
        {
            auto stmt = SetupSqlStatement(R"sql(
                INSERT OR FAIL INTO Transactions (
                    Type,
                    Hash,
                    Time,
                    AddressId,
                    Int1,
                    Int2
                ) SELECT ?,?,?,
                    (select a.Id from Addresses a where a.Address=?),
                    (select t.Id from Transactions t where t.Hash=?),
                    ?
                WHERE not exists (select 1 from Transactions t where t.Hash=?)
            )sql");

            auto result = TryBindStatementInt(stmt, 1, ptx->GetTypeInt());
            result &= TryBindStatementText(stmt, 2, ptx->GetHash());
            result &= TryBindStatementInt64(stmt, 3, ptx->GetTime());
            result &= TryBindStatementText(stmt, 4, ptx->GetAccountAddress());
            result &= TryBindStatementText(stmt, 5, ptx->GetCommentTxHash());
            result &= TryBindStatementInt64(stmt, 6, ptx->GetValue());
            result &= TryBindStatementText(stmt, 7, ptx->GetHash());

            if (!result)
                throw runtime_error(strprintf("BuildModelStatement %s %d\n", *ptx->GetHash(), *ptx->GetType()));

            return stmt;
        }

        shared_ptr<sqlite3_stmt*> BuildModelStatement(Subscribe* ptx)
        {
            auto stmt = SetupSqlStatement(R"sql(
                INSERT OR FAIL INTO Transactions (
                    Type,
                    Hash,
                    Time,
                    AddressId,
                    Int1
                ) SELECT ?,?,?,
                    (select a.Id from Addresses a where a.Address=?),
                    (select a.Id from Addresses a where a.Address=?)
                WHERE not exists (select 1 from Transactions t where t.Hash=?)
            )sql");

            auto result = TryBindStatementInt(stmt, 1, ptx->GetTypeInt());
            result &= TryBindStatementText(stmt, 2, ptx->GetHash());
            result &= TryBindStatementInt64(stmt, 3, ptx->GetTime());
            result &= TryBindStatementText(stmt, 4, ptx->GetAccountAddress());
            result &= TryBindStatementText(stmt, 5, ptx->GetAddressTo());
            result &= TryBindStatementText(stmt, 6, ptx->GetHash());

            if (!result)
                throw runtime_error(strprintf("BuildModelStatement %s %d\n", *ptx->GetHash(), *ptx->GetType()));

            return stmt;
        }

        shared_ptr<sqlite3_stmt*> BuildModelStatement(SubscribeCancel* ptx)
        {
            auto stmt = SetupSqlStatement(R"sql(
                INSERT OR FAIL INTO Transactions (
                    Type,
                    Hash,
                    Time,
                    AddressId,
                    Int1
                ) SELECT ?,?,?,
                    (select a.Id from Addresses a where a.Address=?),
                    (select a.Id from Addresses a where a.Address=?)
                WHERE not exists (select 1 from Transactions t where t.Hash=?)
            )sql");

            auto result = TryBindStatementInt(stmt, 1, ptx->GetTypeInt());
            result &= TryBindStatementText(stmt, 2, ptx->GetHash());
            result &= TryBindStatementInt64(stmt, 3, ptx->GetTime());
            result &= TryBindStatementText(stmt, 4, ptx->GetAccountAddress());
            result &= TryBindStatementText(stmt, 5, ptx->GetAddressTo());
            result &= TryBindStatementText(stmt, 6, ptx->GetHash());

            if (!result)
                throw runtime_error(strprintf("BuildModelStatement %s %d\n", *ptx->GetHash(), *ptx->GetType()));

            return stmt;
        }

        shared_ptr<sqlite3_stmt*> BuildModelStatement(SubscribePrivate* ptx)
        {
            auto stmt = SetupSqlStatement(R"sql(
                INSERT OR FAIL INTO Transactions (
                    Type,
                    Hash,
                    Time,
                    AddressId,
                    Int1
                ) SELECT ?,?,?,
                    (select a.Id from Addresses a where a.Address=?),
                    (select a.Id from Addresses a where a.Address=?)
                WHERE not exists (select 1 from Transactions t where t.Hash=?)
            )sql");

            auto result = TryBindStatementInt(stmt, 1, ptx->GetTypeInt());
            result &= TryBindStatementText(stmt, 2, ptx->GetHash());
            result &= TryBindStatementInt64(stmt, 3, ptx->GetTime());
            result &= TryBindStatementText(stmt, 4, ptx->GetAccountAddress());
            result &= TryBindStatementText(stmt, 5, ptx->GetAddressTo());
            result &= TryBindStatementText(stmt, 6, ptx->GetHash());

            if (!result)
                throw runtime_error(strprintf("BuildModelStatement %s %d\n", *ptx->GetHash(), *ptx->GetType()));

            return stmt;
        }

    };

} // namespace PocketDb

#endif // POCKETDB_TRANSACTIONREPOSITORY_HPP

