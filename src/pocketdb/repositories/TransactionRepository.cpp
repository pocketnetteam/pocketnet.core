// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include "pocketdb/repositories/TransactionRepository.h"

namespace PocketDb
{
    class CollectDataToModelConverter
    {
    public:
        // TODO (optimization): consider remove `repository` parameter from here.
        static optional<CollectData> ModelToCollectData(const PTransactionRef& ptx)
        {
            if (!ptx->GetHash()) return nullopt;
            TxContextualData txData;
            optional<CollectData> res;
            if (!DbViewHelper::Extract(txData, ptx)) {
                return nullopt;
            }

            CollectData collectData(*ptx->GetHash());
            collectData.txContextData = std::move(txData);
            collectData.inputs = ptx->Inputs();
            collectData.outputs = ptx->OutputsConst();
            collectData.payload = ptx->GetPayload();
            collectData.ptx = ptx;

            return collectData;
        }

        static PTransactionRef CollectDataToModel(const CollectData& collectData)
        {
            auto ptx = collectData.ptx;

            if (!DbViewHelper::Inject(ptx, collectData.txContextData))
                return nullptr;

            auto outputs = collectData.outputs;
            std::sort(outputs.begin(), outputs.end(), [](const TransactionOutput& elem1, const TransactionOutput& elem2) { return *elem1.GetNumber() < *elem2.GetNumber(); });
            auto inputs = collectData.inputs;
            std::sort(inputs.begin(), inputs.end(), [](const TransactionInput& elem1, const TransactionInput& elem2) { return *elem1.GetNumber() < *elem2.GetNumber(); });
            ptx->Inputs() = std::move(inputs);
            ptx->Outputs() = std::move(outputs);
            if (collectData.payload) ptx->SetPayload(*collectData.payload);
            return ptx;
        }
    };

    class TransactionReconstructor
    {
    public:
        TransactionReconstructor(std::map<std::string, CollectData> initData) {
            m_collectData = std::move(initData);
        };

        /**
         * Pass a new row for reconstructor to collect all necessary data from it.
         * @param stmt - sqlite stmt. Null is not allowed and will potentially cause a segfault
         * Returns boolean result of collecting data. If false is returned - all data inside reconstructor is possibly corrupted due to bad input (missing columns, etc)
         * and it should not be used anymore
         * 
         * Columns: PartType (0), TxHash (1), ...
         * PartTypes: 0 Tx, 1 Payload, 2 Input, 3 Output
         */
        bool FeedRow(Stmt& stmt)
        {
            auto[okPartType, partType] = stmt.TryGetColumnInt(0);
            if (!okPartType) return false;

            auto[okTxHash, txHash] = stmt.TryGetColumnString(1);
            if (!okTxHash) return false;

            auto dataItr = m_collectData.find(txHash);
            if (dataItr == m_collectData.end()) return false;

            auto& data = dataItr->second;

            switch (partType)
            {
                case 0: {
                    return ParseTransaction(stmt, data);
                }
                case 1:
                    return ParsePayload(stmt, data);
                case 2:
                    return ParseInput(stmt, data);
                case 3:
                    return ParseOutput(stmt, data);
                default:
                    return false;
            }
        }

        /**
         * Return contructed block.
         */
        std::vector<CollectData> GetResult()
        {
            std::vector<CollectData> res;

            transform(
                m_collectData.begin(),
                m_collectData.end(),
                back_inserter(res),
                [](const auto& elem) {return elem.second;});

            return res;
        }

    private:

        map<std::string, CollectData> m_collectData;

