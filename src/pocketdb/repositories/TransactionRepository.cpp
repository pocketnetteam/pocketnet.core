// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include "pocketdb/repositories/TransactionRepository.h"

namespace PocketDb
{
    class TransactionReconstructor : public RowAccessor
    {
    public:
        TransactionReconstructor(bool includePayload, bool includeOutputs, bool includeInputs)
            : m_includePayload(includePayload), m_includeOutputs(includeOutputs), m_includeInputs(includeInputs)
        {}
        TransactionReconstructor() = delete;

        /**
         * Pass a new row for reconstructor to collect all necessary data from it.
         * @param stmt - sqlite stmt. Null is not allowed and will potentially cause a segfault
         * Returns boolean result of collecting data. If false is returned - all data inside reconstructor is possibly corrupted due to bad input (missing columns, etc)
         * and it should not be used anymore
         */
        bool FeedRow(sqlite3_stmt* stmt)
        {
            auto[ok0, txHash] = TryGetColumnString(stmt, 1);
            if (!ok0) {
                return false;
            }

            // Using columns counter because other columns numbers depend on include* flags
            // It is controlled inside below methods
            int currentColumn = 0;

            auto [ok1, constructResult] = ProcessTransaction(stmt, txHash, currentColumn);
            if (!ok1 || constructResult == nullptr) {
                return false;
            }

            // The order of below functions is necessary. It should correspond to order of joins in query.
            if (m_includeOutputs && !ParseOutput(stmt, *constructResult, currentColumn)) {
                return false;
            }
            if (m_includeInputs && !ParseInput(stmt, *constructResult, currentColumn)) {
                return false;
            }
            
            return true;
        }

        /**
         * Constructs all collected data to PocketBlock format
         */
        PocketBlock GetResult()
        {
            PocketBlock result;
            for (auto& constructResultPair: m_constructed) {
                auto& constructResult = *constructResultPair.second;

                auto tx = constructResult.tx;
                if (constructResult.payload.has_value()) {
                    tx->SetPayload(constructResult.payload.value());
                }

                auto& outputs = tx->Outputs();
                outputs.clear();
                outputs.reserve(constructResult.outputs.size());
                for (const auto& outputPair: constructResult.outputs) {
                    outputs.emplace_back(outputPair.second);
                }

                result.emplace_back(tx);
            }
            return result;
        }
    private:
        /**
         * Used to collect all data for constructing transaction.
         * It is needed to avoid duplicates of payload and outputs in case of multiple joins because it uses optional and map
         * instead of object and vector in Transaction.
         */
        class ConstructEntry
        {
        public:
            /**
             * Transaction is going to be filled only with data from "Transactions" table
             */
            PTransactionRef tx;
            /**
             * Will be null and not included to transaction during constructing if includePayload == false.
             * Otherwise, will be included even if there is no payload for transaction
             */
            std::optional<Payload> payload;
            /**
             * Map for outputs where key is "Number" from TxOutputs table
             */
            std::map<int64_t, std::shared_ptr<TransactionOutput>> outputs;

            // TODO (team): Inputs here too
        };
        /**
         * Map that is filled on every FeedRow(). TxHash is used as a key.
         */
        std::map<std::string, std::shared_ptr<ConstructEntry>> m_constructed;

        bool m_includePayload = false;
        bool m_includeOutputs = false;
        bool m_includeInputs = false;

