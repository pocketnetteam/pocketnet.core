// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 Bitcoin developers
// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETDB_TRANSACTIONREPOSITORY_HPP
#define POCKETDB_TRANSACTIONREPOSITORY_HPP

#include "pocketdb/helpers/TransactionHelper.hpp"

#include "pocketdb/repositories/BaseRepository.hpp"
#include "pocketdb/models/base/Transaction.hpp"
#include "pocketdb/models/base/TransactionOutput.hpp"
#include "pocketdb/models/base/Rating.hpp"
#include "pocketdb/models/base/ReturnDtoModels.hpp"
#include "pocketdb/models/dto/User.hpp"
#include "pocketdb/models/dto/ScoreContent.hpp"
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
        void InsertTransactions(PocketBlock& pocketBlock)
        {
            TryTransactionStep([&]()
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
                        InsertTransactionPayload(ptx);
                }
            });
        }

        // Selects for get models data
        // TODO (brangr): move to ConsensusRepository
        shared_ptr<ScoreDataDto> GetScoreData(const string& txHash)
        {
            shared_ptr<ScoreDataDto> result = nullptr;

            auto sql = R"sql(
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
            )sql";

            // ---------------------------------------

            TryTransactionStep([&]()
            {
                auto stmt = SetupSqlStatement(sql);
                TryBindStatementText(stmt, 1, txHash);

                if (sqlite3_step(*stmt) == SQLITE_ROW)
                {
                    ScoreDataDto data;

                    if (auto[ok, value] = TryGetColumnString(*stmt, 0); ok) data.ScoreTxHash = value;
                    if (auto[ok, value] = TryGetColumnInt(*stmt, 1); ok) data.ScoreType = (PocketTxType) value;
                    if (auto[ok, value] = TryGetColumnInt64(*stmt, 2); ok) data.ScoreTime = value;
                    if (auto[ok, value] = TryGetColumnInt(*stmt, 3); ok) data.ScoreValue = value;
                    if (auto[ok, value] = TryGetColumnInt(*stmt, 4); ok) data.ScoreAddressId = value;
                    if (auto[ok, value] = TryGetColumnString(*stmt, 5); ok) data.ScoreAddressHash = value;

                    if (auto[ok, value] = TryGetColumnString(*stmt, 6); ok) data.ContentTxHash = value;
                    if (auto[ok, value] = TryGetColumnInt(*stmt, 7); ok) data.ContentType = (PocketTxType) value;
                    if (auto[ok, value] = TryGetColumnInt64(*stmt, 8); ok) data.ContentTime = value;
                    if (auto[ok, value] = TryGetColumnInt(*stmt, 9); ok) data.ContentId = value;
                    if (auto[ok, value] = TryGetColumnInt(*stmt, 10); ok) data.ContentAddressId = value;
                    if (auto[ok, value] = TryGetColumnString(*stmt, 11); ok) data.ContentAddressHash = value;

                    result = make_shared<ScoreDataDto>(data);
                }

                FinalizeSqlStatement(*stmt);
            });

            return result;
        }

        // Select many referrers
        // TODO (brangr): move to ConsensusRepository
        shared_ptr<map<string, string>> GetReferrers(const vector<string>& addresses, int minHeight)
        {
            shared_ptr<map<string, string>> result = make_shared<map<string, string>>();

            if (addresses.empty())
                return result;

            string sql = R"sql(
                select a.AddressHash, ifnull(a.ReferrerAddressHash,'')
                from vAccounts a
                where a.Height >= ?
                    and a.Height = (select min(a1.Height) from vAccounts a1 where a1.AddressHash=a.AddressHash)
                    and a.ReferrerAddressHash is not null
                    and a.AddressHash in (
            )sql";

            sql += addresses[0];
            for (size_t i = 1; i < addresses.size(); i++)
            {
                sql += ',';
                sql += addresses[i];
            }
            sql += ")";

            // --------------------------------------------

            TryTransactionStep([&]()
            {
                auto stmt = SetupSqlStatement(sql);
                TryBindStatementInt(stmt, 0, minHeight);

                while (sqlite3_step(*stmt) == SQLITE_ROW)
                {
                    if (auto[ok1, value1] = TryGetColumnString(*stmt, 1); ok1 && !value1.empty())
                        if (auto[ok2, value2] = TryGetColumnString(*stmt, 2); ok2 && !value2.empty())
                            result->emplace(value1, value2);
                }

                FinalizeSqlStatement(*stmt);
            });

            return result;
        }

        // Select referrer for one account
        // TODO (brangr): move to ConsensusRepository
        shared_ptr<string> GetReferrer(const string& address, int minTime)
        {
            shared_ptr<string> result;

            auto sql = R"sql(
                select a.ReferrerAddressHash
                from vAccounts a
                where a.Time >= ?
                    and a.AddressHash = ?
                order by a.Height asc
                limit 1
            )sql";

            TryTransactionStep([&]()
            {
                auto stmt = SetupSqlStatement(sql);

                TryBindStatementInt(stmt, 1, minTime);
                TryBindStatementText(stmt, 2, address);

                if (sqlite3_step(*stmt) == SQLITE_ROW)
                {
                    if (auto[ok, value] = TryGetColumnString(*stmt, 0); ok && !value.empty())
                        result = make_shared<string>(value);
                }

                FinalizeSqlStatement(*stmt);
            });

            return result;
        }

        // TODO (joni): стринги в sql не будут работать без бинда
        shared_ptr<PocketBlock> GetList(const vector<string>& txHashes, bool includePayload = false)
        {
            string sql;
            if (!includePayload)
            {
                sql = R"sql(
                    SELECT t.Type, t.Hash, t.Time, t.Height, t.Last, t.Id, t.String1, t.String2, t.String3, t.String4, t.String5, t.Int1
                    FROM Transactions t
                    WHERE 1 = 1
                )sql";
            }
            else
            {
                sql = R"sql(
                    SELECT  t.Type, t.Hash, t.Time, t.Height, t.Last, t.Id, t.String1, t.String2, t.String3, t.String4, t.String5, t.Int1,
                        p.TxHash pTxHash, p.String1 pString1, p.String2 pString2, p.String3 pString3, p.String4 pString4, p.String5 pString5, p.String6 pString6, p.String7 pString7
                    FROM Transactions t
                    LEFT JOIN Payload p on t.Hash = p.TxHash
                    WHERE 1 = 1
                )sql";
            }

            sql += " and t.Hash in ( '";
            sql += txHashes[0];
            sql += "'";
            for (size_t i = 1; i < txHashes.size(); i++)
            {
                sql += ",'";
                sql += txHashes[i];
                sql += "'";
            }
            sql += ")";

            auto result = make_shared<PocketBlock>(PocketBlock{});
            TryTransactionStep([&]()
            {
                auto stmt = SetupSqlStatement(sql);

                while (sqlite3_step(*stmt) == SQLITE_ROW)
                {
                    if (auto[ok, transaction] = CreateTransactionFromListRow(stmt, includePayload); ok)
                        result->push_back(transaction);
                }

                FinalizeSqlStatement(*stmt);
            });

            return result;
        }

        shared_ptr<PocketBlock> GetList(int height, bool includePayload = false)
        {
            string sql;
            if (!includePayload)
            {
                sql = R"sql(
                    SELECT t.Type, t.Hash, t.Time, t.Last, t.Id, t.String1, t.String2, t.String3, t.String4, t.String5, t.Int1
                    FROM Transactions t
                    WHERE Height = ?
                )sql";
            }
            else
            {
                sql = R"sql(
                    SELECT  t.Type, t.Hash, t.Time, t.Last, t.Id, t.String1, t.String2, t.String3, t.String4, t.String5, t.Int1,
                        p.TxHash pTxHash, p.String1 pString1, p.String2 pString2, p.String3 pString3, p.String4 pString4, p.String5 pString5, p.String6 pString6, p.String7 pString7
                    FROM Transactions t
                    LEFT JOIN Payload p on t.Hash = p.TxHash
                    WHERE Height = ?
                )sql";
            }
            auto stmt = SetupSqlStatement(sql);
            auto bindResult = TryBindStatementInt(stmt, 1, make_shared<int>(height));

            if (!bindResult)
            {
                FinalizeSqlStatement(*stmt);
                throw runtime_error(strprintf("%s: can't get list by height (bind out)\n", __func__));
            }

            auto result = make_shared<PocketBlock>(PocketBlock{});

            while (sqlite3_step(*stmt) == SQLITE_ROW)
            {
                if (auto[ok, transaction] = CreateTransactionFromListRow(stmt, includePayload); ok)
                {
                    result->push_back(transaction);
                }
            }

            FinalizeSqlStatement(*stmt);
            return result;
        }

        shared_ptr<Transaction> GetById(const string& hash, bool includePayload = false)
        {
            auto lst = GetList({ hash }, includePayload);
            if (!lst->empty())
                return lst->front();
            
            return nullptr;
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
                    WHERE NOT EXISTS (
                        select 1
                        from TxOutputs o
                        where o.TxHash = ?
                            and o.Number = ?
                            and o.AddressHash = ?
                    )
                )sql");

                TryBindStatementText(stmt, 1, ptx->GetHash());
                TryBindStatementInt64(stmt, 2, output->GetNumber());
                TryBindStatementText(stmt, 3, output->GetAddressHash());
                TryBindStatementInt64(stmt, 4, output->GetValue());
                TryBindStatementText(stmt, 5, ptx->GetHash());
                TryBindStatementInt64(stmt, 6, output->GetNumber());
                TryBindStatementText(stmt, 7, output->GetAddressHash());

                TryStepStatement(stmt);
            }

            LogPrint(BCLog::SYNC, "  - Insert Outputs %s : %d\n", *ptx->GetHash(), (int) ptx->Outputs().size());
        }

        void InsertTransactionPayload(shared_ptr<Transaction> ptx)
        {
            auto stmt = SetupSqlStatement(R"sql(
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

            TryBindStatementText(stmt, 1, ptx->GetHash());
            TryBindStatementText(stmt, 2, ptx->GetPayload()->GetString1());
            TryBindStatementText(stmt, 3, ptx->GetPayload()->GetString2());
            TryBindStatementText(stmt, 4, ptx->GetPayload()->GetString3());
            TryBindStatementText(stmt, 5, ptx->GetPayload()->GetString4());
            TryBindStatementText(stmt, 6, ptx->GetPayload()->GetString5());
            TryBindStatementText(stmt, 7, ptx->GetPayload()->GetString6());
            TryBindStatementText(stmt, 8, ptx->GetPayload()->GetString7());
            TryBindStatementText(stmt, 9, ptx->GetHash());

            TryStepStatement(stmt);

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

            TryBindStatementInt(stmt, 1, ptx->GetTypeInt());
            TryBindStatementText(stmt, 2, ptx->GetHash());
            TryBindStatementInt64(stmt, 3, ptx->GetTime());
            TryBindStatementText(stmt, 4, ptx->GetString1());
            TryBindStatementText(stmt, 5, ptx->GetString2());
            TryBindStatementText(stmt, 6, ptx->GetString3());
            TryBindStatementText(stmt, 7, ptx->GetString4());
            TryBindStatementText(stmt, 8, ptx->GetString5());
            TryBindStatementInt64(stmt, 9, ptx->GetInt1());
            TryBindStatementText(stmt, 10, ptx->GetHash());

            TryStepStatement(stmt);

            LogPrint(BCLog::SYNC, "  - Insert Model body %s : %d\n", *ptx->GetHash(), *ptx->GetType());
        }

    protected:

        tuple<bool, shared_ptr<Transaction>>
        CreateTransactionFromListRow(const shared_ptr<sqlite3_stmt*>& stmt, bool includedPayload)
        {
            auto txType = static_cast<PocketTxType>(GetColumnInt(*stmt, 0));
            auto txHash = GetColumnString(*stmt, 1);
            auto nTime = GetColumnInt64(*stmt, 2);

            auto ptx = PocketHelpers::CreateInstance(txType, txHash, nTime, nullptr);
            if (ptx == nullptr)
            {
                return make_tuple(false, nullptr);
            }

            if (auto[ok, value] = TryGetColumnInt(*stmt, 3); ok) ptx->SetLast(value == 1);
//            if (auto[ok, value] = TryGetColumnInt(*stmt, 4); ok) ptx->SetHeight(value);
            if (auto[ok, value] = TryGetColumnInt64(*stmt, 4); ok) ptx->SetId(value);
            if (auto[ok, value] = TryGetColumnString(*stmt, 5); ok) ptx->SetString1(value);
            if (auto[ok, value] = TryGetColumnString(*stmt, 6); ok) ptx->SetString2(value);
            if (auto[ok, value] = TryGetColumnString(*stmt, 7); ok) ptx->SetString3(value);
            if (auto[ok, value] = TryGetColumnString(*stmt, 8); ok) ptx->SetString4(value);
            if (auto[ok, value] = TryGetColumnString(*stmt, 9); ok) ptx->SetString5(value);
            if (auto[ok, value] = TryGetColumnInt64(*stmt, 10); ok) ptx->SetInt1(value);

            if (!includedPayload)
            {
                return make_tuple(true, ptx);
            }

            auto payload = Payload();
            if (auto[ok, value] = TryGetColumnString(*stmt, 11); ok) payload.SetTxHash(value);
            if (auto[ok, value] = TryGetColumnString(*stmt, 12); ok) payload.SetString1(value);
            if (auto[ok, value] = TryGetColumnString(*stmt, 13); ok) payload.SetString2(value);
            if (auto[ok, value] = TryGetColumnString(*stmt, 14); ok) payload.SetString3(value);
            if (auto[ok, value] = TryGetColumnString(*stmt, 15); ok) payload.SetString4(value);
            if (auto[ok, value] = TryGetColumnString(*stmt, 16); ok) payload.SetString5(value);
            if (auto[ok, value] = TryGetColumnString(*stmt, 17); ok) payload.SetString6(value);
            if (auto[ok, value] = TryGetColumnString(*stmt, 18); ok) payload.SetString7(value);

            ptx->SetPayload(payload);

            return make_tuple(true, ptx);
        }

    };
} // namespace PocketDb

#endif // POCKETDB_TRANSACTIONREPOSITORY_HPP