        /**
         * This method parses transaction data, constructs if needed and return construct entry to fill
         * Index:   0  1     2     3     4          5       6     7   8        9        10       11       12       13    14    15
         * Columns: 0, Hash, Type, Time, BlockHash, Height, Last, Id, String1, String2, String3, String4, String5, null, null, Int1
         */
        bool ParseTransaction(Stmt& stmt, CollectData& collectData)
        {
            // Try get Type and create pocket transaction instance
            auto[okType, txType] = stmt.TryGetColumnInt(2);
            if (!okType) return false;
            
            PTransactionRef ptx = PocketHelpers::TransactionHelper::CreateInstance(static_cast<TxType>(txType));
            if (!ptx) return false;

            PocketHelpers::TxContextualData txContextData;

            if (auto[ok, value] = stmt.TryGetColumnInt64(3); ok) ptx->SetTime(value);
            if (auto[ok, value] = stmt.TryGetColumnInt64(4); ok) ptx->SetHeight(value);
            // TODO (optimization): implement "first" field
            // if (auto[ok, value] = stmt.TryGetColumnInt64(5); ok) ptx->SetFirst(value);
            if (auto[ok, value] = stmt.TryGetColumnInt64(5); ok) ptx->SetLast(value);
            if (auto[ok, value] = stmt.TryGetColumnInt64(6); ok) ptx->SetId(value);
            if (auto[ok, value] = stmt.TryGetColumnInt64(7); ok) txContextData.int1 = value;

            if (auto[ok, value] = stmt.TryGetColumnString(8); ok) txContextData.string1 = value;
            if (auto[ok, value] = stmt.TryGetColumnString(9); ok) txContextData.string2 = value;
            if (auto[ok, value] = stmt.TryGetColumnString(10); ok) txContextData.string3 = value;
            if (auto[ok, value] = stmt.TryGetColumnString(11); ok) txContextData.string4 = value;
            if (auto[ok, value] = stmt.TryGetColumnString(12); ok) txContextData.string5 = value;

            if (auto[ok, value] = stmt.TryGetColumnString(13); ok) ptx->SetBlockHash(value);

            if (auto[ok, value] = stmt.TryGetColumnString(14); ok) txContextData.list = value;

            ptx->SetHash(collectData.txHash);
            collectData.ptx = std::move(ptx);            
            collectData.txContextData = std::move(txContextData);

            return true;
        }

        /**
         * Parse Payload
         * Index:   0  1       2     3     4     5     6     7     8        9        10       11       12       13       14       15
         * Columns: 1, TxHash, null, null, null, null, null, null, String1, String2, String3, String4, String5, String6, String7, Int1
         */
        bool ParsePayload(Stmt& stmt, CollectData& collectData)
        {
            Payload payload;
            if (auto[ok, value] = stmt.TryGetColumnInt64(2); ok) { payload.SetInt1(value); }
            if (auto[ok, value] = stmt.TryGetColumnString(8); ok) { payload.SetString1(value); }
            if (auto[ok, value] = stmt.TryGetColumnString(9); ok) { payload.SetString2(value); }
            if (auto[ok, value] = stmt.TryGetColumnString(10); ok) { payload.SetString3(value); }
            if (auto[ok, value] = stmt.TryGetColumnString(11); ok) { payload.SetString4(value); }
            if (auto[ok, value] = stmt.TryGetColumnString(12); ok) { payload.SetString5(value); }
            if (auto[ok, value] = stmt.TryGetColumnString(13); ok) { payload.SetString6(value); }
            if (auto[ok, value] = stmt.TryGetColumnString(14); ok) { payload.SetString7(value); }

            collectData.payload = std::move(payload);

            return true;
        }

        /**
         * Parse Inputs
         * Index:   0  1              2     3     4         5         6        7     8              9     10    11    12    13    14    15
         * Columns: 2, i.SpentTxHash, null, null, i.TxHash, i.Number, o.Value, null, o.AddressHash, null, null, null, null, null, null, null
         */
        bool ParseInput(Stmt& stmt, CollectData& collectData)
        {
            bool incomplete = false;

            TransactionInput input;
            input.SetSpentTxHash(collectData.txHash);
            if (auto[ok, value] = stmt.TryGetColumnInt64(2); ok) input.SetNumber(value);
            else incomplete = true;

            if (auto[ok, value] = stmt.TryGetColumnInt64(3); ok) input.SetValue(value);

            if (auto[ok, value] = stmt.TryGetColumnString(8); ok) input.SetTxHash(value);
            else incomplete = true;

            if (auto[ok, value] = stmt.TryGetColumnString(9); ok) input.SetAddressHash(value);

            collectData.inputs.emplace_back(input);
            return !incomplete;
        }

