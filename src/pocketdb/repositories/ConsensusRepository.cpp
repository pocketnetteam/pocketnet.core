// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include "pocketdb/repositories/ConsensusRepository.h"

namespace PocketDb
{
    ConsensusData_AccountUser ConsensusRepository::AccountUser(const string& address, int depth, const string& name)
    {
        #pragma region Prepare

        ConsensusData_AccountUser result;
        
        auto _name = EscapeValue(name);

        string sql = R"sql(
            with
                addressRegId as (
                    select
                        r.RowId
                    from
                        Registry r
                    where
                        String = ?
                ),
                lastTx as (
                    select
                        ifnull(min(t.Type),0) as type
                    from
                        addressRegId
                        -- filter registrations & deleting transactions by address
                        cross join Transactions t indexed by Transactions_Type_RegId1_RegId2_RegId3
                            on t.Type in (100, 170) and t.RegId1 = addressRegId.RowId
                        -- filter by chain for exclude mempool
                        cross join Chain c
                            on c.TxId = t.RowId
                        -- filter by Last
                        cross join Last l
                            on l.TxId = t.RowId
                ),
                edits as (
                    select
                        count() cnt
                    from
                        addressRegId
                        -- filter registrations transactions by address
                        cross join Transactions t indexed by Transactions_Type_RegId1_RegId2_RegId3
                            on t.Type = 100 and t.RegId1 = addressRegId.RowId
                        -- filter by height for exclude mempool
                        cross join Chain c
                            on c.TxId = t.RowId and c.Height >= ?
                ),
                mempool as (
                    select
                        count()cnt
                    from
                        addressRegId
                        -- filter registrations transactions and address
                        cross join Transactions t indexed by Transactions_Type_RegId1_RegId2_RegId3
                            on t.Type in (100, 170) and t.RegId1 = addressRegId.RowId
                    where
                        -- include only non-chain transactions
                        not exists(select 1 from Chain c where c.TxId = t.RowId)
                ),
                duplicatesChain as (
                    select
                        count() cnt
                    from
                        addressRegId
                        -- find all same names in payload
                        cross join Payload p
                            on (p.String2 like ? escape '\')
                        cross join Transactions t
                            on t.RowId = p.TxId and t.Type = 100 and t.RegId1 != addressRegId.RowId
                        -- filter by chain for exclude mempool
                        cross join Chain c
                            on c.TxId = t.RowId
                        -- filter by Last
                        cross join Last l
                            on l.TxId = t.RowId
                ),
                duplicatesMempool as (
                    select
                        count() cnt
                    from
                        addressRegId
                        -- find all same names in payload
                        cross join Payload p
                            on (p.String2 like ? escape '\')
                        cross join Transactions t
                            on t.RowId = p.TxId and t.Type = 100 and t.RegId1 != addressRegId.RowId
                        -- filter by chain for exclude mempool
                        cross join Chain c
                            on c.TxId = t.RowId
                    where
                        -- include only non-chain transactions
                        not exists(select 1 from Chain c where c.TxId = t.RowId)
                )
            select
                lastTx.type,
                edits.cnt,
                mempool.cnt,
                duplicatesChain.cnt,
                duplicatesMempool.cnt
            from
                lastTx,
                edits,
                mempool,
                duplicatesChain,
                duplicatesMempool
        )sql";

        #pragma endregion

        SqlTransaction(__func__, [&]()
        {
            Sql(sql)
            .Bind(address, depth, _name, _name)
            .Select([&](Cursor& cursor) {
                if (cursor.Step())
                {
                    cursor.CollectAll(
                        result.LastTxType,
                        result.EditsCount,
                        result.MempoolCount,
                        result.DuplicatesChainCount,
                        result.DuplicatesMempoolCount
                    );
                }
            });
        });

        return result;
    }

    ConsensusData_BarteronAccount ConsensusRepository::BarteronAccount(const string& address)
    {
        #pragma region Prepare

        ConsensusData_BarteronAccount result;
        
        string sql = R"sql(
            with

                addressRegId as (
                    select
                        r.RowId
                    from
                        Registry r
                    where
                        String = ?
                ),

                mempool as (
                    select
                        count()cnt
                    from
                        addressRegId,
                        Transactions t
                    where
                        t.Type in (104) and
                        t.RegId1 = addressRegId.RowId and
                        -- include only non-chain transactions
                        not exists(select 1 from Chain c where c.TxId = t.RowId)
                ),

            select
                mempool.cnt
            from
                mempool
        )sql";

        #pragma endregion

        SqlTransaction(__func__, [&]()
        {
            Sql(sql)
            .Bind(address)
            .Select([&](Cursor& cursor) {
                if (cursor.Step())
                {
                    cursor.CollectAll(
                        result.MempoolCount
                    );
                }
            });
        });

        return result;
    }

    ConsensusData_BarteronOffer ConsensusRepository::BarteronOffer(const string& address, const string& rootTxHash)
    {
        #pragma region Prepare

        ConsensusData_BarteronOffer result;
        
        string sql = R"sql(
            with

                addressRegId as (
                    select
                        r.RowId
                    from
                        Registry r
                    where
                        String = ?
                ),

                rootRegId as (
                    select
                        r.RowId
                    from
                        Registry r
                    where
                        String = ?
                ),

                lastTx as (
                    select
                        ifnull(min(t.Type),0) type
                    from
                        addressRegId,
                        rootRegId,
                        Transactions t -- todo : index
                    where
                        t.Type in (211) and
                        t.RegId1 = addressRegId.RowId and
                        t.RegId2 = rootRegId.RowId and
                        -- filter by chain for exclude mempool
                        exists(select 1 from Chain c where c.TxId = t.RowId) and
                        -- filter by Last
                        exists(select 1 from Last l where l.TxId = t.RowId)
                ),

                active as (
                    select
                        count()cnt
                    from
                        addressRegId,
                        Transactions t
                    where
                        t.Type in (211) and
                        t.RegId1 = addressRegId.RowId and
                        -- include only chain transactions
                        exists (select 1 from Chain c where c.TxId = t.RowId)
                ),

                mempool as (
                    select
                        count()cnt
                    from
                        addressRegId,
                        Transactions t
                    where
                        t.Type in (211) and
                        t.RegId1 = addressRegId.RowId and
                        -- include only non-chain transactions
                        not exists (select 1 from Chain c where c.TxId = t.RowId)
                ),

            select
                lastTx.type,
                active.cnt,
                mempool.cnt
            from
                lastTx,
                active,
                mempool
        )sql";

        #pragma endregion

        SqlTransaction(__func__, [&]()
        {
            Sql(sql)
            .Bind(address, rootTxHash)
            .Select([&](Cursor& cursor) {
                if (cursor.Step())
                {
                    cursor.CollectAll(
                        result.LastTxType,
                        result.ActiveCount,
                        result.MempoolCount
                    );
                }
            });
        });

