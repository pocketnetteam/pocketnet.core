// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include "pocketdb/repositories/TransactionRepository.h"

namespace PocketDb
{
    class TransactionReconstructor : public RowAccessor
    {
    public:
        TransactionReconstructor() {};

        /**
         * Pass a new row for reconstructor to collect all necessary data from it.
         * @param stmt - sqlite stmt. Null is not allowed and will potentially cause a segfault
         * Returns boolean result of collecting data. If false is returned - all data inside reconstructor is possibly corrupted due to bad input (missing columns, etc)
         * and it should not be used anymore
         * 
         * Columns: PartType (0), TxHash (1), ...
         * PartTypes: 0 Tx, 1 Payload, 2 Input, 3 Output
         */
        bool FeedRow(sqlite3_stmt* stmt)
        {
            auto[okPartType, partType] = TryGetColumnInt(stmt, 0);
            if (!okPartType) return false;

            auto[okTxHash, txHash] = TryGetColumnString(stmt, 1);
            if (!okTxHash) return false;

            if (partType > 0 && m_transactions.find(txHash) == m_transactions.end())
                return false;

            switch (partType)
            {
                case 0: {
                    auto[ok, ptx] = ParseTransaction(stmt, txHash);
                    if (!ok) return false;

                    ptx->SetHash(txHash);
                    m_transactions.emplace(txHash, ptx);
                    
                    return true;
                }
                case 1:
                    return ParsePayload(stmt, m_transactions[txHash]);
                case 2:
                    return ParseInput(stmt, m_transactions[txHash], txHash);
                case 3:
                    return ParseOutput(stmt, m_transactions[txHash], txHash);
                default:
                    return false;
            }
        }

        /**
         * Return contructed block.
         */
        PocketBlockRef GetResult()
        {
            PocketBlockRef pocketBlock = make_shared<PocketBlock>();
            for (const auto[txHash, ptx] : m_transactions)
                pocketBlock->push_back(ptx);

            return pocketBlock;
        }

    private:

        map<string, PTransactionRef> m_transactions;

        /**
         * This method parses transaction data, constructs if needed and return construct entry to fill
         * Index:   0  1     2     3     4          5       6     7   8        9        10       11       12       13    14    15
         * Columns: 0, Hash, Type, Time, BlockHash, Height, Last, Id, String1, String2, String3, String4, String5, null, null, Int1
         */
        tuple<bool, PTransactionRef> ParseTransaction(sqlite3_stmt* stmt, const string& txHash)
        {
            // Try get Type and create pocket transaction instance
            auto[okType, txType] = TryGetColumnInt(stmt, 2);
            if (!okType) return {false, nullptr};
            
            PTransactionRef ptx = PocketHelpers::TransactionHelper::CreateInstance(static_cast<TxType>(txType));
            if (!ptx) return {false, nullptr};

            // Required fields
            if (auto[ok, value] = TryGetColumnInt64(stmt, 3); ok) ptx->SetTime(value);
            else return {false, nullptr};

            // Optional fields
            if (auto[ok, value] = TryGetColumnString(stmt, 4); ok) ptx->SetBlockHash(value);
            if (auto[ok, value] = TryGetColumnInt64(stmt, 5); ok) ptx->SetHeight(value);
            if (auto[ok, value] = TryGetColumnInt(stmt, 6); ok) ptx->SetLast(value == 1);
            if (auto[ok, value] = TryGetColumnInt64(stmt, 7); ok) ptx->SetId(value);
            if (auto[ok, value] = TryGetColumnString(stmt, 8); ok) ptx->SetString1(value);
            if (auto[ok, value] = TryGetColumnString(stmt, 9); ok) ptx->SetString2(value);
            if (auto[ok, value] = TryGetColumnString(stmt, 10); ok) ptx->SetString3(value);
            if (auto[ok, value] = TryGetColumnString(stmt, 11); ok) ptx->SetString4(value);
            if (auto[ok, value] = TryGetColumnString(stmt, 12); ok) ptx->SetString5(value);
            if (auto[ok, value] = TryGetColumnInt64(stmt, 15); ok) ptx->SetInt1(value);

            return {true, ptx};
        }

