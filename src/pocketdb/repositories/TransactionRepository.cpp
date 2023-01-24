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
            collectData.txContextData = move(txData);
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
            sort(outputs.begin(), outputs.end(), [](const TransactionOutput& elem1, const TransactionOutput& elem2) { return *elem1.GetNumber() < *elem2.GetNumber(); });
            auto inputs = collectData.inputs;
            sort(inputs.begin(), inputs.end(), [](const TransactionInput& elem1, const TransactionInput& elem2) { return *elem1.GetNumber() < *elem2.GetNumber(); });
            ptx->Inputs() = move(inputs);
            ptx->Outputs() = move(outputs);
            if (collectData.payload) ptx->SetPayload(*collectData.payload);
            return ptx;
        }
    };

    class TransactionReconstructor
    {
    public:
        TransactionReconstructor(map<string, CollectData> initData) {
            m_collectData = move(initData);
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
        vector<CollectData> GetResult()
        {
            vector<CollectData> res;

            transform(
                m_collectData.begin(),
                m_collectData.end(),
                back_inserter(res),
                [](const auto& elem) {return elem.second;});

            return res;
        }

    private:

        map<string, CollectData> m_collectData;

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
            collectData.ptx = move(ptx);            
            collectData.txContextData = move(txContextData);

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

            collectData.payload = move(payload);

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

    static auto _findStringsAndListsToBeInserted(const vector<CollectData>& collectDataVec)
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

        return pair { stringsToBeInserted, listsToBeInserted };
    }

    void TransactionRepository::InsertTransactions(PocketBlock& pocketBlock)
    {
        vector<CollectData> collectDataVec;
        for (const auto& ptx: pocketBlock)
        {
            auto collectData = CollectDataToModelConverter::ModelToCollectData(ptx);
            assert(collectData);
            
            collectDataVec.emplace_back(move(*collectData));
        }

        vector<string> registyStrings;
        vector<string> lists;
        tie(registyStrings, lists) = _findStringsAndListsToBeInserted(collectDataVec);

        SqlTransaction(__func__, [&]()
        {
            InsertRegistry(registyStrings);
            InsertRegistryLists(lists);

            for (const auto& collectData: collectDataVec)
            {
                // Insert general transaction
                InsertTransactionModel(collectData);

                // Insert lists for transaction
                if (collectData.txContextData.list)
                    InsertList(*collectData.txContextData.list, collectData.txHash);

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

        map<string, CollectData> initData;
        for (const auto& hash: txHashes) {
            initData.emplace(hash, CollectData{hash});
        }

        TransactionReconstructor reconstructor(initData);
        SqlTransaction(__func__, [&]()
        {
            auto& stmt = Sql(sql).Bind(txHashes);
            while (stmt.Step() == SQLITE_ROW)
            {
                if (!reconstructor.FeedRow(stmt))
                    throw runtime_error("Transaction::List feedRow failed - no return data");
            }
        });

        auto pBlock = make_shared<PocketBlock>();
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
        if (txIdItr == txIdMap.end())
            return nullptr;

        auto& txId = txIdItr->second;
        PTransactionOutputRef txOutput = nullptr;

        SqlTransaction(__func__, [&]()
        {
            Sql(R"sql(
                select
                    o.Number,
                    (
                        select r.String
                        from Registry r -- primary key
                        where r.RowId = o.AddressId
                    ),
                    o.Value,
                    (
                        select r.String
                        from Registry r -- primary key
                        where r.RowId = o.ScriptPubKeyId
                    )
                from
                    TxOutputs o indexed by TxOutputs_TxId_Number_AddressId
                where
                    o.TxId = ? and
                    o.Number = ?
            )sql")
            .Bind(txId, number)
            .Select([&](Stmt& stmt) {
                if (stmt.Step())
                {
                    txOutput = make_shared<TransactionOutput>();
                    txOutput->SetTxHash(txHash);
                    if (auto[ok, value] = stmt.TryGetColumnInt64(0); ok) txOutput->SetNumber(value);
                    if (auto[ok, value] = stmt.TryGetColumnString(1); ok) txOutput->SetAddressHash(value);
                    if (auto[ok, value] = stmt.TryGetColumnInt64(2); ok) txOutput->SetValue(value);
                    if (auto[ok, value] = stmt.TryGetColumnString(3); ok) txOutput->SetScriptPubKey(value);
                }
            });
        });

        return txOutput;
    }

    bool TransactionRepository::Exists(const string& hash)
    {
        bool result = false;
        SqlTransaction(__func__, [&]()
        {
            result = (
                Sql(R"sql(
                    select
                        1
                    from
                        vTxRowId
                    where
                        String = ?
                )sql")
                .Bind(hash)
                .Run() == SQLITE_ROW
            );
        });

        return result;
    }

    bool TransactionRepository::ExistsInChain(const string& hash)
    {
        bool result = false;
        SqlTransaction(__func__, [&]()
        {
            result = (
                Sql(R"sql(
                    select
                        1
                    from 
                        vTxRowId t
                        cross join Chain c -- primary key
                            on c.TxId = t.RowId
                    where
                        t.String = ''
                )sql")
                .Bind(hash)
                .Step() == SQLITE_ROW
            );
        });

        return result;
    }

    int TransactionRepository::MempoolCount()
    {
        int result = 0;
        SqlTransaction(__func__, [&]()
        {
            Sql(R"sql(
                select
                    (select count() from Transactions)
                    -
                    (select count() from Chain)
            )sql")
            .Select([&](Stmt& stmt) {
                stmt.Collect(result);
            });
        });

        return result;
    }

    void TransactionRepository::CleanTransaction(const string& hash)
    {
        SqlTransaction(__func__, [&]()
        {
            // Clear Payload tablew
            Sql(R"sql(
                with
                    tx as (
                        select
                            t.ROWID
                        from
                            vTxRowId t
                        where
                            t.String = ?
                    )
                delete from Payload
                where
                    TxId = (select RowId from tx) and
                    not exists (
                        select
                            1
                        from
                            tx,
                            Chain c -- primary key
                        where
                            c.TxId = tx.RowId
                    )
            )sql")
            .Bind(hash)
            .Step();

            // Clear TxOutputs table
            Sql(R"sql(
                with
                    tx as (
                        select
                            t.ROWID
                        from
                            vTxRowId t
                        where
                            t.String = ?
                    )
                delete from TxOutputs
                where
                    TxId = (select RowId from tx) and
                    not exists (
                        select
                            1
                        from
                            tx,
                            Chain c -- primary key
                        where
                            c.TxId = tx.RowId
                    )
            )sql")
            .Bind(hash)
            .Step();

            // Clear Transactions table
            Sql(R"sql(
                with
                    tx as (
                        select
                            t.ROWID
                        from
                            vTxRowId t
                        where
                            t.String = ?
                    )
                delete from Transactions
                where
                    RowId = (select RowId from tx) and
                    not exists (
                        select
                            1
                        from
                            tx,
                            Chain c -- primary key
                        where
                            c.TxId = tx.RowId
                    )
            )sql")
            .Bind(hash)
            .Step();
        });
    }

    void TransactionRepository::CleanMempool()
    {
        SqlTransaction(__func__, [&]()
        {
            // Clear Payload table
            Sql(R"sql(
                delete from Payload
                where
                    not exists (
                        select
                            1
                        from
                            Chain c
                        where
                            c.TxId = Payload.TxId
                    )
            )sql")
            .Step();

            // Clear TxOutputs table
            Sql(R"sql(
                delete from TxOutputs
                where
                    not exists (
                        select
                            1
                        from
                            Chain c
                        where
                            c.TxId = TxOutputs.TxId
                    )
            )sql")
            .Step();

            // Clear Transactions table
            Sql(R"sql(
                delete from Transactions
                where
                    not exists (
                        select
                            1
                        from
                            Chain c
                        where
                            c.TxId = Transactions.RowId
                    )
            )sql")
            .Step();
        });
    }

    void TransactionRepository::InsertTransactionInputs(const vector<TransactionInput>& inputs, const string& txHash)
    {
        auto& stmt = Sql(R"sql(
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
            stmt.Bind(
                input.GetSpentTxHash(),
                txHash,
                input.GetNumber()
            ).Step(true);
        }
    }
    
    void TransactionRepository::InsertTransactionOutputs(const vector<TransactionOutput>& outputs, const string& txHash)
    {
        auto& stmt = Sql(R"sql(
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
            stmt.Bind(
                txHash,
                output.GetNumber(),
                output.GetAddressHash(),
                output.GetValue(),
                output.GetScriptPubKey(),
                output.GetNumber()
            ).Step(true);
        }
    }

    void TransactionRepository::InsertTransactionPayload(const Payload& payload)
    {
        Sql(R"sql(
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
        )sql").Bind(
            payload.GetTxHash(),
            payload.GetString1(),
            payload.GetString2(),
            payload.GetString3(),
            payload.GetString4(),
            payload.GetString5(),
            payload.GetString6(),
            payload.GetString7(),
            payload.GetInt1()
        ).Step();
    }

    void TransactionRepository::InsertTransactionModel(const CollectData& collectData)
    {
        Sql(R"sql(
            with
                h as (
                    select RowId as HashId
                    from Registry
                    where String = ?
                )
            insert or fail into 
                Transactions (
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
            select
                ?,
                ?,
                ?,
                h.HashId,
                (
                    select RowId
                    from Registry
                    where String = ?
                ),
                (
                    select RowId
                    from Registry
                    where String = ?
                ),
                (
                    select RowId
                    from Registry
                    where String = ?
                ),
                (
                    select RowId
                    from Registry
                    where String = ?
                ),
                (
                    select RowId
                    from Registry
                    where String = ?
                )
            from
                h
            where
                not exists(
                    select
                        1
                    from
                        Transactions a
                    where
                        a.HashId = h.HashId
                )
        )sql")
        .Bind(
            collectData.txHash,
            (int)*collectData.ptx->GetType(),
            collectData.ptx->GetTime(),
            collectData.txContextData.int1,
            collectData.txContextData.string1,
            collectData.txContextData.string2,
            collectData.txContextData.string3,
            collectData.txContextData.string4,
            collectData.txContextData.string5)
        .Step();
    }

    tuple<bool, PTransactionRef> TransactionRepository::CreateTransactionFromListRow(Stmt& stmt, bool includedPayload)
    {
        // TODO (lostystyg): move deserialization logic to models ?
        // aok: думаю нет - пусть пока тут будет

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
        map<string,int64_t> res;

        SqlTransaction(__func__, [&]()
        {
            Sql(R"sql(
                select
                    String,
                    RowId
                from
                    vTxRowId
                where
                    String in ( )sql" + txReplacers + R"sql( )
            )sql")
            .Bind(txHashes)
            .Select([&](Stmt& stmt) {
                while (stmt.Step() == SQLITE_ROW) {
                    auto[ok1, hash] = stmt.TryGetColumnString(0);
                    auto[ok2, txId] = stmt.TryGetColumnInt64(1);
                    if (ok1 && ok2) res.emplace(hash, txId);
                }
            });
        });

        return res;
    }

    // TODO (lostystyg): need?
    // optional<string> TransactionRepository::TxIdToHash(const int64_t& id)
    // {
    //     optional<string> hash;
    //     SqlTransaction(__func__, [&]()
    //     {
    //         Sql(R"sql(
    //             select Hash
    //             from Transactions
    //             where Id = ?
    //         )sql")
    //         .Bind(id)
    //         .Collect(hash);
    //     });

    //     return hash;
    // }

    // optional<int64_t> TransactionRepository::TxHashToId(const string& hash)
    // {
    //     auto sql = ;

    //     optional<int64_t> id;
    //     SqlTransaction(__func__, [&]()
    //     {
    //         Sql(R"sql(
                
    //         )sql")
    //         .Bind(hash)
    //         .Collect(id);
    //     });

    //     return id;
    // }

    // optional<string> TransactionRepository::AddressIdToHash(const int64_t& id)
    // {
    //     auto sql = R"sql(
    //         select Hash
    //         from Addresses
    //         where Id = ?
    //     )sql";

    //     optional<string> hash;
    //     SqlTransaction(__func__, [&]()
    //     {
    //         auto& stmt = Sql(sql);
    //         stmt.Bind(id);
    //         if (stmt.Step() == SQLITE_ROW)
    //             if (auto [ok, val] = stmt.TryGetColumnString(0); ok)
    //                 hash = val;
    //     });

    //     return hash;
    // }

    // optional<int64_t> TransactionRepository::AddressHashToId(const string& hash)
    // {
    //     auto sql = R"sql(
    //         select Id
    //         from Addresses
    //         where Hash = ?
    //     )sql";

    //     optional<int64_t> id;
    //     SqlTransaction(__func__, [&]()
    //     {
    //         auto& stmt = Sql(sql);
    //         stmt.Bind(hash);
    //         if (stmt.Step() == SQLITE_ROW)
    //             if (auto [ok, val] = stmt.TryGetColumnInt64(0); ok)
    //                 id = val;
    //     });

    //     return id;
    // }

    void TransactionRepository::InsertRegistry(const vector<string> &strings)
    {
        if (strings.empty())
            return;

        // TODO (optimization): create many stmt's for this keys...
        Sql(R"sql(
            insert or ignore into Registry (String) values
            )sql" + join(vector<string>(strings.size(), "(?)"), ",") + R"sql( ;
        )sql")
        .Bind(strings)
        .Step();
    }

    void TransactionRepository::InsertRegistryLists(const vector<string> &lists)
    {
        if (lists.empty())
            return;

        auto& stmt = Sql(R"sql(
            insert or ignore into Registry (string)
            select json_each(?)
        )sql");

        for (const auto& list: lists) {
            stmt.Bind(list).Step(true);
        }
    }

    void TransactionRepository::InsertList(const string &list, const string& txHash)
    {
        Sql(R"sql(
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
        )sql")
        .Bind(txHash, list)
        .Step();
    }

} // namespace PocketDb