        /**
         * Parse Outputs
         * Index:   0  1       2     3       4            5      6     7     8     9     10            11    12    13    14           15
         * Columns: 3, TxHash, null, Number, AddressHash, Value, null, null, null, null, ScriptPubKey, null, null, null, SpentTxHash, SpentHeight
         */
        bool ParseOutput(Stmt& stmt, CollectData& collectData)
        {
            bool incomplete = false;

            TransactionOutput output;
            output.SetTxHash(collectData.txHash);

            if (auto[ok, value] = stmt.TryGetColumnInt64(2); ok) output.SetValue(value);
            else incomplete = true;

            if (auto[ok, value] = stmt.TryGetColumnInt64(3); ok) output.SetNumber(value);
            else incomplete = true;

            if (auto[ok, value] = stmt.TryGetColumnString(8); ok) output.SetAddressHash(value);
            else incomplete = true;

            if (auto[ok, value] = stmt.TryGetColumnString(9); ok) output.SetScriptPubKey(value);
            else incomplete = true;

            collectData.outputs.emplace_back(output);
            return !incomplete;
        }
    };

    static auto _findStringsAndListsToBeInserted(const std::vector<CollectData>& collectDataVec)
    {
        vector<string> stringsToBeInserted;
        vector<string> listsToBeInserted;
        for (const auto& collectData: collectDataVec)
        {
            const auto& txData = collectData.txContextData;
            if (txData.string1) stringsToBeInserted.emplace_back(*txData.string1);
            if (txData.string2) stringsToBeInserted.emplace_back(*txData.string2);
            if (txData.string3) stringsToBeInserted.emplace_back(*txData.string3);
            if (txData.string4) stringsToBeInserted.emplace_back(*txData.string4);
            if (txData.string5) stringsToBeInserted.emplace_back(*txData.string5);
            stringsToBeInserted.emplace_back(collectData.txHash);
            if (txData.list) listsToBeInserted.emplace_back(*txData.list);
            
            for (const auto& output: collectData.outputs) {
                if (output.GetAddressHash()) stringsToBeInserted.emplace_back(*output.GetAddressHash());
                if (output.GetScriptPubKey()) stringsToBeInserted.emplace_back(*output.GetScriptPubKey());
            }
        }

        return std::pair { stringsToBeInserted, listsToBeInserted };
    }

    void TransactionRepository::InsertTransactions(PocketBlock& pocketBlock)
    {
        std::vector<CollectData> collectDataVec;
        for (const auto& ptx: pocketBlock)
        {
            auto collectData = CollectDataToModelConverter::ModelToCollectData(ptx);
            if (!collectData) {
                // TODO (optimization): error
                LogPrintf("DEBUG: failed to convert model to CollecData\n");
                continue;
            }
            collectDataVec.emplace_back(std::move(*collectData));
        }

        vector<string> registyStrings;
        vector<string> lists;
        tie(registyStrings, lists) = _findStringsAndListsToBeInserted(collectDataVec);

        TryTransactionStep(__func__, [&]()
        {
            InsertRegistry(registyStrings);
            InsertRegistryLists(lists);
            for (const auto& collectData: collectDataVec)
            {
                // Filling Registry with text data
                // TODO (lostystyg): implement insert into registry

                // Insert general transaction
                InsertTransactionModel(collectData);

                // Insert lists for transaction
                if (collectData.txContextData.list) InsertList(*collectData.txContextData.list, collectData.txHash);

                // Inputs
                InsertTransactionInputs(collectData.inputs, collectData.txHash);

                // Outputs
                InsertTransactionOutputs(collectData.outputs, collectData.txHash);

                // Also need insert payload of transaction
                // But need get new rowId
                // If last id equal 0 - insert ignored - or already exists or error -> paylod not inserted
                if (collectData.payload)
                    InsertTransactionPayload(*collectData.payload);
            }
        });
    }