        /**
         * Parse Payload
         * Index:   0  1       2     3     4     5     6     7     8        9        10       11       12       13       14       15
         * Columns: 1, TxHash, null, null, null, null, null, null, String1, String2, String3, String4, String5, String6, String7, Int1
         */
        bool ParsePayload(sqlite3_stmt* stmt, PTransactionRef& ptx)
        {
            ptx->GeneratePayload();

            if (auto[ok, value] = TryGetColumnString(stmt, 8); ok) { ptx->GetPayload()->SetString1(value); }
            if (auto[ok, value] = TryGetColumnString(stmt, 9); ok) { ptx->GetPayload()->SetString2(value); }
            if (auto[ok, value] = TryGetColumnString(stmt, 10); ok) { ptx->GetPayload()->SetString3(value); }
            if (auto[ok, value] = TryGetColumnString(stmt, 11); ok) { ptx->GetPayload()->SetString4(value); }
            if (auto[ok, value] = TryGetColumnString(stmt, 12); ok) { ptx->GetPayload()->SetString5(value); }
            if (auto[ok, value] = TryGetColumnString(stmt, 13); ok) { ptx->GetPayload()->SetString6(value); }
            if (auto[ok, value] = TryGetColumnString(stmt, 14); ok) { ptx->GetPayload()->SetString7(value); }
            if (auto[ok, value] = TryGetColumnInt64(stmt, 15); ok) { ptx->GetPayload()->SetInt1(value); }

            return true;
        }

        /**
         * Parse Inputs
         * Index:   0  1              2     3     4         5         6        7     8              9     10    11    12    13    14    15
         * Columns: 2, i.SpentTxHash, null, null, i.TxHash, i.Number, o.Value, null, o.AddressHash, null, null, null, null, null, null, null
         */
        bool ParseInput(sqlite3_stmt* stmt, PTransactionRef& ptx, const string& txHash)
        {
            bool incomplete = false;

            PTransactionInputRef input = make_shared<TransactionInput>();
            input->SetSpentTxHash(txHash);

            if (auto[ok, value] = TryGetColumnString(stmt, 4); ok) input->SetTxHash(value);
            else incomplete = true;

            if (auto[ok, value] = TryGetColumnInt64(stmt, 5); ok) input->SetNumber(value);
            else incomplete = true;

            if (auto[ok, value] = TryGetColumnInt64(stmt, 6); ok) input->SetValue(value);
            if (auto[ok, value] = TryGetColumnString(stmt, 8); ok) input->SetAddressHash(value);

            ptx->Inputs().push_back(input);
            return !incomplete;
        }

        /**
         * Parse Outputs
         * Index:   0  1       2     3       4            5      6     7     8     9     10            11    12    13    14           15
         * Columns: 3, TxHash, null, Number, AddressHash, Value, null, null, null, null, ScriptPubKey, null, null, null, SpentTxHash, SpentHeight
         */
        bool ParseOutput(sqlite3_stmt* stmt, PTransactionRef& ptx, const string& txHash)
        {
            bool incomplete = false;

            PTransactionOutputRef output = make_shared<TransactionOutput>();
            output->SetTxHash(txHash);

            if (auto[ok, value] = TryGetColumnInt64(stmt, 3); ok) output->SetNumber(value);
            else incomplete = true;

            if (auto[ok, value] = TryGetColumnString(stmt, 4); ok) output->SetAddressHash(value);
            else incomplete = true;

            if (auto[ok, value] = TryGetColumnInt64(stmt, 5); ok) output->SetValue(value);
            else incomplete = true;

            if (auto[ok, value] = TryGetColumnString(stmt, 10); ok) output->SetScriptPubKey(value);
            else incomplete = true;

            if (auto[ok, value] = TryGetColumnString(stmt, 14); ok) output->SetSpentTxHash(value);
            if (auto[ok, value] = TryGetColumnInt64(stmt, 15); ok) output->SetSpentHeight(value);

            ptx->Outputs().push_back(output);
            return !incomplete;
        }
    };

