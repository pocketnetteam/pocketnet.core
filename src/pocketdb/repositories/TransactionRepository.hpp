// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 Bitcoin developers
// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETDB_TRANSACTIONREPOSITORY_HPP
#define POCKETDB_TRANSACTIONREPOSITORY_HPP

#include "pocketdb/helpers/TypesHelper.hpp"
#include "pocketdb/repositories/BaseRepository.hpp"
#include "pocketdb/models/base/Transaction.hpp"
#include "pocketdb/models/base/TransactionOutput.hpp"
#include "pocketdb/models/base/Rating.hpp"
#include "pocketdb/models/dto/ReturnDtoModels.hpp"
#include "pocketdb/models/dto/User.hpp"
#include "pocketdb/models/dto/ScorePost.hpp"
#include "pocketdb/models/dto/ScoreComment.hpp"

namespace PocketDb
{
    using std::runtime_error;

    using namespace PocketTx;
    using namespace PocketHelpers;

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
        tuple<bool, shared_ptr<ListDto<AddressInfoDto>>> GetAddressInfo(int count)
        {
            shared_ptr<ListDto<AddressInfoDto>> result = make_shared<ListDto<AddressInfoDto>>();

            bool tryResult = TryTransactionStep([&]()
            {
                auto stmt = SetupSqlStatement(R"sql(
                    select o.AddressHash, sum(o.Value)
                    from TxOutputs o
                    where o.SpentHeight is null
                    group by o.AddressHash
                    order by sum(o.Value) desc
                    limit ?
                )sql");

                auto countPtr = make_shared<int>(count);
                if (!TryBindStatementInt(stmt, 1, countPtr))
                {
                    FinalizeSqlStatement(*stmt);
                    throw runtime_error(strprintf("%s: can't select GetAddressInfo (bind)\n", __func__));
                }

                while (sqlite3_step(*stmt) == SQLITE_ROW)
                {
                    AddressInfoDto inf;
                    inf.Address = GetColumnString(*stmt, 0);
                    inf.Balance = GetColumnInt64(*stmt, 1);
                    result->Add(inf);
                }

                FinalizeSqlStatement(*stmt);
                return true;
            });

            return make_tuple(tryResult, result);
        }

        // Selects for get models data
        tuple<bool, ScoreDataDto> GetScoreData(string txHash)
        {
            ScoreDataDto result;

            bool tryResult = TryTransactionStep([&]()
            {
                auto stmt = SetupSqlStatement(R"sql(
                    select
                        s.Hash sTxHash,
                        s.Type sType,
                        s.Time sTime,
                        s.Value sValue,
                        sa.Id saId,
                        sa.AddressHash saHash,
                        c.Hash cTxHash,
                        c.Type cType,
                        c.Time cTime,
                        c.Id cId,
                        ca.Id caId,
                        ca.AddressHash caHash
                    from
                        vScores s
                        join vAccounts sa on sa.AddressHash=s.AddressHash
                        join vContents c on c.Hash=s.ContentTxHash
                        join vAccounts ca on ca.AddressHash=c.AddressHash
                    where s.Hash = ?
                    limit 1
                )sql");

                auto scoreTxHashPtr = make_shared<string>(txHash);
                auto bindResult = TryBindStatementText(stmt, 1, scoreTxHashPtr);
                if (!bindResult)
                {
                    FinalizeSqlStatement(*stmt);
                    throw runtime_error(strprintf("%s: can't select GetScoreData (bind)\n", __func__));
                }

                if (sqlite3_step(*stmt) == SQLITE_ROW)
                {
                    result.ScoreTxHash = GetColumnString(*stmt, 0);
                    result.ScoreType = (PocketTxType) GetColumnInt(*stmt, 1);
                    result.ScoreTime = GetColumnInt64(*stmt, 2);
                    result.ScoreValue = GetColumnInt(*stmt, 3);
                    result.ScoreAddressId = GetColumnInt(*stmt, 4);
                    result.ScoreAddressHash = GetColumnString(*stmt, 5);

                    result.ContentTxHash = GetColumnString(*stmt, 6);
                    result.ContentType = (PocketTxType) GetColumnInt(*stmt, 7);
                    result.ContentTime = GetColumnInt64(*stmt, 8);
                    result.ContentId = GetColumnInt(*stmt, 9);
                    result.ContentAddressId = GetColumnInt(*stmt, 10);
                    result.ContentAddressHash = GetColumnString(*stmt, 11);
                }
                else
                {
                    return false;
                }

                FinalizeSqlStatement(*stmt);
                return true;
            });

            return make_tuple(tryResult, result);
        }

        // Select many referrers
        tuple<bool, map<string, string>> GetReferrers(vector<string> addresses, int minHeight)
        {
            map<string, string> result;

            if (addresses.empty())
                return make_tuple(true, result);

            bool tryResult = TryTransactionStep([&]()
            {
                string sql = R"sql(
                    select a.AddressHash, ifnull(a.ReferrerAddressHash,'')
                    from vAccounts a
                    where a.Height >= ?
                        and a.Height = (select min(a1.Height) from vAccounts a1 where a1.AddressHash=a.AddressHash)
                        and a.ReferrerAddressHash is not null
                )sql";

                sql += " and a.AddressHash in ( ";
                sql += addresses[0];
                for (size_t i = 1; i < addresses.size(); i++)
                {
                    sql += ',';
                    sql += addresses[i];
                }
                sql += ")";

                auto stmt = SetupSqlStatement(sql);

                auto minHeightPtr = make_shared<int>(minHeight);
                auto bindResult = TryBindStatementInt(stmt, 0, minHeightPtr);

                if (!bindResult)
                {
                    FinalizeSqlStatement(*stmt);
                    throw runtime_error(strprintf("%s: can't select GetReferrers (bind)\n", __func__));
                }

                while (sqlite3_step(*stmt) == SQLITE_ROW)
                {
                    auto ref = GetColumnString(*stmt, 2);
                    if (!ref.empty())
                        result.emplace(GetColumnString(*stmt, 1), GetColumnString(*stmt, 2));
                }

                FinalizeSqlStatement(*stmt);
                return true;
            });

            return make_tuple(tryResult, result);
        }

        // Select referrer for one account
        tuple<bool, string> GetReferrer(string address, int minTime)
        {
            string result;

            bool tryResult = TryTransactionStep([&]()
            {
                auto stmt = SetupSqlStatement(R"sql(
                    select ifnull(a.ReferrerAddressHash, '')
                    from vAccounts a
                    where a.Time >= ?
                        and a.Height = (select min(a1.Height) from vAccounts a1 where a1.AddressHash=a.AddressHash)
                        and a.ReferrerAddressHash is not null
                        and a.AddressHash = ?
                )sql");

                auto minTimePtr = make_shared<int>(minTime);
                auto bindResult = TryBindStatementInt(stmt, 1, minTimePtr);

                auto addressPtr = make_shared<string>(address);
                bindResult &= TryBindStatementText(stmt, 2, addressPtr);

                if (!bindResult)
                {
                    FinalizeSqlStatement(*stmt);
                    throw runtime_error(strprintf("%s: can't select GetReferrers (bind)\n", __func__));
                }

                if (sqlite3_step(*stmt) == SQLITE_ROW)
                {
                    result = GetColumnString(*stmt, 0);
                }

                FinalizeSqlStatement(*stmt);
                return !result.empty();
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

            LogPrint(BCLog::SYNC, "  - Insert Model body %s : %d\n", *ptx->GetHash(), *ptx->GetType());
        }

    };

} // namespace PocketDb

#endif // POCKETDB_TRANSACTIONREPOSITORY_HPP