        /**
         * This method parses transaction data, constructs if needed and return construct entry to fill
         */
        tuple<bool, std::shared_ptr<ConstructEntry>> ProcessTransaction(sqlite3_stmt* stmt, const std::string& txHash, int& currentColumn)
        {
            auto tx = m_constructed.find(txHash);
            if (tx == m_constructed.end()) {
                // Transaction's columns are hardcoded because they are always the same and should always be presented.
                // Probably this can be generalized in future
                auto[ok1, _txType] = TryGetColumnInt(stmt, 0);
                auto[ok2, nTime] = TryGetColumnInt64(stmt, 2);

                if (!ok1 || !ok2)
                    return {false, nullptr};

                auto txType = static_cast<TxType>(_txType);
                auto ptx = PocketHelpers::TransactionHelper::CreateInstance(txType);
                if (ptx == nullptr)
                    return {false, nullptr};
                
                ptx->SetTime(nTime);
                ptx->SetHash(txHash);

                if (auto[ok, value] = TryGetColumnInt(stmt, 3); ok) ptx->SetLast(value == 1);
                if (auto[ok, value] = TryGetColumnInt64(stmt, 4); ok) ptx->SetId(value);
                if (auto[ok, value] = TryGetColumnString(stmt, 5); ok) ptx->SetString1(value);
                if (auto[ok, value] = TryGetColumnString(stmt, 6); ok) ptx->SetString2(value);
                if (auto[ok, value] = TryGetColumnString(stmt, 7); ok) ptx->SetString3(value);
                if (auto[ok, value] = TryGetColumnString(stmt, 8); ok) ptx->SetString4(value);
                if (auto[ok, value] = TryGetColumnString(stmt, 9); ok) ptx->SetString5(value);
                if (auto[ok, value] = TryGetColumnInt64(stmt, 10); ok) ptx->SetInt1(value);

                // Creating construct entry that will be associated with current transaction and filled with payload, outputs, inputs, etc inside
                auto currentTx = std::make_shared<ConstructEntry>();
                currentTx->tx = std::move(ptx);
                m_constructed.insert({txHash, currentTx});

                currentColumn = 11;
                // Parsing single joins here results in some optimizing because we will not try to parse them later
                // only after one check if transaction has already parsed
                auto singleJoinsRes = ParseSingleJoins(stmt, *currentTx, currentColumn);
                return {singleJoinsRes, currentTx};
            } else {
                // TODO (losty): Ugly hardcoded.
                currentColumn = 19;
                return {true, tx->second};
            }
        }

        /**
         * This method is used inside ProcessTransaction to optimize and collect all one-row joins
         * only once at time collecting transaction data.
         */
        bool ParseSingleJoins(sqlite3_stmt* stmt, ConstructEntry& result, int& currentColumn)
        {
            if (m_includePayload && !ParsePayload(stmt, result, currentColumn)) {
                return false;
            }

            return true;
        }

        bool ParsePayload(sqlite3_stmt* stmt, ConstructEntry& result, int& currentColumn)
        {
            if (result.payload.has_value()) {
                // Skipping payload columns because they have been already parsed
                currentColumn+= 8;
                return true;
            }

            auto payload = Payload();

            if (auto[ok, value] = TryGetColumnString(stmt, currentColumn); ok) payload.SetTxHash(value); currentColumn++;
            if (auto[ok, value] = TryGetColumnString(stmt, currentColumn); ok) payload.SetString1(value); currentColumn++;
            if (auto[ok, value] = TryGetColumnString(stmt, currentColumn); ok) payload.SetString2(value); currentColumn++;
            if (auto[ok, value] = TryGetColumnString(stmt, currentColumn); ok) payload.SetString3(value); currentColumn++;
            if (auto[ok, value] = TryGetColumnString(stmt, currentColumn); ok) payload.SetString4(value); currentColumn++;
            if (auto[ok, value] = TryGetColumnString(stmt, currentColumn); ok) payload.SetString5(value); currentColumn++;
            if (auto[ok, value] = TryGetColumnString(stmt, currentColumn); ok) payload.SetString6(value); currentColumn++;
            if (auto[ok, value] = TryGetColumnString(stmt, currentColumn); ok) payload.SetString7(value); currentColumn++;

            result.payload = std::move(payload);

            return true;
        }

        bool ParseOutput(sqlite3_stmt* stmt, ConstructEntry& result, int& currentColumn)
        {
            auto[ok0, number] = TryGetColumnInt64(stmt, currentColumn); currentColumn++;
            if (!ok0) {
                return false;
            }

            if (result.outputs.find(number) != result.outputs.end()) {
                // Skipping output's columns because they have been already parsed
                currentColumn+=4;
                return true;
            }

            auto output = std::make_shared<TransactionOutput>();
            output->SetNumber(number);

            if (auto[ok, value] = TryGetColumnString(stmt, currentColumn); ok) output->SetTxHash(value); currentColumn++;
            if (auto[ok, value] = TryGetColumnString(stmt, currentColumn); ok) output->SetAddressHash(value); currentColumn++;
            if (auto[ok, value] = TryGetColumnInt64(stmt, currentColumn); ok) output->SetValue(value); currentColumn++;
            if (auto[ok, value] = TryGetColumnString(stmt, currentColumn); ok) output->SetScriptPubKey(value); currentColumn++;

            result.outputs.insert({number, std::move(output)});

            return true;
        }
        
