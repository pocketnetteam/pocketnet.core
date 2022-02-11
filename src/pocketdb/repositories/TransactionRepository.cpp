// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include "pocketdb/repositories/TransactionRepository.h"

namespace PocketDb
{
    class TransactionReconstructor_0_21 : public RowAccessor
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

            auto [ok1, txEntry] = ProcessTransaction(stmt, txHash, currentColumn);
            if (!ok1 || txEntry == nullptr) {
                return false;
            }

            // The order of below functions is necessary. It should correspond to order of joins in query.
            if (m_includeOutputs && !ParseOutput(stmt, *txEntry, currentColumn)) {
                return false;
            }
            if (m_includeInputs && !ParseInput(stmt, *txEntry, currentColumn)) {
                return false;
            }
            
            return true;
        }

        /**
         * Constructs all collected data to PocketBlock format.
         * blockHashes is filled with block hashes corresponding to all collected transactions, where key is tx hash
         * and value is blockhash.
         * 
         */
        PocketBlock GetResult(map<string, string>& blockHashes)
        {
            PocketBlock result;
            map<string, string> blockHashesRes;
            for (auto& txEntryPair: m_constructed) {
                auto& txEntry = *txEntryPair.second;

                auto tx = txEntry.tx;
                if (txEntry.payload.has_value()) {
                    tx->SetPayload(txEntry.payload.value());
                }

                auto& outputs = tx->Outputs();
                outputs.clear();
                outputs.reserve(txEntry.outputs.size());
                for (const auto& outputPair: txEntry.outputs) {
                    outputs.emplace_back(outputPair.second);
                }

                result.emplace_back(tx);
                blockHashesRes.insert({*tx->GetHash(), txEntry.blockHash});
            }
            blockHashes = move(blockHashesRes);
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
            optional<Payload> payload;
            /**
             * Map for outputs where key is "Number" from TxOutputs table
             */
            map<int64_t, shared_ptr<TransactionOutput>> outputs;
            /**
             * Map for inputs
             */
            map<tuple<int64_t, int>, shared_ptr<TransactionInput>> outputs;
            /**
             * Block hash for current transaction
             */
            string blockHash;
        };
        /**
         * Map that is filled on every FeedRow(). TxHash is used as a key.
         */
        map<string, shared_ptr<ConstructEntry>> m_constructed;

        bool m_includePayload = false;
        bool m_includeOutputs = false;
        bool m_includeInputs = false;

        /**
         * This will bi filled by ProcessTransaction method to allow scip all already parsed single-row columns
         */
        int m_skipSingleRowColumnsOffset = 0;

        /**
         * This method parses transaction data, constructs if needed and return construct entry to fill
         */
        tuple<bool, shared_ptr<ConstructEntry>> ProcessTransaction(sqlite3_stmt* stmt, const string& txHash, int& currentColumn)
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

                // Creating construct entry that will be associated with current transaction and filled with payload, outputs, inputs, etc inside
                auto txEntry = make_shared<ConstructEntry>();

                if (auto[ok, value] = TryGetColumnInt(stmt, 3); ok) ptx->SetLast(value == 1);
                if (auto[ok, value] = TryGetColumnInt64(stmt, 4); ok) ptx->SetId(value);
                if (auto[ok, value] = TryGetColumnString(stmt, 5); ok) ptx->SetString1(value);
                if (auto[ok, value] = TryGetColumnString(stmt, 6); ok) ptx->SetString2(value);
                if (auto[ok, value] = TryGetColumnString(stmt, 7); ok) ptx->SetString3(value);
                if (auto[ok, value] = TryGetColumnString(stmt, 8); ok) ptx->SetString4(value);
                if (auto[ok, value] = TryGetColumnString(stmt, 9); ok) ptx->SetString5(value);
                if (auto[ok, value] = TryGetColumnInt64(stmt, 10); ok) ptx->SetInt1(value);
                if (auto[ok, value] = TryGetColumnString(stmt, 11); ok) txEntry->blockHash = value;

                txEntry->tx = move(ptx);
                m_constructed.insert({txHash, txEntry});

                currentColumn = 12;
                // Parsing single joins here results in some optimizing because we will not try to parse them later
                // only after one check if transaction has already parsed
                auto singleJoinsRes = ParseSingleJoins(stmt, *txEntry, currentColumn);
                m_skipSingleRowColumnsOffset = currentColumn;
                return {singleJoinsRes, txEntry};
            } else {
                currentColumn = m_skipSingleRowColumnsOffset;
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
            auto skip = [&]() { currentColumn += 8; return true; };
            if (result.payload.has_value()) {
                // Already have parse this payload, skip
                return skip();
            }

            bool ok0;
            string txHash;
            if (tie(ok0, txHash) = TryGetColumnString(stmt, currentColumn); !ok0) {
                // Assuming missing hash is missing payload that is a legal situation even if we request it.
                return skip();
            }

            auto payload = Payload();

            payload.SetTxHash(txHash); currentColumn++;
            if (auto[ok, value] = TryGetColumnString(stmt, currentColumn); ok) payload.SetString1(value); currentColumn++;
            if (auto[ok, value] = TryGetColumnString(stmt, currentColumn); ok) payload.SetString2(value); currentColumn++;
            if (auto[ok, value] = TryGetColumnString(stmt, currentColumn); ok) payload.SetString3(value); currentColumn++;
            if (auto[ok, value] = TryGetColumnString(stmt, currentColumn); ok) payload.SetString4(value); currentColumn++;
            if (auto[ok, value] = TryGetColumnString(stmt, currentColumn); ok) payload.SetString5(value); currentColumn++;
            if (auto[ok, value] = TryGetColumnString(stmt, currentColumn); ok) payload.SetString6(value); currentColumn++;
            if (auto[ok, value] = TryGetColumnString(stmt, currentColumn); ok) payload.SetString7(value); currentColumn++;

            result.payload = move(payload);

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

            auto output = make_shared<TransactionOutput>();
            output->SetNumber(number);

            if (auto[ok, value] = TryGetColumnString(stmt, currentColumn); ok) output->SetTxHash(value); currentColumn++;
            if (auto[ok, value] = TryGetColumnString(stmt, currentColumn); ok) output->SetAddressHash(value); currentColumn++;
            if (auto[ok, value] = TryGetColumnInt64(stmt, currentColumn); ok) output->SetValue(value); currentColumn++;
            if (auto[ok, value] = TryGetColumnString(stmt, currentColumn); ok) output->SetScriptPubKey(value); currentColumn++;

            result.outputs.insert({number, move(output)});

            return true;
        }
        
        bool ParseInput(sqlite3_stmt* stmt, ConstructEntry& result, int& currentColumn)
        {
            // TODO (team): Implement this
            return true;
        }
    };

    class TransactionReconstructor : public RowAccessor
    {
    public:
        TransactionReconstructor() = default;
        TransactionReconstructor() = delete;

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

            if (partType >= 1 && partType <= 3 && m_transactions.find(txHash) == m_transactions.end())
                return false;

            switch (partType)
            {
            case 0:
                string blockHash;
                auto[ok, ptx] = ParseTransaction(stmt, txHash, blockHash);
                if (!ok) return false;
                m_transactions.emplace(txHash, ptx);
                if (!blockHash.empty())
                    m_blockHashes.emplace(txHash, blockHash);
                break;
            case 1:
                if (!ParsePayload(stmt, m_transactions[txHash], txhash))
                    return false;
                break;
            case 2:
                if (!ParseInput(stmt, m_transactions[txHash], txhash))
                    return false;
                break;
            case 3:
                if (!ParseOutput(stmt, m_transactions[txHash], txhash))
                    return false;
                break;
            default:
                return false;
            }

            return true;
        }

        /**
         * Return contructed block.
         * blockHashes is filled with block hashes corresponding to all collected transactions, where key is tx hash and value is blockhash.
         */
        PocketBlockRef GetResult(map<string, string>& blockHashes = nullptr)
        {
            PocketBlockRef pocketBlock = make_shared<PocketBlock>();
            for (const auto[txHash, ptx] : m_transactions)
                pocketBlock->push_back(ptx);

            if (blockHashes)
                blockHashes = m_blockHashes;

            return pocketBlock;
        }

    private:

        map<string, PTransactionRef> m_transactions;
        map<string, string> m_blockHashes;

        /**
         * This method parses transaction data, constructs if needed and return construct entry to fill
         * Index:   0  1     2     3     4          5     6   7        8        9        10       11       12    13    14
         * Columns: 0, Hash, Type, Time, BlockHash, Last, Id, String1, String2, String3, String4, String5, null, null, Int1
         */
        tuple<bool, PTransactionRef> ParseTransaction(sqlite3_stmt* stmt, const string& txHash, string& blockHash)
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
            if (auto[ok, value] = TryGetColumnString(stmt, 4); ok) blockHash = value;
            if (auto[ok, value] = TryGetColumnInt(stmt, 5); ok) ptx->SetLast(value == 1);
            if (auto[ok, value] = TryGetColumnInt64(stmt, 6); ok) ptx->SetId(value);
            if (auto[ok, value] = TryGetColumnString(stmt, 7); ok) ptx->SetString1(value);
            if (auto[ok, value] = TryGetColumnString(stmt, 8); ok) ptx->SetString2(value);
            if (auto[ok, value] = TryGetColumnString(stmt, 9); ok) ptx->SetString3(value);
            if (auto[ok, value] = TryGetColumnString(stmt, 10); ok) ptx->SetString4(value);
            if (auto[ok, value] = TryGetColumnString(stmt, 11); ok) ptx->SetString5(value);
            if (auto[ok, value] = TryGetColumnInt64(stmt, 14); ok) ptx->SetInt1(value);

            return {true, ptx};
        }

        /**
         * Parse Payload
         * Index:   0  1       2     3     4     5     6     7        8        9        10       11       12       13       14
         * Columns: 1, TxHash, null, null, null, null, null, String1, String2, String3, String4, String5, String6, String7, Int1
         */
        bool ParsePayload(sqlite3_stmt* stmt, PTransactionRef& ptx, const string& txHash)
        {
            bool empty = true;
            Payload payload;
            payload.SetTxHash(txHash);

            if (auto[ok, value] = TryGetColumnString(stmt, 7); ok) { payload.SetString1(value); empty = false; }
            if (auto[ok, value] = TryGetColumnString(stmt, 8); ok) { payload.SetString2(value); empty = false; }
            if (auto[ok, value] = TryGetColumnString(stmt, 9); ok) { payload.SetString3(value); empty = false; }
            if (auto[ok, value] = TryGetColumnString(stmt, 10); ok) { payload.SetString4(value); empty = false; }
            if (auto[ok, value] = TryGetColumnString(stmt, 11); ok) { payload.SetString5(value); empty = false; }
            if (auto[ok, value] = TryGetColumnString(stmt, 12); ok) { payload.SetString6(value); empty = false; }
            if (auto[ok, value] = TryGetColumnString(stmt, 13); ok) { payload.SetString7(value); empty = false; }
            if (auto[ok, value] = TryGetColumnInt64(stmt, 14); ok) { payload.SetInt1(value); empty = false; }

            ptx->SetPayload(payload);
            return !empty;
        }

        /**
         * Parse Inputs
         * Index:   0  1            2     3     4       5       6     7     8     9     10    11    12    13    14
         * Columns: 2, SpentTxHash, null, null, TxHash, Number, null, null, null, null, null, null, null, null, null
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

            ptx->Inputs().push_back(input);
            return !incomplete;
        }

        /**
         * Parse Outputs
         * Index:   0  1       2     3       4            5      6     7     8     9             10    11    12    13    14
         * Columns: 3, TxHash, null, Number, AddressHash, Value, null, null, null, ScriptPubKey, null, null, null, null, null
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

            if (auto[ok, value] = TryGetColumnString(stmt, 9); ok) output->SetScriptPubKey(value);
            else incomplete = true;

            ptx->Outputs().push_back(output);
            return !incomplete;
        }
    }

    void TransactionRepository::InsertTransactions(PocketBlock& pocketBlock)
    {
        TryTransactionStep(__func__, [&]()
        {
            for (const auto& ptx: pocketBlock)
            {
                // Insert general transaction
                InsertTransactionModel(ptx);

                // Inputs
                InsertTransactionOutputs(ptx);

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
        return List(txHashes, nullptr, includePayload, includeInputs, includeOutputs);
    }

    PocketBlockRef TransactionRepository::List(const vector<string>& txHashes, map<string, string>& blockHashes, bool includePayload, bool includeInputs, bool includeOutputs)
    {
        string txReplacers = join(vector<string>(txHashes.size(), "?"), ",");
        auto sql = R"sql(
            select 0, Hash, Type, Time, BlockHash, Last, Id, String1, String2, String3, String4, String5, null, null, Int1
            from Transactions
            where Hash in ( )sql" + txReplacers + R"sql( )
        )sql" +

        // Payload part
        (includePayload ? string(R"sql(,
            union
            select 1, TxHash, null, null, null, null, null, String1, String2, String3, String4, String5, String6, String7, Int1
            from Payload
            where TxHash in ( )sql" + txReplacers + R"sql( )
        )sql") : "") +

        // Inputs part
        (includeInputs ? string(R"sql(,
            union
            select 2, SpentTxHash, null, null, TxHash, Number, null, null, null, null, null, null, null, null, null
            from TxInputs
            where SpentTxHash in ( )sql" + txReplacers + R"sql( )
        )sql") : "") +
        
        // Outputs part
        (includeOutputs ? string(R"sql(,
            union
            select 3, TxHash, null, Number, AddressHash, Value, null, null, null, ScriptPubKey, null, null, null, null, null
            from TxOutputs
            where TxHash in ( )sql" + txReplacers + R"sql( )
        )sql") : "") +
        
        TransactionReconstructor constructor();

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
                if (!constructor.FeedRow(*stmt))
                    break;
            }

            FinalizeSqlStatement(*stmt);
        });

        return constructor.GetResult(blockHashes);
    }

    PTransactionRef TransactionRepository::Get(const string& hash, string& blockHash, bool includePayload, bool includeInputs, bool includeOutputs)
    {
        map<string, string> blockHashes;
        auto lst = List({ hash }, blockHashes, includePayload, includeInputs, includeOutputs);
        if (lst && !lst->empty())
        {
            if (blockHash != nullptr)
            {
                auto blockHashItr = blockHashes.find(hash);
                if (blockHashItr != blockHashes.end())
                    blockHash = blockHashItr->second;
            }

            return lst->front();
        }

        return nullptr;
    }

    PTransactionRef TransactionRepository::Get(const string& hash, bool includePayload, bool includeInputs, bool includeOutputs)
    {
        return Get(hash, nullptr, includePayload, includeInputs, includeOutputs);
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