    void TransactionRepository::InsertTransactions(PocketBlock& pocketBlock)
    {
        TryTransactionStep(__func__, [&]()
        {
            for (const auto& ptx: pocketBlock)
            {
                // Insert general transaction
                InsertTransactionModel(ptx);

                // Inputs
                InsertTransactionInputs(ptx);

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

    PocketBlockRef TransactionRepository::List(const vector<string>& txHashes, bool includePayload, bool includeInputs, bool includeOutputs)
    {
        string txReplacers = join(vector<string>(txHashes.size(), "?"), ",");

        auto sql = R"sql(
            select (0)tp, Hash, Type, Time, BlockHash, Height, Last, Id, String1, String2, String3, String4, String5, null, null, Int1
            from Transactions
            where Hash in ( )sql" + txReplacers + R"sql( )
        )sql" +

        // Payload part
        (includePayload ? string(R"sql(
            union
            select (1)tp, TxHash, null, null, null, null, null, null, String1, String2, String3, String4, String5, String6, String7, Int1
            from Payload
            where TxHash in ( )sql" + txReplacers + R"sql( )
        )sql") : "") +

        // Inputs part
        (includeInputs ? string(R"sql(
            union
            select (2)tp, i.SpentTxHash, null, null, i.TxHash, i.Number, o.Value, null, o.AddressHash, null, null, null, null, null, null, null
            from TxInputs i
            join TxOutputs o on o.TxHash = i.TxHash and o.Number = i.Number
            where i.SpentTxHash in ( )sql" + txReplacers + R"sql( )
        )sql") : "") +
        
        // Outputs part
        (includeOutputs ? string(R"sql(
            union
            select (3)tp, TxHash, null, Number, AddressHash, Value, null, null, null, null, ScriptPubKey, null, null, null, SpentTxHash, SpentHeight
            from TxOutputs
            where TxHash in ( )sql" + txReplacers + R"sql( )
        )sql") : "") +

        string(R"sql(
            order by tp asc
        )sql");
        
        TransactionReconstructor reconstructor;

        TryTransactionStep(__func__, [&]()
        {
            auto stmt = SetupSqlStatement(sql);
            size_t i = 1;

            for (auto& txHash : txHashes)
                TryBindStatementText(stmt, i++, txHash);
            
            if (includePayload)
                for (auto& txHash : txHashes)
                    TryBindStatementText(stmt, i++, txHash);

            if (includeInputs)
                for (auto& txHash : txHashes)
                    TryBindStatementText(stmt, i++, txHash);

            if (includeOutputs)
                for (auto& txHash : txHashes)
                    TryBindStatementText(stmt, i++, txHash);

            while (sqlite3_step(*stmt) == SQLITE_ROW)
            {
                // TODO (brangr): maybe throw exception if errors?
                if (!reconstructor.FeedRow(*stmt))
                {
                    break;
                }
            }

            FinalizeSqlStatement(*stmt);
        });

        return reconstructor.GetResult();
    }

    PTransactionRef TransactionRepository::Get(const string& hash, bool includePayload, bool includeInputs, bool includeOutputs)
    {
        auto lst = List({ hash }, includePayload, includeInputs, includeOutputs);
        if (lst && !lst->empty())
            return lst->front();

        return nullptr;
    }

    PTransactionOutputRef TransactionRepository::GetTxOutput(const string& txHash, int number)
    {
        auto sql = R"sql(
            SELECT
                TxHash,
                Number,
                AddressHash,
                Value,
                ScriptPubKey
            FROM TxOutputs
            WHERE TxHash = ?
              and Number = ?
        )sql";

        shared_ptr<TransactionOutput> result = nullptr;
        TryTransactionStep(__func__, [&]()
        {
            auto stmt = SetupSqlStatement(sql);

            TryBindStatementText(stmt, 1, txHash);
            TryBindStatementInt(stmt, 2, number);

            if (sqlite3_step(*stmt) == SQLITE_ROW)
            {
                result = make_shared<TransactionOutput>();
                if (auto[ok, value] = TryGetColumnString(*stmt, 0); ok) result->SetTxHash(value);
                if (auto[ok, value] = TryGetColumnInt64(*stmt, 1); ok) result->SetNumber(value);
                if (auto[ok, value] = TryGetColumnString(*stmt, 2); ok) result->SetAddressHash(value);
                if (auto[ok, value] = TryGetColumnInt64(*stmt, 3); ok) result->SetValue(value);
                if (auto[ok, value] = TryGetColumnString(*stmt, 4); ok) result->SetScriptPubKey(value);
            }

            FinalizeSqlStatement(*stmt);
        });

        return result;
    }

    bool TransactionRepository::Exists(const string& hash)
    {
        bool result = false;

        string sql = R"sql(
            select 1
            from Transactions
            where Hash = ?
        )sql";

        TryTransactionStep(__func__, [&]()
        {
            auto stmt = SetupSqlStatement(sql);

            TryBindStatementText(stmt, 1, hash);

            result = (sqlite3_step(*stmt) == SQLITE_ROW);

            FinalizeSqlStatement(*stmt);
        });

        return result;
    }

    bool TransactionRepository::ExistsInChain(const string& hash)
    {
        bool result = false;

        string sql = R"sql(
            select 1
            from Transactions
            where Hash = ?
              and Height is not null
        )sql";

        TryTransactionStep(__func__, [&]()
        {
            auto stmt = SetupSqlStatement(sql);

            TryBindStatementText(stmt, 1, hash);

            if (sqlite3_step(*stmt) == SQLITE_ROW)
                result = true;

            FinalizeSqlStatement(*stmt);
        });

        return result;
    }

    int TransactionRepository::MempoolCount()
    {
        int result = 0;

        string sql = R"sql(
            select count(*)
            from Transactions
            where Height isnull
              and Type != 3
        )sql";

        TryTransactionStep(__func__, [&]()
        {
            auto stmt = SetupSqlStatement(sql);

            if (sqlite3_step(*stmt) == SQLITE_ROW)
                if (auto[ok, value] = TryGetColumnInt(*stmt, 0); ok)
                    result = value;

            FinalizeSqlStatement(*stmt);
        });

        return result;
    }

    void TransactionRepository::Clean()
    {
        TryTransactionStep(__func__, [&]()
        {
            auto stmt = SetupSqlStatement(R"sql(
                delete from Transactions
                where Height is null
            )sql");

            TryStepStatement(stmt);
        });
    }

    void TransactionRepository::CleanTransaction(const string& hash)
    {
        TryTransactionStep(__func__, [&]()
        {
            // Clear Payload table
            auto stmt = SetupSqlStatement(R"sql(
                delete from Payload
                where TxHash = ?
                  and exists(
                    select 1
                    from Transactions t
                    where t.Hash = Payload.TxHash
                      and t.Height isnull
                  )
            )sql");
            TryBindStatementText(stmt, 1, hash);
            TryStepStatement(stmt);

            // Clear TxOutputs table
            stmt = SetupSqlStatement(R"sql(
                delete from TxOutputs
                where TxHash = ?
                  and exists(
                    select 1
                    from Transactions t
                    where t.Hash = TxOutputs.TxHash
                      and t.Height isnull
                  )
            )sql");
            TryBindStatementText(stmt, 1, hash);
            TryStepStatement(stmt);

            // Clear Transactions table
            stmt = SetupSqlStatement(R"sql(
                delete from Transactions
                where Hash = ?
                  and Height isnull
            )sql");
            TryBindStatementText(stmt, 1, hash);
            TryStepStatement(stmt);
        });
    }

    void TransactionRepository::CleanMempool()
    {
        TryTransactionStep(__func__, [&]()
        {
            // Clear Payload table
            auto stmt = SetupSqlStatement(R"sql(
                delete from Payload
                where TxHash in (
                  select t.Hash
                  from Transactions t
                  where t.Height is null
                )
            )sql");
            TryStepStatement(stmt);

            // Clear TxOutputs table
            stmt = SetupSqlStatement(R"sql(
                delete from TxOutputs
                where TxHash in (
                  select t.Hash
                  from Transactions t
                  where t.Height is null
                )
            )sql");
            TryStepStatement(stmt);

            // Clear Transactions table
            stmt = SetupSqlStatement(R"sql(
                delete from Transactions
                where Height isnull
            )sql");
            TryStepStatement(stmt);
        });
    }

    void TransactionRepository::InsertTransactionInputs(const PTransactionRef& ptx)
    {
        for (const auto& input: ptx->Inputs())
        {
            // Build transaction output
            auto stmt = SetupSqlStatement(R"sql(
                INSERT OR IGNORE INTO TxInputs
                (
                    SpentTxHash,
                    TxHash,
                    Number
                )
                VALUES
                (
                    ?,
                    ?,
                    ?
                )
            )sql");

            TryBindStatementText(stmt, 1, input->GetSpentTxHash());
            TryBindStatementText(stmt, 2, input->GetTxHash());
            TryBindStatementInt64(stmt, 3, input->GetNumber());

            TryStepStatement(stmt);
        }
    }
    
    void TransactionRepository::InsertTransactionOutputs(const PTransactionRef& ptx)
    {
        for (const auto& output: ptx->Outputs())
        {
            // Build transaction output
            auto stmt = SetupSqlStatement(R"sql(
                INSERT OR IGNORE INTO TxOutputs (
                    TxHash,
                    Number,
                    AddressHash,
                    Value,
                    ScriptPubKey
                )
                VALUES
                (
                    ?,
                    ?,
                    ?,
                    ?,
                    ?
                )
            )sql");

            TryBindStatementText(stmt, 1, output->GetTxHash());
            TryBindStatementInt64(stmt, 2, output->GetNumber());
            TryBindStatementText(stmt, 3, output->GetAddressHash());
            TryBindStatementInt64(stmt, 4, output->GetValue());
            TryBindStatementText(stmt, 5, output->GetScriptPubKey());

            TryStepStatement(stmt);
        }
    }

    void TransactionRepository::InsertTransactionPayload(const PTransactionRef& ptx)
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
    }

    void TransactionRepository::InsertTransactionModel(const PTransactionRef& ptx)
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

        TryBindStatementInt(stmt, 1, (int)*ptx->GetType());
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
    }