        return result;
    }



    // ------------------------------------------------------------------------------------------



    // bool ConsensusRepository::ExistsUserRegistrations(vector<string>& addresses)
    // {
    //     auto result = false;

    //     if (addresses.empty())
    //         return result;

    //     // Build sql string
    //     string sql = R"sql(
    //         select count()
    //         from Transactions indexed by Transactions_Type_Last_String1_Height_Id
    //         where Type in (100)
    //           and Last = 1
    //           and String1 in ( )sql" + join(vector<string>(addresses.size(), "?"), ",") + R"sql( )
    //           and Height is not null
    //     )sql";

    //     // Execute
    //     SqlTransaction(__func__, [&]()
    //     {
    //         auto stmt = Sql(sql);

    //         stmt.Bind(addresses);

    //         // if (stmt.Step())
    //         //     if (auto[ok, value] = stmt.TryGetColumnInt(0); ok)
    //         //         result = (value == (int) addresses.size());
    //     });

    //     return result;
    // }

    // bool ConsensusRepository::ExistsAccountBan(const string& address, int height)
    // {
    //     auto result = false;

    //     string sql = R"sql(
    //         select
    //             1
    //         from
    //             Transactions u indexed by Transactions_Type_Last_String1_Height_Id
    //             cross join JuryBan b indexed by JuryBan_AccountId_Ending
    //                 on b.AccountId = u.Id and b.Ending > ?
    //         where
    //             u.Type = 100 and
    //             u.Last = 1 and
    //             u.String1 = ? and
    //             u.Height > 0
    //     )sql";

    //     SqlTransaction(__func__, [&]()
    //     {
    //         Sql(sql)
    //         .Bind(height, address)
    //         .Select([&](Cursor& cursor) {
    //             result = cursor.Step();
    //         });
    //     });

    //     return result;
    // }


    bool ConsensusRepository::ExistsAnotherByName(const string& address, const string& name)
    {
        bool result = false;

        auto _name = EscapeValue(name);

        SqlTransaction(__func__, [&]()
        {
            Sql(R"sql(
                with
                    addressRegId as (
                        select
                            RowId
                        from
                            Registry
                        where
                            String = ?
                    )

                select
                    count()
                from
                    addressRegId
                    -- find all same names in payload
                    cross join Payload p
                        on (p.String2 like ? escape '\')
                    cross join Transactions t
                        on t.RowId = p.TxId and t.Type = 100 and t.RegId1 != addressRegId.RowId
                    -- filter by chain for exclude mempool
                    cross join Chain c
                        on c.TxId = t.RowId
                    -- filter by Last
                    cross join Last l
                        on l.TxId = t.RowId
            )sql")
            .Bind(address, _name)
            .Select([&](Cursor& cursor) {
                if (cursor.Step())
                    cursor.CollectAll(result);
            });
        });

        return result;
    }

    tuple<bool, PTransactionRef> ConsensusRepository::GetFirstContent(const string& rootHash)
    {
        PTransactionRef tx = nullptr;

        SqlTransaction(__func__, [&]()
        {
            string sql = R"sql(
                with str2 as (
                    select
                        r.String as string,
                        r.RowId as id
                    from
                        Registry r
                    where
                        r.String = ?
                )
                select
                    t.Type,
                    s.Hash,
                    t.Time,
                    iif(l.TxId, 1, 0),
                    c.Uid,
                    s.String1,
                    s.String2,
                    s.String3,
                    s.String4,
                    s.String5,
                    t.Int1,
                    s.Hash pHash,
                    p.String1 pString1,
                    p.String2 pString2,
                    p.String3 pString3,
                    p.String4 pString4,
                    p.String5 pString5,
                    p.String6 pString6,
                    p.String7 pString7
                from
                    str2,
                    Transactions t indexed by Transactions_Type_RegId2
                    cross join vTxStr s on
                        s.RowId = t.RowId
                    join Chain c on
                        c.TxId = t.RowId
                    left join Payload p on
                        t.RowId = p.TxId
                    cross join First f on f.TxId = t.RowId
                    left join Last l on l.TxId = t.RowId
                where
                    t.Type in (200,201,202,203,204,209,210,211,220) and
                    t.RegId2 = str2.id
            )sql";

            Sql(sql)
            .Bind(rootHash)
            .Select([&](Cursor& cursor) {
                if (cursor.Step())
                    if (auto[ok, transaction] = CreateTransactionFromListRow(cursor, true); ok)
                        tx = std::move(transaction);
            });
        });

        return {tx != nullptr, tx};
    }

    tuple<bool, PTransactionRef> ConsensusRepository::GetLastContent(const string& rootHash, const vector<TxType>& types)
    {
        PTransactionRef tx = nullptr;

        SqlTransaction(__func__, [&]()
        {
            string sql = R"sql(
                with s2 as (
                    select
                        r.String as str,
                        r.RowId as id
                    from
                        Registry r
                    where
                        r.String = ?
                )
                select
                    t.Type,
                    st.Hash,
                    t.Time,
                    iif(l.TxId, 1, 0),
                    c.Uid,
                    st.String1,
                    st.String2,
                    st.String3,
                    st.String4,
                    st.String5,
                    t.Int1,
                    st.Hash pHash,
                    p.String1 pString1,
                    p.String2 pString2,
                    p.String3 pString3,
                    p.String4 pString4,
                    p.String5 pString5,
                    p.String6 pString6,
                    p.String7 pString7
                from
                    s2,
                    Transactions t indexed by Transactions_Type_RegId2
                    cross join vTxStr st on
                        st.RowId = t.RowId
                    join Chain c on
                        c.TxId = t.RowId
                    cross join Last l on
                        l.TxId = t.RowId
                    left join Payload p on
                        t.RowId = p.TxId
                where
                    t.Type in ( )sql" + join(vector<string>(types.size(), "?"), ",") + R"sql( ) and
                    t.RegId2 = s2.id
            )sql";

            Sql(sql)
            .Bind(rootHash, types)
            .Select([&](Cursor& cursor) {
                if (cursor.Step())
                    if (auto[ok, transaction] = CreateTransactionFromListRow(cursor, true); ok)
                        tx = std::move(transaction);
            });
        });

        return {tx != nullptr, tx};
    }

    tuple<bool, vector<PTransactionRef>> ConsensusRepository::GetLastContents(const vector<string> &rootHashes,
                                                                              const vector<TxType> &types)
    {
        vector<PTransactionRef> txs;

        SqlTransaction(__func__, [&]()
        {
            string sql = R"sql(
                with strs2 as (
                    select
                        r.String as string,
                        r.RowId as id
                    from
                        Registry r
                    where
                        r.String in ( )sql" + join(vector<string>(rootHashes.size(), "?"), ",") + R"sql( )
                )
                select
                    t.Type,
                    s.Hash,
                    t.Time,
                    iif(l.TxId, 1, 0),
                    c.Uid,
                    s.String1,
                    s.String2,
                    s.String3,
                    s.String4,
                    s.String5,
                    t.Int1,
                    s.Hash pHash,
                    p.String1 pString1,
                    p.String2 pString2,
                    p.String3 pString3,
                    p.String4 pString4,
                    p.String5 pString5,
                    p.String6 pString6,
                    p.String7 pString7
                from
                    strs2,
                    Transactions t indexed by Transactions_Type_RegId2
                    cross join vTxStr s on
                        s.RowId = t.RowId
                    join Chain c on
                        c.TxId = t.RowId
                    left join Payload p on
                        t.RowId = p.TxId
                    cross join Last l on
                        l.TxId = t.RowId
                where
                    t.Type in ( )sql" + join(vector<string>(types.size(), "?"), ",") + R"sql( ) and
                    t.RegId2 = strs2.id
            )sql";

            Sql(sql)
            .Bind(rootHashes, types)
            .Select([&](Cursor& cursor) {
                while (cursor.Step())
                {
                    if (auto[ok, transaction] = CreateTransactionFromListRow(cursor, true); ok)
                        txs.emplace_back(transaction);
                }
            });
        });

        return {txs.size() != 0, txs};
    }

    tuple<bool, TxType> ConsensusRepository::GetLastBlockingType(const string& address, const string& addressTo)
    {
        bool blockingExists = false;
        TxType blockingType = TxType::NOT_SUPPORTED;

        SqlTransaction(__func__, [&]()
        {
            Sql(R"sql(
                with
                    str1 as (
                        select
                            r.String as str,
                            r.RowId as id
                        from
                            Registry r
                        where
                            r.String = ?
                    ),
                    str2 as (
                        select
                            r.String as str,
                            r.RowId as id
                        from
                            Registry r
                        where
                            r.String = ?
                    )
                select
                    Type
                from
                    str1,
                    str2,
                    Transactions t indexed by Transactions_Type_RegId1_RegId2_RegId3
                    cross join Last l on l.TxId = t.RowId
                where
                    t.Type in (305, 306) and
                    t.RegId1 = str1.id and
                    t.RegId2 = str2.id
            )sql")
            .Bind(address, addressTo)
            .Select([&](Cursor& cursor) {
                if (cursor.Step())
                {
                    if (auto[ok, value] = cursor.TryGetColumnInt(0); ok)
                    {
                        blockingExists = true;
                        blockingType = (TxType) value;
                    }
                }
            });
        });

        return {blockingExists, blockingType};
    }

    bool ConsensusRepository::ExistBlocking(const string& address, const string& addressTo, const string& addressesTo)
    {
        bool blockingExists = false;

        SqlTransaction(__func__, [&]()
        {
            Sql(R"sql(
                with
                    -- TODO (optimization): generalize
                    addr1 as (
                        select
                            r.String as hash,
                            r.RowId as id
                        from
                            Registry r
                        where
                            r.String = ?
                    ),
                    addrs2 as (
                        select
                            r.String as hash,
                            r.RowId as id
                        from
                            Registry r
                        where
                            r.String in (select ? union select value from json_each(?))
                    ),
                    -- TODO (optimization): generalize
                    sourceId as (
                        select
                            c.Uid as id
                        from
                            addr1,
                            Transactions t indexed by Transactions_Type_RegId1_RegId2_RegId3
                            join Chain c on
                                c.TxId = t.RowId
                        where
                            t.Type in (100, 170) and
                            t.RegId1 = addr1.id and
                            exists (select 1 from Last l where l.TxId = t.RowId)
                    ),
                    targetIds as (
                        select
                            c.Uid as id
                        from
                            addrs2,
                            Transactions t indexed by Transactions_Type_RegId1_RegId2_RegId3
                            join Chain c on
                                c.TxId = t.RowId
                        where
                            t.Type in (100, 170) and
                            t.RegId1 = addrs2.id and
                            exists (select 1 from Last l where l.TxId = t.RowId)
                    )
                select 1
                from
                    sourceId,
                    targetIds,
                    BlockingLists b
                where
                    b.IdSource = sourceId.id and
                    b.IdTarget = targetIds.id
                limit 1
            )sql")
            .Bind(address, addressTo, addressesTo)
            .Select([&](Cursor& cursor) {
                if (cursor.Step())
                    if (auto [ok, value] = cursor.TryGetColumnInt(0); ok && value > 0)
                        blockingExists = true;
            });
        });

        return blockingExists;
    }

    tuple<bool, TxType> ConsensusRepository::GetLastSubscribeType(const string& address,
        const string& addressTo)
    {
        bool subscribeExists = false;
        TxType subscribeType = TxType::NOT_SUPPORTED;

        SqlTransaction(__func__, [&]()
        {
            Sql(R"sql(
                with
                    str1 as (
                        select
                            r.RowId as id
                        from
                            Registry r
                        where
                            r.String = ?
                    ),
                    str2 as (
                        select
                            r.RowId as id
                        from
                            Registry r
                        where
                            r.String = ?
                    )
                select
                    t.Type
                from
                    str1,
                    str2,
                    Transactions t indexed by Transactions_Type_RegId1_RegId2_RegId3
                    cross join Last l on l.TxId = t.RowId
                where
                    t.Type in (302, 303, 304) and
                    t.RegId1 = str1.id and
                    t.RegId2 = str2.id
            )sql")
            .Bind(address, addressTo)
            .Select([&](Cursor& cursor) {
                if (cursor.Step())
                    cursor.Collect(0, [&](int value) {
                        subscribeExists = true;
                        subscribeType = (TxType) value;
                    });
            });
        });

        return {subscribeExists, subscribeType};
    }

    optional<string> ConsensusRepository::GetContentAddress(const string& postHash)
    {
        optional<string> result = nullopt;

        SqlTransaction(__func__, [&]()
        {
            Sql(R"sql(
                select
                    s.String1
                from
                    vTx t
                    cross join vTxStr s on
                        s.RowId = t.RowId
                where
                    t.Hash = ? and
                    exists (select 1 from Chain c where c.TxId = t.RowId)
            )sql")
            .Bind(postHash)
            .Select([&](Cursor& cursor) {
                if (cursor.Step())
                    cursor.Collect(0, [&](const string& value) {
                        result = value;
                    });
            });
        });

        return result;
    }

    bool ConsensusRepository::ExistsComplain(const string& postHash, const string& address, bool mempool)
    {
        bool result = false;

        SqlTransaction(__func__, [&]()
        {
            string sql = R"sql(
                with
                    str1 as (
                        select
                            r.RowId as id
                        from
                            Registry r
                        where
                            r.String = ?
                    ),
                    str2 as (
                        select
                            r.RowId as id
                        from
                            Registry r
                        where
                            r.String = ?
                    )
                select
                    -- TODO (optimization): why not select 1?
                    count(*)
                from
                    Transactions t indexed by Transactions_Type_RegId1_RegId2_RegId3
                where
                    t.Type in (307) and
                    t.RegId1 = (select id from str1) and
                    t.RegId2 = (select id from str2)
            )sql";

            if (!mempool)
                sql += "    and exists (select 1 from Chain c where c.TxId = t.RowId)";

            Sql(sql)
            .Bind(address, postHash)
            .Select([&](Cursor& cursor) {
                if (cursor.Step())
                    cursor.Collect(0, [&](int value) {
                        result = (value > 0);
                    });
            });
        });

        return result;
    }

    bool ConsensusRepository::ExistsScore(const string& address, const string& contentHash, TxType type, bool mempool)
    {
        bool result = false;

        string sql = R"sql(
            with
                str1 as (
                    select
                        r.RowId as id
                    from
                        Registry r
                    where
                        r.String = ?
                ),
                str2 as (
                    select
                        r.RowId as id
                    from
                        Registry r
                    where
                        r.String = ?
                )
            select
                count(*)
            from
                Transactions t indexed by Transactions_Type_RegId1_RegId2_RegId3
            where
                t.Type = ? and
                t.RegId1 = (select id from str1) and
                t.RegId2 = (select id from str2)
        )sql";

        if (!mempool)
            sql += "    and exists (select 1 from Chain c where c.TxId = t.RowId)";

        SqlTransaction(__func__, [&]()
        {
            Sql(sql)
            .Bind(address, contentHash, (int) type)
            .Select([&](Cursor& cursor) {
                if (cursor.Step())
                    cursor.Collect(0, [&](int value) {
                        result = (value > 0);
                    });
            });
        });

        return result;
    }

    bool ConsensusRepository::ExistsActiveJury(const string& juryId)
    {
        assert(juryId != "");
        bool result = false;

        SqlTransaction(__func__, [&]()
        {
            Sql(R"sql(
                select
                    1
                from
                    hash,
                    vTx t
                    join Jury j
                        on j.FlagRowId = t.RowId
                    left join JuryVerdict jv
                        on jv.FlagRowId = j.FlagRowId
                where
                    t.Hash = ? and
                    jv.Verdict is null
            )sql")
            .Bind(juryId)
            .Select([&](Cursor& cursor) {
                result = cursor.Step();
            });
        });

        return result;
    }

    
    bool ConsensusRepository::Exists_S1S2T(const string& string1, const string& string2, const vector<TxType>& types)
    {
        assert(string1 != "");
        assert(string2 != "");
        assert(!types.empty());
        bool result = false;

        string sql = R"sql(
            with
                str1 as (
                    select
                        r.RowId as id
                    from
                        Registry r
                    where
                        r.String = ?
                ),
                str2 as (
                    select
                        r.RowId as id
                    from
                        Registry r
                    where
                        r.String = ?
                )
            select 1
            from
                Transactions t indexed by Transactions_Type_RegId1_RegId2_RegId3
            where
                t.Type in ( )sql" + join(vector<string>(types.size(), "?"), ",") + R"sql( ) and
                t.RegId1 = (select id from str1) and
                t.RegId2 = (select id from str2) and
                exists (select 1 from Chain c where c.TxId = t.RowId)
        )sql";

        SqlTransaction(__func__, [&]()
        {
            Sql(sql)
            .Bind(string1, string2, types)
            .Select([&](Cursor& cursor) {
                result = cursor.Step();
            });
        });

        return result;
    }

    bool ConsensusRepository::Exists_MS1T(const string& string1, const vector<TxType>& types)
    {
        assert(string1 != "");
        assert(!types.empty());
        bool result = false;

        SqlTransaction(__func__, [&]()
        {
            Sql(R"sql(
                with
                    address1 as (
                        select RowId
                        from Registry
                        where String = ?
                    )

                select 1
                from address1
                cross join Transactions t indexed by Transactions_Type_RegId1_RegId2_RegId3
                    on  t.RegId1 = address1.RowId and
                        t.Type in ( )sql" + join(vector<string>(types.size(), "?"), ",") + R"sql( ) and
                        not exists (select 1 from Chain c where c.TxId = t.RowId)
            )sql")
            .Bind(string1, types)
            .Select([&](Cursor& cursor) {
                result = cursor.Step();
            });
        });

        return result;
    }

    bool ConsensusRepository::Exists_MS1S2T(const string& string1, const string& string2, const vector<TxType>& types)
    {
        assert(string1 != "");
        assert(string2 != "");
        assert(!types.empty());
        bool result = false;

        SqlTransaction(__func__, [&]()
        {
            Sql(R"sql(
                with
                    address1 as (
                        select RowId
                        from Registry
                        where String = ?
                    ),
                    address2 as (
                        select RowId
                        from Registry
                        where String = ?
                    )

                select 1
                from address1, address2
                cross join Transactions t indexed by Transactions_Type_RegId1_RegId2_RegId3
                    on  t.RegId1 = address1.RowId and 
                        t.RegId2 = address2.RowId and 
                        t.Type in ( )sql" + join(vector<string>(types.size(), "?"), ",") + R"sql( ) and
                        not exists (select 1 from Chain c where c.TxId = t.RowId)
            )sql")
            .Bind(string1, string2, types)
            .Select([&](Cursor& cursor) {
                result = cursor.Step();
            });
        });

        return result;
    }

    bool ConsensusRepository::Exists_LS1T(const string& string1, const vector<TxType>& types)
    {
        assert(string1 != "");
        assert(!types.empty());
        bool result = false;

        string sql = R"sql(
            with
                str1 as (
                    select
                        r.RowId as id
                    from
                        Registry r
                    where
                        r.String = ?
                )
            select 1
            from
                str1,
                Transactions t indexed by Transactions_Type_RegId1_RegId2_RegId3
            where
                t.Type in ( )sql" + join(vector<string>(types.size(), "?"), ",") + R"sql( ) and
                RegId1 = str1.id and
                exists (select 1 from Last l where l.TxId = t.RowId)
        )sql";

        SqlTransaction(__func__, [&]()
        {
            Sql(sql)
            .Bind(string1, types)
            .Select([&](Cursor& cursor) {
                result = cursor.Step();
            });
        });

        return result;
    }

    bool ConsensusRepository::Exists_LS1S2T(const string& string1, const string& string2, const vector<TxType>& types)
    {
        assert(string1 != "");
        assert(string2 != "");
        assert(!types.empty());
        bool result = false;

        string sql = R"sql(
            with
                str1 as (
                    select
                        r.RowId as id
                    from
                        Registry r
                    where
                        r.String = ?
                ),
                str2 as (
                    select
                        r.RowId as id
                    from
                        Registry r
                    where
                        r.String = ?
                )
            select 1
            from
                Transactions t indexed by Transactions_Type_RegId1_RegId2_RegId3
            where
                t.Type in ( )sql" + join(vector<string>(types.size(), "?"), ",") + R"sql( ) and
                t.RegId1 = (select id from str1) and
                t.RegId2 = (select id from str2) and
                exists (select 1 from Last l where l.TxId = t.RowId)
        )sql";

        SqlTransaction(__func__, [&]()
        {
            Sql(sql)
            .Bind(string1, string2, types)
            .Select([&](Cursor& cursor) {
                result = cursor.Step();
            });
        });

        return result;
    }

    bool ConsensusRepository::Exists_HS1T(const string& txHash, const string& string1, const vector<TxType>& types, bool last)
    {
        assert(txHash != "");
        assert(string1 != "");
        assert(!types.empty());
        bool result = false;

        string sql = R"sql(
            with
                str1 as (
                    select
                        r.String as id
                    from
                        Registry r
                    where
                        r.String = ?
                )
            select 1
            from
                str1,
                vTx t
            where t.Hash = ? and
                t.Type in ( )sql" + join(vector<string>(types.size(), "?"), ",") + R"sql( ) and
                )sql" + (last ? "   exists (select 1 from Last l where l.TxId = t.RowId) and" : "") + R"sql(
                t.RegId1 = str1.id
        )sql";

        SqlTransaction(__func__, [&]()
        {
            Sql(sql)
            .Bind(string1, txHash, types)
            .Select([&](Cursor& cursor) {
                result = cursor.Step();
            });
        });

        return result;
    }

    bool ConsensusRepository::Exists_HS2T(const string& txHash, const string& string2, const vector<TxType>& types, bool last)
    {
        assert(txHash != "");
        assert(string2 != "");
        assert(!types.empty());
        bool result = false;

        string sql = R"sql(
            select 1
            from Transactions indexed by sqlite_autoindex_Transactions_1
            where Hash = ?
              and Type in ( )sql" + join(vector<string>(types.size(), "?"), ",") + R"sql( )
              )sql" + (last ? " and Last = 1 " : "") + R"sql(
              and String2 = ?
              and Height is not null
        )sql";

        SqlTransaction(__func__, [&]()
        {
            Sql(sql)
            .Bind(txHash, types, string2)
            .Select([&](Cursor& cursor) {
                result = cursor.Step();
            });
        });

        return result;
    }

    bool ConsensusRepository::Exists_HS1S2T(const string& txHash, const string& string1, const string& string2, const vector<TxType>& types, bool last)
    {
        assert(txHash != "");
        assert(string1 != "");
        assert(string2 != "");
        assert(!types.empty());
        bool result = false;

        string sql = R"sql(
            select 1
            from Transactions indexed by sqlite_autoindex_Transactions_1
            where Hash = ?
              and Type in ( )sql" + join(vector<string>(types.size(), "?"), ",") + R"sql( )
              )sql" + (last ? " and Last = 1 " : "") + R"sql(
              and String1 = ?
              and String2 = ?
              and Height is not null
        )sql";

        SqlTransaction(__func__, [&]()
        {
            Sql(sql)
            .Bind(txHash, types, string1, string2)
            .Select([&](Cursor& cursor) {
                result = cursor.Step();
            });
        });

        return result;
    }

    bool ConsensusRepository::ExistsNotDeleted(const string& txHash, const string& address, const vector<TxType>& types)
    {
        bool result = false;

        string sql = R"sql(
            select 1
            from Transactions t
            where t.Hash = ?
              and t.Type in ( )sql" + join(vector<string>(types.size(), "?"), ",") + R"sql( )
              and t.String1 = ?
              and t.Height > 0
              and not exists (select 1 from Transactions d indexed by Transactions_Id_Last where d.Id = t.Id and d.Last = 1 and d.Type in (207,206))
        )sql";

        SqlTransaction(__func__, [&]()
        {
            auto stmt = Sql(sql);

            stmt.Bind(txHash, types, address);

            // if (stmt.Step())
            //     result = true;
        });

        return result;
    }


    int64_t ConsensusRepository::GetUserBalance(const string& address)
    {
        int64_t result = 0;

        SqlTransaction(__func__, [&]()
        {
            Sql(R"sql(
                with
                    address as (
                        select RowId
                        from Registry
                        where String = ?
                    )

                select Value
                from address
                cross join Balances b
                    on b.AddressId = address.RowId
            )sql")
            .Bind(address)
            .Select([&](Cursor& cursor) {
                if (cursor.Step())
                    cursor.CollectAll(result);
            });
        });

        return result;
    }

    int ConsensusRepository::GetUserReputation(const string& address)
    {
        int result = 0;

        SqlTransaction(__func__, [&]()
        {
            Sql(R"sql(
                with
                    address as (
                        select RowId
                        from Registry
                        where String = ?
                    )

                select
                    r.Value

                from address

                cross join Transactions u indexed by Transactions_Type_RegId1_RegId2_RegId3
                    on u.Type in (100, 170) and u.RegId1 = address.RowId

                cross join Last lu
                    on lu.TxId = u.RowId

                cross join Chain cu
                    on cu.TxId = u.RowId

                cross join Ratings r indexed by Ratings_Type_Uid_Last_Value
                    on r.Type = 0 and r.Uid = cu.Uid and r.Last = 1
            )sql")
            .Bind(address)
            .Select([&](Cursor& cursor) {
                if (cursor.Step())
                    cursor.CollectAll(result);
            });
        });

        return result;
    }

    int ConsensusRepository::GetUserReputation(int addressId)
    {
        int result = 0;

        SqlTransaction(__func__, [&]()
        {
            Sql(R"sql(
                select r.Value
                from Ratings r
                where r.Type = 0
                and r.Uid = ?
                and r.Last = 1
            )sql")
            .Bind(addressId)
            .Select([&](Cursor& cursor) {
                if (cursor.Step())
                    cursor.CollectAll(result);
            });
        });

        return result;
    }

    // TODO (aok): maybe remove in future?
    int64_t ConsensusRepository::GetAccountRegistrationTime(const string& address)
    {
        int64_t result = 0;

        SqlTransaction(__func__, [&]()
        {
            Sql(R"sql(
                with
                    address as (
                        select RowId
                        from Registry
                        where String = ?
                    )

                select Time
                from address
                cross join Transactions u indexed by Transactions_Type_RegId1_RegId2_RegId3
                    on u.Type in (100, 170) and u.RegId1 = address.RowId
                cross join First fu
                    on fu.TxId = u.RowId
            )sql")
            .Bind(address)
            .Select([&](Cursor& cursor) {
                if (cursor.Step())
                    cursor.CollectAll(result);
            });
        });

        return result;
    }

    map<string, AccountData> ConsensusRepository::GetAccountsData(const vector<string>& addresses)
    {
        map<string, AccountData> result;

        SqlTransaction(__func__, [&]()
        {
            Sql(R"sql(
                select

                    (accs.String)Addresshash,
                    (cu.Uid)AddressId,
                    uf.Time as RegistrationDate,
                    cuf.Height as RegistrationHeight,
                    ifnull(b.Value,0)Balance,
                    ifnull(r.Value,0)Reputation,
                    ifnull(lp.Value,0)LikersContent,
                    ifnull(lc.Value,0)LikersComment,
                    ifnull(lca.Value,0)LikersCommentAnswer,
                    iif(bmod.Badge,1,0)ModeratorBadge

                from Registry accs

                cross join Transactions u indexed by Transactions_Type_RegId1_RegId2_RegId3
                    on u.Type in (100, 170) and u.RegId1 = accs.RowId

                cross join Last lu
                    on lu.TxId = u.RowId

                cross join Chain cu
                    on cu.TxId = u.RowId

                cross join Chain cuf
                    on cuf.Uid = cu.Uid

                cross join First fu
                    on fu.TxId = cuf.TxId

                cross join Transactions uf
                    on uf.RowId = fu.TxId

                left join Balances b
                    on b.AddressId = accs.RowId

                left join Ratings r indexed by Ratings_Type_Uid_Last_Value
                    on r.Type = 0 and r.Uid = cu.Uid and r.Last = 1

                left join Ratings lp indexed by Ratings_Type_Uid_Last_Value
                    on lp.Type = 111 and lp.Uid = cu.Uid and lp.Last = 1

                left join Ratings lc indexed by Ratings_Type_Uid_Last_Value
                    on lc.Type = 112 and lc.Uid = cu.Uid and lc.Last = 1

                left join Ratings lca indexed by Ratings_Type_Uid_Last_Value
                    on lca.Type = 113 and lca.Uid = cu.Uid and lca.Last = 1

                left join vBadges bmod
                    on bmod.Badge = 3 and bmod.AccountId = cu.Uid

                where
                    accs.String in ( )sql" + join(vector<string>(addresses.size(), "?"), ",") + R"sql( )

            )sql")
            .Bind(addresses)
            .Select([&](Cursor& cursor) {
                while (cursor.Step())
                {
                    AccountData data;
                    
                    cursor.CollectAll(
                        data.AddressHash,
                        data.AddressId,
                        data.RegistrationTime,
                        data.RegistrationHeight,
                        data.Balance,
                        data.Reputation,
                        data.LikersContent,
                        data.LikersComment,
                        data.LikersCommentAnswer,
                        data.ModeratorBadge
                    );

                    result.emplace(data.AddressHash, data);
                }
            });
        });

        return result;
    }

    map<string, ScoreDataDtoRef> ConsensusRepository::GetScoresData(int height, int64_t scores_time_depth)
    {
        map<string, ScoreDataDtoRef> result;

        SqlTransaction(__func__, [&]()
        {
            Sql(R"sql(
                with
                    height as (
                        select ? as value
                    ),
                    time_depth as (
                        select ? as value
                    )

                select

                    (select r.String from Registry r where r.RowId = s.HashId)sTxHash,
                    (s.Type)sType,
                    (s.Time)sTime,
                    (s.Int1)sValue,
                    (csa.Uid)saId,
                    (select r.String from Registry r where r.RowId = sa.RegId1)saHash,
                    (select r.String from Registry r where r.RowId = c.HashId)cTxHash,
                    (c.Type)cType,
                    (c.Time)cTime,
                    (cc.Uid)cId,
                    (cca.Uid)caId,
                    (select r.String from Registry r where r.RowId = ca.RegId1)caHash,
                    (select r.String from Registry r where r.RowId = c.RegId5)CommentAnswerRootTxHash,

                    (
                        case
                            when s.Type = 300 then
                            (
                                select
                                    count()
                                from Transactions c_s indexed by Transactions_Type_RegId1_Int1_Time
                                cross join Chain c_sc
                                    on c_sc.TxId = c_s.RowId
                                cross join Transactions c_c indexed by Transactions_HashId
                                    on c_c.HashId = c_s.RegId2 and c_c.RegId1 = c.RegId1
                                cross join First f
                                    on f.TxId = c_c.RowId
                                where
                                    c_s.Type = s.Type and
                                    c_s.RegId1 = s.RegId1 and
                                    c_s.Int1 in ( 1,2,3,4,5 ) and
                                    c_s.Time >= s.Time - time_depth.value and
                                    c_s.Time < s.Time and
                                    c_s.RowId != s.RowId
                            )
                            else
                            (
                                select
                                    count()
                                from Transactions c_s indexed by Transactions_Type_RegId1_Int1_Time
                                cross join Chain c_sc
                                    on c_sc.TxId = c_s.RowId
                                cross join Transactions c_c indexed by Transactions_HashId
                                    on c_c.HashId = c_s.RegId2 and c_c.RegId1 = c.RegId1
                                cross join First f
                                    on f.TxId = c_c.RowId
                                where
                                    c_s.Type = s.Type and
                                    c_s.RegId1 = s.RegId1 and
                                    c_s.Int1 in ( -1,1 ) and
                                    c_s.Time >= s.Time - time_depth.value and
                                    c_s.Time < s.Time and
                                    c_s.RowId != s.RowId
                            )
                        end
                    )allScoresCount,

                    (
                        case
                            when s.Type = 300 then
                            (
                                select
                                    count()
                                from Transactions c_s indexed by Transactions_Type_RegId1_Int1_Time
                                cross join Chain c_sc
                                    on c_sc.TxId = c_s.RowId
                                cross join Transactions c_c indexed by Transactions_HashId
                                    on c_c.HashId = c_s.RegId2 and c_c.RegId1 = c.RegId1
                                cross join First f
                                    on f.TxId = c_c.RowId
                                where
                                    c_s.Type = s.Type and
                                    c_s.RegId1 = s.RegId1 and
                                    c_s.Int1 in ( 4,5 ) and
                                    c_s.Time >= s.Time - time_depth.value and
                                    c_s.Time < s.Time and
                                    c_s.RowId != s.RowId
                            )
                            else
                            (
                                select
                                    count()
                                from Transactions c_s indexed by Transactions_Type_RegId1_Int1_Time
                                cross join Chain c_sc
                                    on c_sc.TxId = c_s.RowId
                                cross join Transactions c_c indexed by Transactions_HashId
                                    on c_c.HashId = c_s.RegId2 and c_c.RegId1 = c.RegId1
                                cross join First f
                                    on f.TxId = c_c.RowId
                                where
                                    c_s.Type = s.Type and
                                    c_s.RegId1 = s.RegId1 and
                                    c_s.Int1 in ( 1 ) and
                                    c_s.Time >= s.Time - time_depth.value and
                                    c_s.Time < s.Time and
                                    c_s.RowId != s.RowId
                            )
                        end
                    )positiveScoresCount

                from height, time_depth

                cross join Registry regSc
                    on regSc.String in (select (select r.String from registry r where r.RowId=t.HashId) from Chain c, Transactions t where c.TxId = t.RowId and c.Height = height.value and t.Type in (300,301))

                cross join Transactions s indexed by Transactions_HashId
                    on s.HashId = regSc.RowId

                -- Score Address
                cross join Transactions sa indexed by Transactions_Type_RegId1_RegId2_RegId3
                    on sa.Type in (100, 170) and sa.RegId1 = s.RegId1
                cross join Chain csa
                    on csa.TxId = sa.RowId
                cross join Last lsa
                    on lsa.TxId = sa.RowId

                -- Content
                cross join Transactions c indexed by Transactions_HashId
                    on c.HashId = s.RegId2
                cross join Chain cc
                    on cc.TxId = c.RowId

                -- Content Address
                cross join Transactions ca indexed by Transactions_Type_RegId1_RegId2_RegId3
                    on ca.Type in (100, 170) and ca.RegId1 = c.RegId1
                cross join Chain cca
                    on cca.TxId = ca.RowId
                cross join Last lca
                    on lca.TxId = ca.RowId
            )sql")
            .Bind(height, scores_time_depth)
            .Select([&](Cursor& cursor) {
                while (cursor.Step())
                {
                    ScoreDataDto data;

                    int contentType, scoreType = -1;
                    cursor.CollectAll(
                        data.ScoreTxHash,
                        scoreType, // Dirty hack
                        data.ScoreTime,
                        data.ScoreValue,
                        data.ScoreAddressId,
                        data.ScoreAddressHash,
                        data.ContentTxHash,
                        contentType, // Dirty hack
                        data.ContentTime,
                        data.ContentId,
                        data.ContentAddressId,
                        data.ContentAddressHash,
                        data.CommentAnswerRootTxHash,
                        data.ScoresAllCount,
                        data.ScoresPositiveCount
                    );

                    data.ContentType = (TxType)contentType;
                    data.ScoreType = (TxType)scoreType;

                    result.emplace(data.ScoreTxHash, make_shared<ScoreDataDto>(data));
                }
            });
        });

        return result;
    }

    // Select referrer for one account
    tuple<bool, string> ConsensusRepository::GetReferrer(const string& address)
    {
        bool result = false;
        string referrer;

        SqlTransaction(__func__, [&]()
        {
            Sql(R"sql(
                with
                    address as (
                        select RowId
                        from Registry
                        where String = ?
                    )

                select
                    (select r.String from Registry r where r.RowId = u.RegId2)
                from address
                cross join Transactions u indexed by Transactions_Type_RegId1_RegId2_RegId3
                    on u.Type in (100, 170) and u.RegId1 = address.RowId and u.RegId2 is not null
                cross join First fu
                    on fu.TxId = u.RowId
            )sql")
            .Bind(address)
            .Select([&](Cursor& cursor) {
                if (cursor.Step())
                {
                    result = true;
                    cursor.CollectAll(referrer);
                }
            });
        });

        return {result, referrer};
    }

    tuple<bool, TxType> ConsensusRepository::GetLastAccountType(const string& address)
    {
        tuple<bool, TxType> result = {false, TxType::ACCOUNT_DELETE};

        SqlTransaction(__func__, [&]()
        {
            Sql(R"sql(
                with
                    addressRegId as (
                        select
                            r.RowId
                        from
                            Registry r
                        where
                            String = ?
                    )
                
                select
                    t.Type
                from
                    addressRegId
                    -- filter registrations & deleting transactions by address
                    cross join Transactions t indexed by Transactions_Type_RegId1_RegId2_RegId3
                        on t.Type in (100, 170) and t.RegId1 = addressRegId.RowId
                    -- filter by chain for exclude mempool
                    cross join Chain c
                        on c.TxId = t.RowId
                    -- filter by Last
                    cross join Last l
                        on l.TxId = t.RowId
            )sql")
            .Bind(address)
            .Select([&](Cursor& cursor)
            {
                if (cursor.Step())
                {
                    int type;
                    cursor.CollectAll(type);
                    result = { true, (TxType)type };
                }
            });
        });

        return result;
    }

    tuple<bool, int64_t> ConsensusRepository::GetTransactionHeight(const string& hash)
    {
        tuple<bool, int64_t> result = {false, 0};

        SqlTransaction(__func__, [&]()
        {
            auto stmt = Sql(R"sql(
                select t.Height
                from Transactions t
                where t.Hash = ?
                  and t.Height is not null
            )sql");

            stmt.Bind(hash);

            // if (stmt.Step())
            //     if (auto [ok, val] = stmt.TryGetColumnInt64(0); ok)
            //         result = { true, val };
        });

        return result;
    }


    // Mempool counts

    int ConsensusRepository::CountMempoolBlocking(const string& address, const string& addressTo)
    {
        int result = 0;

        SqlTransaction(__func__, [&]()
        {
            auto stmt = Sql(R"sql(
                select count()
                from Transactions
                where Type in (305, 306)
                  and Height is null
                  and String1 = ?
                  and (String2 = ? or String3 is not null)
            )sql");

            stmt.Bind(address, addressTo);

            // if (stmt.Step())
            //     stmt.Collect(result);
        });

        return result;
    }
    int ConsensusRepository::CountMempoolSubscribe(const string& address, const string& addressTo)
    {
        int result = 0;

        SqlTransaction(__func__, [&]()
        {
            auto stmt = Sql(R"sql(
                select count(*)
                from Transactions
                where Type in (302, 303, 304)
                  and Height is null
                  and String1 = ?
                  and String2 = ?
            )sql");

            stmt.Bind(address, addressTo);

            // if (stmt.Step())
            //     stmt.Collect(result);
        });

        return result;
    }

    int ConsensusRepository::CountMempoolComment(const string& address)
    {
        int result = 0;

        SqlTransaction(__func__, [&]()
        {
            auto stmt = Sql(R"sql(
                select count(*)
                from Transactions indexed by Transactions_Type_String1_Height_Time_Int1
                where Type in (204)
                  and Height is null
                  and String1 = ?
                  and Hash = String2
            )sql");

            stmt.Bind(address);

            // if (stmt.Step())
            //     stmt.Collect(result);
        });

        return result;
    }
    int ConsensusRepository::CountChainCommentTime(const string& address, int64_t time)
    {
        int result = 0;

        SqlTransaction(__func__, [&]()
        {
            auto stmt = Sql(R"sql(
                select count(*)
                from Transactions indexed by Transactions_Type_String1_Height_Time_Int1
                where Type in (204)
                  and Height is not null
                  and Time >= ?
                  and String1 = ?
                  and Hash = String2
            )sql");

            stmt.Bind(time, address);

            // if (stmt.Step())
            //     stmt.Collect(result);
        });

        return result;
    }
    int ConsensusRepository::CountChainCommentHeight(const string& address, int height)
    {
        int result = 0;

        SqlTransaction(__func__, [&]()
        {
            auto stmt = Sql(R"sql(
                select count(*)
                from Transactions indexed by Transactions_Type_Last_String1_Height_Id
                where Type in (204,205,206)
                  and Last = 1
                  and Height is not null
                  and Height >= ?
                  and String1 = ?
            )sql");

            stmt.Bind(height, address);

            // if (stmt.Step())
            //     stmt.Collect(result);
        });

        return result;
    }

    int ConsensusRepository::CountMempoolComplain(const string& address)
    {
        int result = 0;

        SqlTransaction(__func__, [&]()
        {
            auto stmt = Sql(R"sql(
                select count(*)
                from Transactions
                where Type in (307)
                  and Height is null
                  and String1 = ?
                  and Hash = String2
            )sql");

            stmt.Bind(address);

            // if (stmt.Step())
            //     stmt.Collect(result);
        });

        return result;
    }
    int ConsensusRepository::CountChainComplainTime(const string& address, int64_t time)
    {
        int result = 0;

        SqlTransaction(__func__, [&]()
        {
            auto stmt = Sql(R"sql(
                select count(*)
                from Transactions
                where Type in (307)
                  and Height is not null
                  and Time >= ?
                  and String1 = ?
                  and Hash = String2
            )sql");

            stmt.Bind(time, address);

            // if (stmt.Step())
            //     stmt.Collect(result);
        });

        return result;
    }
    int ConsensusRepository::CountChainComplainHeight(const string& address, int height)
    {
        int result = 0;

        SqlTransaction(__func__, [&]()
        {
            auto stmt = Sql(R"sql(
                select count(*)
                from Transactions
                where Type in (307)
                  and Height is not null
                  and Height >= ?
                  and String1 = ?
                  and Hash = String2
            )sql");

            stmt.Bind(height, address);

            // if (stmt.Step())
            //     stmt.Collect(result);
        });

        return result;
    }

    int ConsensusRepository::CountMempoolPost(const string& address)
    {
        int result = 0;

        SqlTransaction(__func__, [&]()
        {
            auto stmt = Sql(R"sql(
                select count(*)
                from Transactions indexed by Transactions_Type_String1_Height_Time_Int1
                where Type in (200)
                  and Height is null
                  and String1 = ?
                  and Hash = String2
            )sql");

            stmt.Bind(address);

            // if (stmt.Step())
            //     stmt.Collect(result);
        });

        return result;
    }
    int ConsensusRepository::CountChainPostTime(const string& address, int64_t time)
    {
        int result = 0;

        SqlTransaction(__func__, [&]()
        {
            auto stmt = Sql(R"sql(
                select count(*)
                from Transactions indexed by Transactions_Type_String1_Height_Time_Int1
                where Type in (200)
                  and Height is not null
                  and String1 = ?
                  and Time >= ?
                  and Hash = String2
            )sql");

            stmt.Bind(address, time);

            // if (stmt.Step())
            //     stmt.Collect(result);
        });

        return result;
    }
    int ConsensusRepository::CountChainPostHeight(const string& address, int height)
    {
        int result = 0;

        SqlTransaction(__func__, [&]()
        {
            auto stmt = Sql(R"sql(
                select count()
                from Transactions indexed by Transactions_Type_String1_String2_Height
                where Type in (200)
                  and String1 = ?
                  and Height >= ?
                  and Hash = String2
            )sql");

            stmt.Bind(address, height);

            // if (stmt.Step())
            //     stmt.Collect(result);
        });

        return result;
    }

    int ConsensusRepository::CountMempoolVideo(const string& address)
    {
        int result = 0;

        SqlTransaction(__func__, [&]()
        {
            auto stmt = Sql(R"sql(
                select count(*)
                from Transactions indexed by Transactions_Type_String1_Height_Time_Int1
                where Type in (201)
                  and Height is null
                  and String1 = ?
                  and Hash = String2
            )sql");

            stmt.Bind(address);

            // if (stmt.Step())
            //     stmt.Collect(result);
        });

        return result;
    }
    int ConsensusRepository::CountChainVideo(const string& address, int height)
    {
        int result = 0;

        SqlTransaction(__func__, [&]()
        {
            auto stmt = Sql(R"sql(
                select count(*)
                from Transactions indexed by Transactions_Type_String1_Height_Time_Int1
                where Type in (201)
                  and String1 = ?
                  and Height >= ?
                  and Hash = String2
            )sql");

            stmt.Bind(address, height);

            // if (stmt.Step())
            //     stmt.Collect(result);
        });

        return result;
    }

    int ConsensusRepository::CountMempoolArticle(const string& address)
    {
        int result = 0;

        SqlTransaction(__func__, [&]()
        {
            auto stmt = Sql(R"sql(
                select count(*)
                from Transactions indexed by Transactions_Type_String1_Height_Time_Int1
                where Type in (202)
                  and Height is null
                  and String1 = ?
                  and Hash = String2
            )sql");

            stmt.Bind(address);

            // if (stmt.Step())
            //     stmt.Collect(result);
        });

        return result;
    }
    int ConsensusRepository::CountChainArticle(const string& address, int height)
    {
        int result = 0;

        SqlTransaction(__func__, [&]()
        {
            auto stmt = Sql(R"sql(
                select count(*)
                from Transactions indexed by Transactions_Type_String1_Height_Time_Int1
                where Type in (202)
                  and String1 = ?
                  and Height >= ?
                  and Hash = String2
            )sql");

            stmt.Bind(address, height);

            // if (stmt.Step())
            //     stmt.Collect(result);
        });

        return result;
    }

    int ConsensusRepository::CountMempoolStream(const string& address)
    {
        int result = 0;

        SqlTransaction(__func__, [&]()
        {
            auto stmt = Sql(R"sql(
                select count(*)
                from Transactions indexed by Transactions_Type_String1_Height_Time_Int1
                where Type in (209)
                  and Height is null
                  and String1 = ?
                  and Hash = String2
            )sql");

            stmt.Bind(address);

            // if (stmt.Step())
            //     stmt.Collect(result);
        });

        return result;
    }
    int ConsensusRepository::CountChainStream(const string& address, int height)
    {
        int result = 0;

        SqlTransaction(__func__, [&]()
        {
            auto stmt = Sql(R"sql(
                select count(*)
                from Transactions indexed by Transactions_Type_String1_Height_Time_Int1
                where Type in (209)
                  and String1 = ?
                  and Height >= ?
                  and Hash = String2
            )sql");

            stmt.Bind(address, height);

            // if (stmt.Step())
            //     stmt.Collect(result);
        });

        return result;
    }

    int ConsensusRepository::CountMempoolAudio(const string& address)
    {
        int result = 0;

        SqlTransaction(__func__, [&]()
        {
            auto stmt = Sql(R"sql(
                select count(*)
                from Transactions indexed by Transactions_Type_String1_Height_Time_Int1
                where Type in (210)
                  and Height is null
                  and String1 = ?
                  and Hash = String2
            )sql");

            stmt.Bind(address);

            // if (stmt.Step())
            //     stmt.Collect(result);
        });

        return result;
    }
    int ConsensusRepository::CountChainAudio(const string& address, int height)
    {
        int result = 0;

        SqlTransaction(__func__, [&]()
        {
            auto stmt = Sql(R"sql(
                select count(*)
                from Transactions indexed by Transactions_Type_String1_Height_Time_Int1
                where Type in (210)
                  and String1 = ?
                  and Height >= ?
                  and Hash = String2
            )sql");

            stmt.Bind(address, height);

            // if (stmt.Step())
            //     stmt.Collect(result);
        });

        return result;
    }

    int ConsensusRepository::CountMempoolCollection(const string& address)
    {
        int result = 0;

        SqlTransaction(__func__, [&]()
        {
            Sql(R"sql(
                select count(*)
                from Transactions indexed by Transactions_Type_String1_Height_Time_Int1
                where Type in (220)
                  and Height is null
                  and String1 = ?
                  and Hash = String2
            )sql")
            .Bind(address)
            .Select([&](Cursor& cursor) {
                if (cursor.Step())
                    cursor.CollectAll(result);
            });
        });

        return result;
    }
    int ConsensusRepository::CountChainCollection(const string& address, int height)
    {
        int result = 0;

        SqlTransaction(__func__, [&]()
        {
            Sql(R"sql(
                select count(*)
                from Transactions indexed by Transactions_Type_String1_Height_Time_Int1
                where Type in (220)
                  and String1 = ?
                  and Height >= ?
                  and Hash = String2
            )sql")
            .Bind(address, height)
            .Select([&](Cursor& cursor) {
                if (cursor.Step())
                    cursor.CollectAll(result);
            });
        });

        return result;
    }

    int ConsensusRepository::CountMempoolScoreComment(const string& address)
    {
        int result = 0;

        SqlTransaction(__func__, [&]()
        {
            auto stmt = Sql(R"sql(
                select count(*)
                from Transactions indexed by Transactions_Type_String1_Height_Time_Int1
                where Type in (301)
                  and Height is null
                  and String1 = ?
            )sql");

            stmt.Bind(address);

            // if (stmt.Step())
            //     stmt.Collect(result);
        });

        return result;
    }
    int ConsensusRepository::CountChainScoreCommentTime(const string& address, int64_t time)
    {
        int result = 0;

        SqlTransaction(__func__, [&]()
        {
            auto stmt = Sql(R"sql(
                select count(*)
                from Transactions indexed by Transactions_Type_String1_Height_Time_Int1
                where Type in (301)
                  and Height is not null
                  and String1 = ?
                  and Time >= ?
            )sql");

            stmt.Bind(address, time);

            // if (stmt.Step())
            //     stmt.Collect(result);
        });

        return result;
    }
    int ConsensusRepository::CountChainScoreCommentHeight(const string& address, int height)
    {
        int result = 0;

        SqlTransaction(__func__, [&]()
        {
            auto stmt = Sql(R"sql(
                select count(*)
                from Transactions indexed by Transactions_Type_String1_Height_Time_Int1
                where Type in (301)
                  and Height is not null
                  and Height >= ?
                  and String1 = ?
            )sql");

            stmt.Bind(height, address);

            // if (stmt.Step())
            //     stmt.Collect(result);
        });

        return result;
    }

    int ConsensusRepository::CountMempoolScoreContent(const string& address)
    {
        int result = 0;

        SqlTransaction(__func__, [&]()
        {
            auto stmt = Sql(R"sql(
                select count(*)
                from Transactions indexed by Transactions_Type_String1_Height_Time_Int1
                where Type in (300)
                  and Height is null
                  and String1 = ?
            )sql");

            stmt.Bind(address);

            // if (stmt.Step())
            //     stmt.Collect(result);
        });

        return result;
    }
    int ConsensusRepository::CountChainScoreContentTime(const string& address, int64_t time)
    {
        int result = 0;

        SqlTransaction(__func__, [&]()
        {
            auto stmt = Sql(R"sql(
                select count(*)
                from Transactions indexed by Transactions_Type_String1_Height_Time_Int1
                where Type in (300)
                  and Height is not null
                  and String1 = ?
                  and Time >= ?
            )sql");

            stmt.Bind(address, time);

            // if (stmt.Step())
            //     stmt.Collect(result);
        });

        return result;
    }
    int ConsensusRepository::CountChainScoreContentHeight(const string& address, int height)
    {
        int result = 0;

        SqlTransaction(__func__, [&]()
        {
            auto stmt = Sql(R"sql(
                select count(*)
                from Transactions indexed by Transactions_Type_String1_Height_Time_Int1
                where Type in (300)
                  and Height is not null
                  and Height >= ?
                  and String1 = ?
            )sql");

            stmt.Bind(height, address);

            // if (stmt.Step())
            //     stmt.Collect(result);
        });

        return result;
    }

    int ConsensusRepository::CountMempoolAccountSetting(const string& address)
    {
        int result = 0;

        SqlTransaction(__func__, [&]()
        {
            auto stmt = Sql(R"sql(
                select count(*)
                from Transactions
                where Type in (103)
                  and Height is null
                  and String1 = ?
            )sql");

            stmt.Bind(address);

            // if (stmt.Step())
            //     stmt.Collect(result);
        });

        return result;
    }
    int ConsensusRepository::CountChainAccountSetting(const string& address, int height)
    {
        int result = 0;

        SqlTransaction(__func__, [&]()
        {
            auto stmt = Sql(R"sql(
                select count(*)
                from Transactions indexed by Transactions_Type_String1_Height_Time_Int1
                where Type in (103)
                  and Height is not null
                  and Height >= ?
                  and String1 = ?
            )sql");

            stmt.Bind(address, height);

            // if (stmt.Step())
            //     stmt.Collect(result);
        });

        return result;
    }

    int ConsensusRepository::CountChainAccount(TxType txType, const string& address, int height)
    {
        int result = 0;

        SqlTransaction(__func__, [&]()
        {
            Sql(R"sql(
                with
                    address as (
                        select RowId
                        from Registry
                        where String = ?
                    )

                select count()
                from address
                cross join Transactions t indexed by Transactions_Type_RegId1_RegId2_RegId3
                    on t.Type = ? and t.RegId1 = address.RowId
                cross join Chain c
                    on c.TxId = t.RowId
                cross join Chain cc indexed by Chain_Uid_Height
                    on cc.Uid = c.Uid and cc.Height >= ? and cc.TxId = c.TxId
            )sql")
            .Bind(address, (int)txType, height)
            .Select([&](Cursor& cursor) {
                if (cursor.Step())
                    cursor.CollectAll(result);
            });
        });

        return result;
    }

    // EDITS

    int ConsensusRepository::CountMempoolCommentEdit(const string& address, const string& rootTxHash)
    {
        int result = 0;

        SqlTransaction(__func__, [&]()
        {
            auto stmt = Sql(R"sql(
                select count(*)
                from Transactions indexed by Transactions_Type_String1_String2_Height
                where Type in (204,205,206)
                  and Height is null
                  and String1 = ?
                  and String2 = ?
            )sql");

            stmt.Bind(address, rootTxHash);

            // if (stmt.Step())
            //     if (auto[ok, value] = stmt.TryGetColumnInt(0); ok)
            //         result = value;
        });

        return result;
    }
    int ConsensusRepository::CountChainCommentEdit(const string& address, const string& rootTxHash)
    {
        int result = 0;

        SqlTransaction(__func__, [&]()
        {
            auto stmt = Sql(R"sql(
                select count(*)
                from Transactions indexed by Transactions_Type_String1_String2_Height
                where Type in (204,205,206)
                  and Height is not null
                  and Hash != String2
                  and String1 = ?
                  and String2 = ?
            )sql");

            stmt.Bind(address, rootTxHash);

            // if (stmt.Step())
            //     stmt.Collect(result);
        });

        return result;
    }

    int ConsensusRepository::CountMempoolPostEdit(const string& address, const string& rootTxHash)
    {
        int result = 0;

        SqlTransaction(__func__, [&]()
        {
            auto stmt = Sql(R"sql(
                select count(*)
                from Transactions indexed by Transactions_Type_String1_String2_Height
                where Type in (200,207)
                  and Height is null
                  and String1 = ?
                  and String2 = ?
            )sql");

            stmt.Bind(address, rootTxHash);

            // if (stmt.Step())
            //     stmt.Collect(result);
        });

        return result;
    }
    int ConsensusRepository::CountChainPostEdit(const string& address, const string& rootTxHash)
    {
        int result = 0;

        SqlTransaction(__func__, [&]()
        {
            auto stmt = Sql(R"sql(
                select count(*)
                from Transactions indexed by Transactions_Type_String1_String2_Height
                where Type in (200)
                  and Height is not null
                  and Hash != String2
                  and String1 = ?
                  and String2 = ?
            )sql");

            stmt.Bind(address, rootTxHash);

            // if (stmt.Step())
            //     stmt.Collect(result);
        });

        return result;
    }

    int ConsensusRepository::CountMempoolVideoEdit(const string& address, const string& rootTxHash)
    {
        int result = 0;

        SqlTransaction(__func__, [&]()
        {
            auto stmt = Sql(R"sql(
                select count(*)
                from Transactions indexed by Transactions_Type_String1_String2_Height
                where Type in (201,207)
                  and Height is null
                  and String1 = ?
                  and String2 = ?
            )sql");

            stmt.Bind(address, rootTxHash);

            // if (stmt.Step())
            //     stmt.Collect(result);
        });

        return result;
    }
    int ConsensusRepository::CountChainVideoEdit(const string& address, const string& rootTxHash)
    {
        int result = 0;

        SqlTransaction(__func__, [&]()
        {
            auto stmt = Sql(R"sql(
                select count(*)
                from Transactions indexed by Transactions_Type_String1_String2_Height
                where Type in (201)
                  and Height is not null
                  and Hash != String2
                  and String1 = ?
                  and String2 = ?
            )sql");

            stmt.Bind(address, rootTxHash);

            // if (stmt.Step())
            //     stmt.Collect(result);
        });

        return result;
    }

    int ConsensusRepository::CountMempoolArticleEdit(const string& address, const string& rootTxHash)
    {
        int result = 0;

        SqlTransaction(__func__, [&]()
        {
            auto stmt = Sql(R"sql(
                select count(*)
                from Transactions indexed by Transactions_Type_String1_String2_Height
                where Type in (202,207)
                  and Height is null
                  and String1 = ?
                  and String2 = ?
            )sql");

            stmt.Bind(address, rootTxHash);

            // if (stmt.Step())
            //     stmt.Collect(result);
        });

        return result;
    }
    int ConsensusRepository::CountChainArticleEdit(const string& address, const string& rootTxHash)
    {
        int result = 0;

        SqlTransaction(__func__, [&]()
        {
            auto stmt = Sql(R"sql(
                select count(*)
                from Transactions indexed by Transactions_Type_String1_String2_Height
                where Type in (202)
                  and Height is not null
                  and Hash != String2
                  and String1 = ?
                  and String2 = ?
            )sql");

            stmt.Bind(address, rootTxHash);

            // if (stmt.Step())
            //     stmt.Collect(result);
        });

        return result;
    }

    int ConsensusRepository::CountMempoolStreamEdit(const string& address, const string& rootTxHash)
    {
        int result = 0;

        SqlTransaction(__func__, [&]()
        {
            auto stmt = Sql(R"sql(
                select count(*)
                from Transactions indexed by Transactions_Type_String1_String2_Height
                where Type in (209,207)
                  and Height is null
                  and String1 = ?
                  and String2 = ?
            )sql");

            stmt.Bind(address, rootTxHash);

            // if (stmt.Step())
            //     stmt.Collect(result);
        });

        return result;
    }
    int ConsensusRepository::CountChainStreamEdit(const string& address, const string& rootTxHash)
    {
        int result = 0;

        SqlTransaction(__func__, [&]()
        {
            auto stmt = Sql(R"sql(
                select count(*)
                from Transactions indexed by Transactions_Type_String1_String2_Height
                where Type in (209)
                  and Height is not null
                  and Hash != String2
                  and String1 = ?
                  and String2 = ?
            )sql");

            stmt.Bind(address, rootTxHash);

            // if (stmt.Step())
            //     stmt.Collect(result);
        });

        return result;
    }

    int ConsensusRepository::CountMempoolAudioEdit(const string& address, const string& rootTxHash)
    {
        int result = 0;

        SqlTransaction(__func__, [&]()
        {
            auto stmt = Sql(R"sql(
                select count(*)
                from Transactions indexed by Transactions_Type_String1_String2_Height
                where Type in (210,207)
                  and Height is null
                  and String1 = ?
                  and String2 = ?
            )sql");

            stmt.Bind(address, rootTxHash);

            // if (stmt.Step())
            //     stmt.Collect(result);
        });

        return result;
    }
    int ConsensusRepository::CountChainAudioEdit(const string& address, const string& rootTxHash)
    {
        int result = 0;

        SqlTransaction(__func__, [&]()
        {
            auto stmt = Sql(R"sql(
                select count(*)
                from Transactions indexed by Transactions_Type_String1_String2_Height
                where Type in (210)
                  and Height is not null
                  and Hash != String2
                  and String1 = ?
                  and String2 = ?
            )sql");

            stmt.Bind(address, rootTxHash);

            // if (stmt.Step())
            //     stmt.Collect(result);
        });

        return result;
    }

    int ConsensusRepository::CountMempoolCollectionEdit(const string& address, const string& rootTxHash)
    {
        int result = 0;

        SqlTransaction(__func__, [&]()
        {
            Sql(R"sql(
                select count(*)
                from Transactions indexed by Transactions_Type_String1_String2_Height
                where Type in (220,207)
                  and Height is null
                  and String1 = ?
                  and String2 = ?
            )sql")
            .Bind(address, rootTxHash)
            .Select([&](Cursor& cursor) {
                if (cursor.Step())
                    cursor.CollectAll(result);
            });
        });

        return result;
    }
    int ConsensusRepository::CountChainCollectionEdit(const string& address, const string& rootTxHash, const int& nHeight, const int& depth)
    {
        int result = 0;

        SqlTransaction(__func__, [&]()
        {
            Sql(R"sql(
                select count(*)
                from Transactions indexed by Transactions_Type_String1_String2_Height
                where Type in (220)
                  and Height <= ?
                  and Height > ?
                  and Hash != String2
                  and String1 = ?
                  and String2 = ?
            )sql")
            .Bind(nHeight, nHeight - depth, address, rootTxHash)
            .Select([&](Cursor& cursor) {
                if (cursor.Step())
                    cursor.CollectAll(result);
            });
        });

        return result;
    }

    int ConsensusRepository::CountMempoolContentDelete(const string& address, const string& rootTxHash)
    {
        int result = 0;

        SqlTransaction(__func__, [&]()
        {
            auto stmt = Sql(R"sql(
                select count()
                from Transactions indexed by Transactions_Type_String1_String2_Height
                where Type in (200,201,202,209,210,211,220,207)
                  and Height is null
                  and String1 = ?
                  and String2 = ?
            )sql");

            stmt.Bind(address, rootTxHash);

            // if (stmt.Step())
            //     stmt.Collect(result);
        });

        return result;
    }

    /* MODERATION */

    int ConsensusRepository::CountModerationFlag(const string& address, int height, bool includeMempool)
    {
        int result = 0;
        string whereMempool = includeMempool ? " or Height is null " : "";

        SqlTransaction(__func__, [&]()
        {
            auto stmt = Sql(R"sql(
                select count()
                from Transactions indexed by Transactions_Type_String1_Height_Time_Int1
                where Type = 410
                  and String1 = ?
                  and ( Height >= ? )sql" + whereMempool + R"sql( )
            )sql");

            stmt.Bind(address, height);

            // if (stmt.Step())
            //     stmt.Collect(result);
        });

        return result;
    }
    int ConsensusRepository::CountModerationFlag(const string& address, const string& addressTo, bool includeMempool)
    {
        int result = 0;
        string whereMempool = includeMempool ? " or Height is null " : "";

        SqlTransaction(__func__, [&]()
        {
            auto stmt = Sql(R"sql(
                select count()
                from Transactions indexed by Transactions_Type_String1_String3_Height
                where Type = 410
                  and String1 = ?
                  and String3 = ?
                  and ( Height > 0 )sql" + whereMempool + R"sql( )
            )sql");

            stmt.Bind(address, addressTo);

            // if (stmt.Step())
            //     stmt.Collect(result);
        });

        return result;
    }

    bool ConsensusRepository::AllowJuryModerate(const string& address, const string& flagTxHash)
    {
        bool result = false;

        SqlTransaction(__func__, [&]()
        {
            Sql(R"sql(
                select 1
                from JuryModerators jm
                where 
                    jm.FlagRowId = (
                        select f.ROWID
                        from Transactions f indexed by sqlite_autoindex_Transactions_1
                        where f.Hash = ?
                    ) and
                    jm.AccountId = (
                        select u.Id
                        from Transactions u indexed by Transactions_Type_Last_String1_Height_Id
                        where u.Type in (100) and u.Last = 1 and u.Height > 0 and u.String1 = ?
                    )
            )sql")
            .Bind(flagTxHash, address)
            .Select([&](Cursor& cursor) {
                if (cursor.Step())
                    cursor.CollectAll(result);
            });
        });

        return result;
    }


}