    PocketBlockRef TransactionRepository::List(const vector<string>& txHashes, bool includePayload, bool includeInputs, bool includeOutputs)
    {
        string txReplacers = join(vector<string>(txHashes.size(), "?"), ",");

        auto sql = R"sql(
            with
                txhash as (
                    select
                        t.RowId,
                        t.String
                    from
                        vTxRowId t
                    where
                        t.String in ( )sql" + txReplacers + R"sql( )
                )
            select
                (0)tp,
                txhash.String,
                t.Type,
                t.Time,
                c.Height,
                ifnull((
                    select 1
                    from Last l -- primary key
                    where l.TxId = t.RowId
                ), 0),
                c.Uid,
                t.Int1,
                (
                    select r.String
                    from Registry r
                    where r.RowId = t.RegId1
                ),
                (
                    select r.String
                    from Registry r
                    where r.RowId = t.RegId2
                ),
                (
                    select r.String
                    from Registry r
                    where r.RowId = t.RegId3
                ),
                (
                    select r.String
                    from Registry r
                    where r.RowId = t.RegId4
                ),
                (
                    select r.String
                    from Registry r
                    where r.RowId = t.RegId5
                ),
                (
                    select r.String
                    from Registry r
                    where r.RowId = c.BlockId
                ),
                (
                    select
                        json_group_array(
                            r.String
                        )
                    from
                        Lists l indexed by Lists_TxId_OrderIndex_RegId
                        cross join Registry r -- primary key
                            on r.RowId = l.RegId
                    where
                        l.TxId = t.RowId
                    order by l.OrderIndex asc
                )
            from
                txhash
                cross join Transactions t -- primary key
                    on t.RowId = txhash.RowId
                left join Chain c -- primary key
                    on c.TxId = t.RowId
        )sql" +