    tuple<bool, PTransactionRef> TransactionRepository::CreateTransactionFromListRow(
        const shared_ptr<sqlite3_stmt*>& stmt, bool includedPayload)
    {
        // TODO (brangr): move deserialization logic to models ?

        auto[ok0, _txType] = TryGetColumnInt(*stmt, 0);
        auto[ok1, txHash] = TryGetColumnString(*stmt, 1);
        auto[ok2, nTime] = TryGetColumnInt64(*stmt, 2);

        if (!ok0 || !ok1 || !ok2)
            return make_tuple(false, nullptr);

        auto txType = static_cast<TxType>(_txType);
        auto ptx = PocketHelpers::TransactionHelper::CreateInstance(txType);
        ptx->SetTime(nTime);
        ptx->SetHash(txHash);

        if (ptx == nullptr)
            return make_tuple(false, nullptr);

        if (auto[ok, value] = TryGetColumnInt(*stmt, 3); ok) ptx->SetLast(value == 1);
        if (auto[ok, value] = TryGetColumnInt64(*stmt, 4); ok) ptx->SetId(value);
        if (auto[ok, value] = TryGetColumnString(*stmt, 5); ok) ptx->SetString1(value);
        if (auto[ok, value] = TryGetColumnString(*stmt, 6); ok) ptx->SetString2(value);
        if (auto[ok, value] = TryGetColumnString(*stmt, 7); ok) ptx->SetString3(value);
        if (auto[ok, value] = TryGetColumnString(*stmt, 8); ok) ptx->SetString4(value);
        if (auto[ok, value] = TryGetColumnString(*stmt, 9); ok) ptx->SetString5(value);
        if (auto[ok, value] = TryGetColumnInt64(*stmt, 10); ok) ptx->SetInt1(value);

        if (!includedPayload)
            return make_tuple(true, ptx);

        if (auto[ok, value] = TryGetColumnString(*stmt, 11); !ok)
            return make_tuple(true, ptx);

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

} // namespace PocketDb