        bool ParseInput(sqlite3_stmt* stmt, ConstructEntry& result, int& currentColumn)
        {
            // TODO (team): Implement this
            return true;
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

    shared_ptr<PocketBlock> TransactionRepository::List(const vector<string>& txHashes, bool includePayload, bool includeInputs, bool includeOutputs)
    {
        // TODO (brangr): implement variable inputs
        auto sql = R"sql(
            SELECT
                t.Type,
                t.Hash,
                t.Time,
                t.Last,
                t.Id,
                t.String1,
                t.String2,
                t.String3,
                t.String4,
                t.String5,
                t.Int1
        )sql" +
        // Payload part
        (includePayload ? std::string(R"sql(,
                p.TxHash pHash,
                p.String1 pString1,
                p.String2 pString2,
                p.String3 pString3,
                p.String4 pString4,
                p.String5 pString5,
                p.String6 pString6,
                p.String7 pString7
        )sql") : "") +
        // 
        // Outputs part
        (includeOutputs ? std::string(R"sql(,
                o.Number,
                o.TxHash,
                o.AddressHash,
                o.Value,
                o.ScriptPubKey
        )sql") : "") +
        // 
        R"sql(
            FROM Transactions t
        )sql" +
        // Payload part
        (includePayload ? std::string(R"sql(
            LEFT JOIN Payload p on t.Hash = p.TxHash
        )sql") : "") +
        //
        // Outputs part
        (includeOutputs ? std::string(R"sql(
            CROSS JOIN TxOutputs o on t.Hash = o.TxHash
        )sql") : "") +
        //
        R"sql(
            WHERE t.Hash in ( )sql" + join(vector<string>(txHashes.size(), "?"), ",") + R"sql( )
        )sql";

        PocketBlockRef result;
        TryTransactionStep(__func__, [&]()
        {
            auto stmt = SetupSqlStatement(sql);

            for (size_t i = 0; i < txHashes.size(); i++)
                TryBindStatementText(stmt, (int) i + 1, txHashes[i]);

            TransactionReconstructor constructor(includePayload, includeOutputs, includeInputs);
            bool res = true;
            while (sqlite3_step(*stmt) == SQLITE_ROW)
            {
                if (!constructor.FeedRow(*stmt)) {
                    // TODO (losty): error here
                    res = false;
                    break;
                }
            }

            FinalizeSqlStatement(*stmt);

            if (res) {
                result = std::make_shared<PocketBlock>(std::move(constructor.GetResult()));
            }
        });

        return result;
    }

    shared_ptr<Transaction> TransactionRepository::Get(const string& hash, bool includePayload, bool includeInputs, bool includeOutputs)
    {
        auto lst = List({hash}, includePayload, includeInputs, includeOutputs);
        if (lst && !lst->empty())
            return lst->front();

        return nullptr;
    }

    shared_ptr<TransactionOutput> TransactionRepository::GetTxOutput(const string& txHash, int number)
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
            select count(*)
            from Transactions
            where Hash = ?
              and Height is not null
        )sql";

        TryTransactionStep(__func__, [&]()
        {
            auto stmt = SetupSqlStatement(sql);

            TryBindStatementText(stmt, 1, hash);

            if (sqlite3_step(*stmt) == SQLITE_ROW)
                if (auto[ok, value] = TryGetColumnInt(*stmt, 0); ok)
                    result = (value >= 1);

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

    void TransactionRepository::InsertTransactionOutputs(const PTransactionRef& ptx)
    {
        for (const auto& output: ptx->Outputs())
        {
            // Build transaction output
            auto stmt = SetupSqlStatement(R"sql(
                INSERT OR FAIL INTO TxOutputs (
                    TxHash,
                    Number,
                    AddressHash,
                    Value,
                    ScriptPubKey
                ) SELECT ?,?,?,?,?
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
            TryBindStatementText(stmt, 5, output->GetScriptPubKey());
            TryBindStatementText(stmt, 6, ptx->GetHash());
            TryBindStatementInt64(stmt, 7, output->GetNumber());
            TryBindStatementText(stmt, 8, output->GetAddressHash());

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

    // TODO (losty): below code it fully duplicated with some nuances in TransactionReconstructor::ProcessTransaction method
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