        // Payload part
        (includePayload ? string(R"sql(
            union
            select
                (1) tp,
                txhash.String,
                Int1,
                null,
                null,
                null,
                null,
                null,
                String1,
                String2,
                String3,
                String4,
                String5,
                String6,
                String7
            from
                txhash
                cross join Payload p -- primary key
                    on p.TxId = txhash.RowId
        )sql") : "") +

        // Inputs part
        (includeInputs ? string(R"sql(
            union
            select
                (2)tp,
                txhash.String,
                i.Number,
                o.Value,
                null,
                null,
                null,
                null,
                (
                    select r.String
                    from Registry r
                    where r.RowId = i.TxId
                ),
                (
                    select a.String
                    from Registry a
                    where a.RowId = o.AddressId
                ),
                null,
                null,
                null,
                null,
                null
            from
                txhash
                cross join TxInputs i indexed by TxInputs_SpentTxId_TxId_Number
                    on i.SpentTxId = txhash.RowId
                cross join TxOutputs o indexed by TxOutputs_TxId_Number_AddressId
                    on o.TxId = i.TxId and o.Number = i.Number
        )sql") : "") +
        
        // Outputs part
        (includeOutputs ? string(R"sql(
            union
            select
                (3)tp,
                txhash.String,
                o.Value,
                o.Number,
                null,
                null,
                null,
                null,
                (
                    select r.String
                    from Registry r
                    where r.RowId = o.AddressId
                ),
                (
                    select r.String
                    from Registry r
                    where r.RowId = o.ScriptPubKeyId
                ),
                null,
                null,
                null,
                null,
                null
            from
                txhash
                cross join TxOutputs o
                    on o.TxId = txhash.RowId
        )sql") : "") +

        string(R"sql(
            order by tp asc
        )sql");

        std::map<std::string, CollectData> initData;
        for (const auto& hash: txHashes) {
            initData.emplace(hash, CollectData{hash});
        }

        TransactionReconstructor reconstructor(initData);

        TryTransactionStep(__func__, [&]()
        {
            auto stmt = SetupSqlStatement(sql);

            stmt->Bind(txHashes);

            while (stmt->Step() == SQLITE_ROW)
            {
                if (!reconstructor.FeedRow(*stmt))
                    throw runtime_error("Transaction::List feedRow failed - no return data");
            }
        });

        auto pBlock = std::make_shared<PocketBlock>();

        for (auto& collectData: reconstructor.GetResult())
        {
            if (auto ptx = CollectDataToModelConverter::CollectDataToModel(collectData); ptx)
                pBlock->emplace_back(ptx);
            else
                throw runtime_error("Transaction::List reconstruct failed - no return data");
        }

        return pBlock;
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
        auto txIdMap = GetTxIds({txHash});
        auto txIdItr = txIdMap.find(txHash);

        if (txIdItr == txIdMap.end()) {
            return nullptr;
        }

        auto& txId = txIdItr->second;

        auto sql = R"sql(
            SELECT
                Number,
                (select String from Registry where RowId = AddressId),
                Value,
                (select String from Registry where RowId = ScriptPubKeyId)
            FROM TxOutputs
            WHERE TxId = ?
              and Number = ?
        )sql";

        shared_ptr<TransactionOutput> result = nullptr;
        TryTransactionStep(__func__, [&]()
        {
            static auto stmt = SetupSqlStatement(sql);

            stmt->Bind(txId, number);

            if (stmt->Step() == SQLITE_ROW)
            {
                result = make_shared<TransactionOutput>();
                result->SetTxHash(txHash);
                if (auto[ok, value] = stmt->TryGetColumnInt64(0); ok) result->SetNumber(value);
                if (auto[ok, value] = stmt->TryGetColumnString(1); ok) result->SetAddressHash(value);
                if (auto[ok, value] = stmt->TryGetColumnInt64(2); ok) result->SetValue(value);
                if (auto[ok, value] = stmt->TryGetColumnString(3); ok) result->SetScriptPubKey(value);
            }
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
            static auto stmt = SetupSqlStatement(sql);

            stmt->Bind(hash);

            result = (stmt->Step() == SQLITE_ROW);

            stmt->Reset();
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
            static auto stmt = SetupSqlStatement(sql);

            stmt->Bind(hash);

            if (stmt->Step() == SQLITE_ROW)
                result = true;

            stmt->Reset();
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
            static auto stmt = SetupSqlStatement(sql);

            if (stmt->Step() == SQLITE_ROW)
                if (auto[ok, value] = stmt->TryGetColumnInt(0); ok)
                    result = value;
            stmt->Reset();
        });

        return result;
    }

    void TransactionRepository::Clean()
    {
        TryTransactionStep(__func__, [&]()
        {
            static auto stmt = SetupSqlStatement(R"sql(
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
            // Clear Payload tablew
            static auto stmt1 = SetupSqlStatement(R"sql(
                delete from Payload
                where TxHash = ?
                  and exists(
                    select 1
                    from Transactions t
                    where t.Hash = Payload.TxHash
                      and t.Height isnull
                  )
            )sql");
            stmt1->Bind(hash);
            TryStepStatement(stmt1);

            // Clear TxOutputs table
            static auto stmt2 = SetupSqlStatement(R"sql(
                delete from TxOutputs
                where TxHash = ?
                  and exists(
                    select 1
                    from Transactions t
                    where t.Hash = TxOutputs.TxHash
                      and t.Height isnull
                  )
            )sql");
            stmt2->Bind(hash);
            TryStepStatement(stmt2);

            // Clear Transactions table
            static auto stmt3 = SetupSqlStatement(R"sql(
                delete from Transactions
                where Hash = ?
                  and Height isnull
            )sql");
            stmt3->Bind(hash);
            TryStepStatement(stmt3);
        });
    }

    void TransactionRepository::CleanMempool()
    {
        TryTransactionStep(__func__, [&]()
        {
            // Clear Payload table
            static auto stmt1 = SetupSqlStatement(R"sql(
                delete from Payload
                where TxHash in (
                  select t.Hash
                  from Transactions t
                  where t.Height is null
                )
            )sql");
            TryStepStatement(stmt1);

            // Clear TxOutputs table
            static auto stmt2 = SetupSqlStatement(R"sql(
                delete from TxOutputs
                where TxHash in (
                  select t.Hash
                  from Transactions t
                  where t.Height is null
                )
            )sql");
            TryStepStatement(stmt2);

            // Clear Transactions table
            static auto stmt3 = SetupSqlStatement(R"sql(
                delete from Transactions
                where Height isnull
            )sql");
            TryStepStatement(stmt3);
        });
    }

    void TransactionRepository::InsertTransactionInputs(const vector<TransactionInput>& inputs, const string& txHash)
    {
        static auto stmt = SetupSqlStatement(R"sql(
            with
                data as (
                    select
                        (select RowId from vTxRowId where String = ?) as spentTx,
                        (select RowId from vTxRowId where String = ?) as tx,
                        ? as number
                )

            insert or fail into TxInputs
            (
                SpentTxId,
                TxId,
                Number
            )
            select
                data.spentTx,
                data.tx,
                data.number
            from
                data
            where
                not exists (
                    select 1
                    from TxInputs i
                    where
                        i.SpentTxId = data.spentTx and
                        i.TxId = data.tx and
                        i.Number = data.number
                )
        )sql");

        for (const auto& input: inputs)
        {
            stmt->Bind(
                input.GetSpentTxHash(),
                txHash,
                input.GetNumber()
            );
            TryStepStatement(stmt);
        }
    }
    
    void TransactionRepository::InsertTransactionOutputs(const vector<TransactionOutput>& outputs, const string& txHash)
    {
        static auto stmt = SetupSqlStatement(R"sql(
                with
                    tx as (
                        select
                            RowId
                        from
                            vTxRowId
                        where
                            String = ?
                    )
                insert or fail into
                    TxOutputs (
                        TxId,
                        Number,
                        AddressId,
                        Value,
                        ScriptPubKeyId
                    )
                select
                    tx.RowId,
                    ?,
                    (
                        select RowId
                        from Registry
                        where String = ?
                    ),
                    ?,
                    (
                        select RowId
                        from Registry
                        where String = ?
                    )
                from tx
                where
                    not exists(
                        select
                        1
                    from
                        TxOutputs indexed by TxOutputs_TxId_Number_AddressId
                    where
                        TxId = tx.RowId and
                        Number = ?           
                    )
            )sql");

        for (const auto& output: outputs)
        {
            stmt->Bind(
                txHash,
                output.GetNumber(),
                output.GetAddressHash(),
                output.GetValue(),
                output.GetScriptPubKey(),
                output.GetNumber()
            );
            TryStepStatement(stmt);
        }
    }

    void TransactionRepository::InsertTransactionPayload(const Payload& payload)
    {
        static auto stmt = SetupSqlStatement(R"sql(
            with
                tx as (
                    select
                        RowId
                    from
                        vTxRowId
                    where
                        String = ?
                )

            insert or fail into
                Payload (
                    TxId,
                    String1,
                    String2,
                    String3,
                    String4,
                    String5,
                    String6,
                    String7,
                    Int1
                )
            select
                tx.RowId,
                ?,
                ?,
                ?,
                ?,
                ?,
                ?,
                ?,
                ?
            from tx
            where
                not exists(
                    select
                        1
                    from
                        Payload p
                    where
                        p.TxId = tx.RowId
                )
        )sql");

        stmt->Bind(
            payload.GetTxHash(),
            payload.GetString1(),
            payload.GetString2(),
            payload.GetString3(),
            payload.GetString4(),
            payload.GetString5(),
            payload.GetString6(),
            payload.GetString7(),
            payload.GetInt1()
        );

        TryStepStatement(stmt);
    }

    void TransactionRepository::InsertTransactionModel(const CollectData& collectData)
    {
        // TODO (optimization): WITH not working?
        static auto stmt = SetupSqlStatement(R"sql(
            with h as (
                select RowId as HashId from Registry where String = ?
            )
            INSERT OR FAIL INTO Transactions (
                Type,
                Time,
                Int1,
                HashId,
                RegId1,
                RegId2,
                RegId3,
                RegId4,
                RegId5
            )
            SELECT ?,?,?, (select h.HashId from h),
                (select RowId from Registry where String = ?),
                (select RowId from Registry where String = ?),
                (select RowId from Registry where String = ?),
                (select RowId from Registry where String = ?),
                (select RowId from Registry where String = ?)
            WHERE not exists (select 1 from Transactions a,h where a.HashId = h.HashId)
        )sql");

        stmt->Bind(
            collectData.txHash,
            (int)*collectData.ptx->GetType(),
            collectData.ptx->GetTime(),
            collectData.txContextData.int1,
            collectData.txContextData.string1,
            collectData.txContextData.string2,
            collectData.txContextData.string3,
            collectData.txContextData.string4,
            collectData.txContextData.string5
        );

        TryStepStatement(stmt);

        if (!collectData.txContextData.list) return;
    }

    tuple<bool, PTransactionRef> TransactionRepository::CreateTransactionFromListRow(
        Stmt& stmt, bool includedPayload)
    {
        // TODO (aok): move deserialization logic to models ?

        auto[ok0, _txType] = stmt.TryGetColumnInt(0);
        auto[ok1, txHash] = stmt.TryGetColumnString(1);
        auto[ok2, nTime] = stmt.TryGetColumnInt64(2);

        if (!ok0 || !ok1 || !ok2)
            return make_tuple(false, nullptr);

        auto txType = static_cast<TxType>(_txType);
        auto ptx = PocketHelpers::TransactionHelper::CreateInstance(txType);
        ptx->SetTime(nTime);
        ptx->SetHash(txHash);

        if (ptx == nullptr)
            return make_tuple(false, nullptr);

        if (auto[ok, value] = stmt.TryGetColumnInt(3); ok) ptx->SetLast(value == 1);
        if (auto[ok, value] = stmt.TryGetColumnInt64(4); ok) ptx->SetId(value);
        if (auto[ok, value] = stmt.TryGetColumnString(5); ok) ptx->SetString1(value);
        if (auto[ok, value] = stmt.TryGetColumnString(6); ok) ptx->SetString2(value);
        if (auto[ok, value] = stmt.TryGetColumnString(7); ok) ptx->SetString3(value);
        if (auto[ok, value] = stmt.TryGetColumnString(8); ok) ptx->SetString4(value);
        if (auto[ok, value] = stmt.TryGetColumnString(9); ok) ptx->SetString5(value);
        if (auto[ok, value] = stmt.TryGetColumnInt64(10); ok) ptx->SetInt1(value);

        if (!includedPayload)
            return make_tuple(true, ptx);

        if (auto[ok, value] = stmt.TryGetColumnString(11); !ok)
            return make_tuple(true, ptx);

        auto payload = Payload();
        if (auto[ok, value] = stmt.TryGetColumnString(11); ok) payload.SetTxHash(value);
        if (auto[ok, value] = stmt.TryGetColumnString(12); ok) payload.SetString1(value);
        if (auto[ok, value] = stmt.TryGetColumnString(13); ok) payload.SetString2(value);
        if (auto[ok, value] = stmt.TryGetColumnString(14); ok) payload.SetString3(value);
        if (auto[ok, value] = stmt.TryGetColumnString(15); ok) payload.SetString4(value);
        if (auto[ok, value] = stmt.TryGetColumnString(16); ok) payload.SetString5(value);
        if (auto[ok, value] = stmt.TryGetColumnString(17); ok) payload.SetString6(value);
        if (auto[ok, value] = stmt.TryGetColumnString(18); ok) payload.SetString7(value);

        ptx->SetPayload(payload);

        return make_tuple(true, ptx);
    }

    map<string,int64_t> TransactionRepository::GetTxIds(const vector<string>& txHashes)
    {
        string txReplacers = join(vector<string>(txHashes.size(), "?"), ",");

        auto sql = R"sql(
            select r.String, t.RowId
            from Transactions t
            join Registry r
                on t.HashId = r.RowId
                and r.String in  ( )sql" + txReplacers + R"sql( )
        )sql";

        map<string,int64_t> res;
        TryTransactionStep(__func__, [&]()
        {
            auto stmt = SetupSqlStatement(sql);
            stmt->Bind(txHashes);

            while (stmt->Step() == SQLITE_ROW) {
                // TODO (optimization): error
                auto[ok1, hash] = stmt->TryGetColumnString(0);
                auto[ok2, txId] = stmt->TryGetColumnInt64(1);
                if (ok1 && ok2) res.emplace(hash, txId);
            }
            stmt->Reset();
        });

        return res;
    }

    optional<string> TransactionRepository::TxIdToHash(const int64_t& id)
    {
        auto sql = R"sql(
            select Hash
            from Transactions
            where Id = ?
        )sql";

        optional<string> hash;
        TryTransactionStep(__func__, [&]()
        {
            static auto stmt = SetupSqlStatement(sql);
            stmt->Bind(id);
            if (stmt->Step() == SQLITE_ROW)
                if (auto [ok, val] = stmt->TryGetColumnString(0); ok)
                    hash = val;
            stmt->Reset();
        });

        return hash;
    }

    optional<int64_t> TransactionRepository::TxHashToId(const string& hash)
    {
        auto sql = R"sql(
            select t.Id
            from Transactions t
            join Registry r
                on r.String = ?
                and r.Id = t.HashId
        )sql";

        optional<int64_t> id;
        TryTransactionStep(__func__, [&]()
        {
            static auto stmt = SetupSqlStatement(sql);
            stmt->Bind(hash);
            if (stmt->Step() == SQLITE_ROW)
                if (auto [ok, val] = stmt->TryGetColumnInt64(0); ok)
                    id = val;
            stmt->Reset();
        });

        return id;
    }

    optional<string> TransactionRepository::AddressIdToHash(const int64_t& id)
    {
        auto sql = R"sql(
            select Hash
            from Addresses
            where Id = ?
        )sql";

        optional<string> hash;
        TryTransactionStep(__func__, [&]()
        {
            static auto stmt = SetupSqlStatement(sql);
            stmt->Bind(id);
            if (stmt->Step() == SQLITE_ROW)
                if (auto [ok, val] = stmt->TryGetColumnString(0); ok)
                    hash = val;
            stmt->Reset();
        });

        return hash;
    }

    optional<int64_t> TransactionRepository::AddressHashToId(const string& hash)
    {
        auto sql = R"sql(
            select Id
            from Addresses
            where Hash = ?
        )sql";

        optional<int64_t> id;
        TryTransactionStep(__func__, [&]()
        {
            static auto stmt = SetupSqlStatement(sql);
            stmt->Bind(hash);
            if (stmt->Step() == SQLITE_ROW)
                if (auto [ok, val] = stmt->TryGetColumnInt64(0); ok)
                    id = val;
            stmt->Reset();
        });

        return id;
    }

    void TransactionRepository::InsertRegistry(const vector<string> &strings)
    {
        if (strings.empty()) return;

        auto stmt = SetupSqlStatement(R"sql(
            insert or ignore into Registry (String) values
            )sql" + join(vector<string>(strings.size(), "(?)"), ",") + R"sql( ;
        )sql");

        stmt->Bind(strings);
        TryStepStatement(stmt);
    }

    void TransactionRepository::InsertRegistryLists(const vector<string> &lists)
    {
        if (lists.empty()) return;
        static auto stmt = SetupSqlStatement(R"sql(
            insert or ignore into Registry (string)
            select json_each(?)
        )sql");

        for (const auto& list: lists) {
            stmt->Bind(list);
            TryStepStatement(stmt);
        }
    }

    void TransactionRepository::InsertList(const string &list, const string& txHash)
    {
        static auto stmt = SetupSqlStatement(R"sql(
            with t as (
                select a.RowId from Transactions a where a.HashId = (select r.RowId from Registry r where r.String = ?)
            )
            insert or ignore into Lists (TxId, OrderIndex, RegId)
            select * from (
                select t.RowId from t), 
                (
                    select key, -- key will be the index in array
                    (select RowId from Registry where String = value) 
                    from json_each(?)
                )
        )sql");
        stmt->Bind(txHash, list);
        TryStepStatement(stmt);
    }

} // namespace PocketDb

