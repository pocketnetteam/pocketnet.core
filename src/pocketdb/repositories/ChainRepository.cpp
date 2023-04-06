// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include "pocketdb/repositories/ChainRepository.h"

namespace PocketDb
{
    void ChainRepository::IndexBlock(const string& blockHash, int height, vector<TransactionIndexingInfo>& txs)
    {
        SqlTransaction(__func__, [&]()
        {
            int64_t nTime1 = GetTimeMicros();

            IndexBlockData(blockHash);

            // Each transaction is processed individually
            for (const auto& txInfo : txs)
            {
                auto[id, lastTxId] = IndexSocial(txInfo);

                // Remove current last record for tx
                if (lastTxId)
                {
                    Sql(R"sql(
                        delete from Last
                        where TxId = ?
                    )sql")
                    .Bind(*lastTxId)
                    .Run();
                }

                // if 'id' means that tx is social and requires last and fist to be set.
                // if not 'lastTxId' means that this is first tx for this id
                if (id && !lastTxId)
                {
                    SetFirst(txInfo.Hash);
                }

                // All transactions must have a blockHash & height relation
                InsertTransactionChainData(
                    blockHash,
                    txInfo.BlockNumber,
                    height,
                    txInfo.Hash,
                    id
                );
            }

            // After set height and mark inputs as spent we need recalculcate balances
            IndexBalances(height);

            int64_t nTime2 = GetTimeMicros();

            LogPrint(BCLog::BENCH, "    - IndexBlock: %.2fms\n", 0.001 * double(nTime2 - nTime1));
        });
    }

    tuple<bool, bool> ChainRepository::ExistsBlock(const string& blockHash, int height)
    {
        int exists = 0;
        int last = 1;

        SqlTransaction(__func__, [&]()
        {
            Sql(R"sql(
                select
                    ifnull((
                        select
                            1
                        from
                            Chain c indexed by Chain_Height_BlockId
                        where
                            c.Height = ? and
                            c.BlockId = (
                                select
                                    r.RowId
                                from
                                    Registry r
                                where
                                    r.String = ?
                            )
                        limit 1
                    ), 0),
                    ifnull((
                        select
                            1
                        from
                            Chain c indexed by Chain_Height_BlockId
                        where
                            c.Height = ?
                        limit 1
                    ), 0)
            )sql")
            .Bind(height, blockHash, height + 1)
            .Select([&](Cursor& cursor) {
                if (cursor.Step())
                    cursor.CollectAll(exists, last);
            });
        });

        return { exists == 1, last == 0 };
    }

    void ChainRepository::IndexBlockData(const string& blockHash)
    {
        Sql(R"sql(
            insert or ignore into Registry (String)
            values (?)
        )sql")
        .Bind(blockHash)
        .Run();
    }

    void ChainRepository::InsertTransactionChainData(const string& blockHash, int blockNumber, int height, const string& txHash, const optional<int64_t>& id)
    {
        Sql(R"sql(
            with
                block as (
                    select
                        RowId
                    from
                        Registry
                    where
                        String = ?
                ),
                tx as (
                    select
                        t.RowId
                    from
                        vTx t
                    where
                        t.Hash = ?
                )
            insert or fail into Chain
                (TxId, BlockId, BlockNum, Height, Uid)
            select
                tx.RowId, block.RowId, ?, ?, ?
            from
                tx,
                block
        )sql")
        .Bind(blockHash, txHash, blockNumber, height, id)
        .Run();

        if (id.has_value())
        {
            Sql(R"sql(
                insert or ignore into Last
                    (TxId)
                select
                    t.RowId
                from
                    vTx t
                where
                    t.Hash = ?
            )sql")
            .Bind(txHash)
            .Run();
        }
    }

    void ChainRepository::SetFirst(const string& txHash)
    {
        Sql(R"sql(
            insert or ignore into First
                (TxId)
            select
                t.RowId
            from
                vTx t
            where
                t.Hash = ?
        )sql")
        .Bind(txHash)
        .Run();
    }

    void ChainRepository::IndexBalances(int height)
    {
        // Generate new balance records
        Sql(R"sql(
            with
                height as (
                    select ? as value
                ),
                outs as (
                    select
                        o.TxId,
                        o.Number,
                        o.AddressId,
                        (+o.Value)val
                    from
                        height,
                        Chain c indexed by Chain_Height_BlockId
                        cross join TxOutputs o indexed by TxOutputs_TxId_Number_AddressId
                            on o.TxId = c.TxId
                    where
                        c.Height = height.value

                    union

                    select
                        o.TxId,
                        o.Number,
                        o.AddressId,
                        (-o.Value)val
                    from
                        height,
                        Chain ci indexed by Chain_Height_BlockId
                        cross join TxInputs i indexed by TxInputs_SpentTxId_TxId_Number
                            on i.SpentTxId = ci.TxId
                        cross join TxOutputs o indexed by TxOutputs_TxId_Number_AddressId
                            on o.TxId = i.TxId and o.Number = i.Number
                    where
                        ci.Height = height.value
                ),
                saldo as (
                    select
                        outs.AddressId,
                        sum(outs.val)Amount
                    from
                        outs
                    group by
                        outs.AddressId
                )

            replace into Balances (AddressId, Value)
            select
                saldo.AddressId,
                ifnull(b.Value, 0) + saldo.Amount
            from
                saldo
                left join Balances b
                    on b.AddressId = saldo.AddressId
            where
                saldo.Amount != 0
        )sql")
        .Bind(height)
        .Run();
    }

    pair<optional<int64_t>, optional<int64_t>> ChainRepository::IndexSocial(const TransactionIndexingInfo& txInfo)
    {
        optional<int64_t> id;
        optional<int64_t> lastTxId;
        string sql;

        // Account and Content must have unique ID
        // Also all edited transactions must have Last=(0/1) field
        if (txInfo.IsAccount())
            sql = IndexAccount();
        else if (txInfo.IsAccountSetting())
            sql = IndexAccountSetting();
        else if (txInfo.IsContent())
            sql = IndexContent();
        else if (txInfo.IsComment())
            sql = IndexComment();
        else if (txInfo.IsBlocking())
            sql = IndexBlocking();
        else if (txInfo.IsSubscribe())
            sql = IndexSubscribe();
        else
            return {id, lastTxId};

        Sql(sql)
        .Bind(txInfo.Hash)
        .Select([&](Cursor& cursor) {
            if (cursor.Step())
                cursor.CollectAll(id, lastTxId);
        });

        return {id, lastTxId};
    }

    string ChainRepository::IndexAccount()
    {
        return R"sql(
            with
                l as (
                    select
                        b.RowId
                    from
                        Transactions a -- primary key
                        join Transactions b indexed by Transactions_Type_RegId1_RegId2_RegId3
                            on b.Type in (100, 170) and 
                                b.RegId1 = a.RegId1
                        join Last l -- primary key
                            on l.TxId = b.RowId
                    where
                        a.Type in (100, 170) and
                        a.RowId = (
                            select t.RowId
                            from vTx t
                            where t.Hash = ?
                        )
                    limit 1
                )
            select
                ifnull(
                    (
                        select
                            c.Uid
                        from
                            Chain c, -- primary key
                            l
                        where
                            c.TxId = l.RowId
                    ),
                    ifnull(
                        -- new record
                        (
                            select
                                max(c.Uid) + 1
                            from
                                Chain c indexed by Chain_Uid_Height
                        ),
                        -- for first record
                        (
                            select 0
                        )
                    )
                ),
                (
                    select l.RowId
                    from l
                )
        )sql";
    }

    string ChainRepository::IndexAccountSetting()
    {
        return R"sql(
            with
                l as (
                    select
                        b.RowId
                    from
                        Transactions a -- primary key
                        join Transactions b indexed by Transactions_Type_RegId1_RegId2_RegId3
                            on b.Type in (103)
                            and b.RegId1 = a.RegId1
                        join Last l -- primary key
                            on l.TxId = b.RowId
                    where
                        a.Type in (103) and
                        a.RowId = (
                            select t.RowId
                            from vTx t
                            where t.Hash = ?
                        )
                    limit 1
                )
            select
                ifnull(
                    (
                        select
                            c.Uid
                        from
                            Chain c, -- primary key
                            l
                        where
                            c.TxId = l.RowId
                    ),
                    ifnull(
                        -- new record
                        (
                            select
                                max(c.Uid) + 1
                            from
                                Chain c indexed by Chain_Uid_Height
                        ),
                        -- for first record
                        (
                            select 0
                        )
                    )
                ),
                (
                    select l.RowId
                    from l
                )
        )sql";
    }

    string ChainRepository::IndexContent()
    {
        return R"sql(
            with
                l as (
                    select
                        b.RowId
                    from
                        Transactions a -- primary key
                        join Transactions b indexed by Transactions_Type_RegId1_RegId2_RegId3
                            on b.Type in (200, 201, 202, 209, 210, 220, 207) and
                            b.RegId1 = a.RegId1 and
                            b.RegId2 = a.RegId2
                        join Last l -- primary key
                            on l.TxId = b.RowId
                    where
                        a.Type in (200, 201, 202, 209, 210, 220, 207) and
                        a.RowId = (
                            select t.RowId
                            from vTx t
                            where t.Hash = ?
                        )
                    limit 1
                )
            select
                ifnull(
                    (
                        select
                            c.Uid
                        from
                            Chain c, -- primary key
                            l
                        where
                            c.TxId = l.RowId
                    ),
                    ifnull(
                        -- new record
                        (
                            select
                                max(c.Uid) + 1
                            from
                                Chain c indexed by Chain_Uid_Height
                        ),
                        -- for first record
                        (
                            select 0
                        )
                    )
                ),
                (
                    select l.RowId
                    from l
                )
        )sql";
    }

    string ChainRepository::IndexComment()
    {
        return R"sql(
            with
                l as (
                    select
                        b.RowId
                    from
                        Transactions a -- primary key
                        join Transactions b indexed by Transactions_Type_RegId1_RegId2_RegId3
                            on b.Type in (204, 205, 206) and
                            b.RegId1 = a.RegId1 and
                            b.RegId2 = a.RegId2
                        join Last l -- primary key
                            on l.TxId = b.RowId
                    where
                        a.Type in (204, 205, 206) and
                        a.RowId = (
                            select t.RowId
                            from vTx t
                            where t.Hash = ?
                        )
                    limit 1
                )
            select
                ifnull(
                    (
                        select
                            c.Uid
                        from
                            Chain c, -- primary key
                            l
                        where
                            c.TxId = l.RowId
                    ),
                    ifnull(
                        -- new record
                        (
                            select
                                max(c.Uid) + 1
                            from
                                Chain c indexed by Chain_Uid_Height
                        ),
                        -- for first record
                        (
                            select 0
                        )
                    )
                ),
                (
                    select l.RowId
                    from l
                )
        )sql";
    }

    string ChainRepository::IndexBlocking()
    {
        return R"sql(
            with
                l as (
                    select
                        b.RowId
                    from
                        Transactions a -- primary key
                        join Transactions b indexed by Transactions_Type_RegId1_RegId2_RegId3
                            on b.Type in (305, 306) and
                            b.RegId1 = a.RegId1 and
                            ifnull(b.RegId2, -1) = ifnull(a.RegId2, -1) and
                            ifnull(b.RegId3, -1) = ifnull(a.RegId3, -1)
                        join Last l -- primary key
                            on l.TxId = b.RowId
                    where
                        a.Type in (305, 306) and
                        a.RowId = (
                            select t.RowId
                            from vTx t
                            where t.Hash = ?
                        )
                    limit 1
                )
            select
                ifnull(
                    (
                        select
                            c.Uid
                        from
                            Chain c, -- primary key
                            l
                        where
                            c.TxId = l.RowId
                    ),
                    ifnull(
                        -- new record
                        (
                            select
                                max(c.Uid) + 1
                            from
                                Chain c indexed by Chain_Uid_Height
                        ),
                        -- for first record
                        (
                            select 0
                        )
                    )
                ),
                (
                    select l.RowId
                    from l
                )
        )sql";

        // TODO (optimization): bad bad bad!!!
        // SqlTransaction(__func__, [&]()
        // {
        //     auto insListStmt = Sql(R"sql(
        //         insert into BlockingLists (IdSource, IdTarget)
        //         select
        //         usc.Id,
        //         utc.Id
        //         from Transactions b -- TODO (optimization): index
        //         join Transactions us -- TODO (optimization): index
        //         on us.Type in (100, 170) and us.Int1 = b.Int1
        //         join Chain usc -- TODO (optimization): index
        //         on usc.TxId = us.Id
        //             and usc.Last = 1
        //         join Transactions ut -- TODO (optimization): index
        //         on ut.Type in (100, 170)
        //             and ut.Int1 in (select b.Int2 union select cast(value as int) from json_each(b.Int3))
        //         join Chain utc -- TODO (optimization): index
        //         on utc.TxId = ut.Id
        //             and utc.Last = 1
        //         where b.Type in (305) and b.Id = (select RowId from Transactions where HashId = (select RowId from Registry where String = ?))
        //             and not exists (select 1 from BlockingLists bl where bl.IdSource = usc.Id and bl.IdTarget = utc.Id)
        //     )sql");
        //     insListStmt->Bind(txHash);
        //     TryStepStatement(insListStmt);

        //     auto delListStmt = Sql(R"sql(
        //         delete from BlockingLists
        //         where exists
        //         (select
        //         1
        //         from Transactions b -- TODO (optimization): index
        //         join Transactions us -- TODO (optimization): index
        //         on us.Type in (100, 170) and us.Int1 = b.Int1
        //         join Chain usc -- TODO (optimization): index
        //         on usc.TxId = us.Id
        //             and usc.Last = 1
        //             and usc.Id = BlockingLists.IdSource
        //         join Transactions ut -- TODO (optimization): index
        //         on ut.Type in (100, 170) and ut.Int1 = b.Int2
        //         join Chain utc -- TODO (optimization): index
        //         on utc.TxId = ut.Id
        //             and utc.Last = 1
        //             and utc.Id = BlockingLists.IdTarget
        //         where b.Type in (306) and b.Id = (select RowId from Transactions where HashId = (select RowId from Registry where String = ?))
        //         )
        //     )sql");
        //     delListStmt->Bind(txHash);
        //     TryStepStatement(delListStmt);
        // });
    }

    string ChainRepository::IndexSubscribe()
    {
        return R"sql(
            with
                l as (
                    select
                        b.RowId
                    from
                        Transactions a -- primary key
                        join Transactions b indexed by Transactions_Type_RegId1_RegId2_RegId3
                            on b.Type in (302, 303, 304) and
                            b.RegId1 = a.RegId1 and
                            b.RegId2 = a.RegId2
                        join Last l
                            on l.TxId = b.RowId
                    where
                        a.Type in (302, 303, 304) and
                        a.RowId = (
                            select t.RowId
                            from vTx t
                            where t.Hash = ?
                        )
                    limit 1
                )
            select
                ifnull(
                    (
                        select
                            c.Uid
                        from
                            Chain c, -- primary key
                            l
                        where
                            c.TxId = l.RowId
                    ),
                    ifnull(
                        -- new record
                        (
                            select
                                max(c.Uid) + 1
                            from
                                Chain c indexed by Chain_Uid_Height
                        ),
                        -- for first record
                        (
                            select 0
                        )
                    )
                ),
                (
                    select l.RowId
                    from l
                )
        )sql";
    }

    string ChainRepository::IndexAccountBarteron()
    {
        return R"sql(
            with
                l as (
                    select
                        b.RowId
                    from
                        Transactions a -- primary key
                        join Transactions b indexed by Transactions_Type_RegId1_RegId2_RegId3
                            on b.Type = 104 and
                            b.RegId1 = a.RegId1
                        join Last l
                            on l.TxId = b.RowId
                    where
                        a.Type = 104 and
                        a.RowId = (
                            select t.RowId
                            from vTx t
                            where t.Hash = ?
                        )
                    limit 1
                )
            select
                ifnull(
                    (
                        select
                            c.Uid
                        from
                            Chain c, -- primary key
                            l
                        where
                            c.TxId = l.RowId
                    ),
                    ifnull(
                        -- new record
                        (
                            select
                                max(c.Uid) + 1
                            from
                                Chain c indexed by Chain_Uid_Height
                        ),
                        -- for first record
                        (
                            select 0
                        )
                    )
                ),
                (
                    select l.RowId
                    from l
                )
        )sql";
    }
    
    void ChainRepository::IndexModerationJury(const string& flagTxHash, int flagsDepth, int flagsMinCount, int juryModeratorsCount)
    {
        // TODO (optimization): update to new db
        SqlTransaction(__func__, [&]()
        {
            Sql(R"sql(
                insert into Jury

                select

                    f.ROWID, /* Unique id of Flag record */
                    cu.Uid, /* Account unique id of the content author */
                    f.Int1 /* Reason */

                from Transactions f

                join Chain cf on
                    cf.TxId = f.RowId

                cross join Transactions u on
                    u.Type = 100 and
                    u.RegId1 = f.RegId3 and
                    exists (select 1 from Last lu where lu.TxId = u.RowId)

                join Chain cu on
                    cu.TxId = u.RowId
                    
                where f.HashId = (select r.RowId from Registry r where r.String = ?)

                    -- Is there no active punishment listed on the account ?
                    and not exists (
                        select 1
                        from JuryBan b indexed by JuryBan_AccountId_Ending
                        where b.AccountId = cu.Uid
                            and b.Ending > cf.Height
                    )

                    -- there is no active jury for the same reason
                    and not exists (
                        select 1
                        from Jury j indexed by Jury_AccountId_Reason
                        left join JuryVerdict jv
                            on jv.FlagRowId = j.FlagRowId
                        where j.AccountId = cu.Uid
                            and j.Reason = f.Int1
                            and jv.Verdict is null
                    )

                    -- if there are X flags of the same reason for X time
                    and ? <= (
                        select count()
                        from Transactions ff
                        join Chain cff indexed by Chain_Height_BlockId on
                            cff.TxId = ff.RowId and
                            cff.Height > ?
                        where ff.Type in (410)
                            and ff.RegId3 = f.RegId3
                            and not exists (select 1 from Last lff where lff.TxId = ff.RowId)
                    )
            )sql")
            .Bind(flagTxHash, flagsMinCount, flagsDepth)
            .Run();

            // Mechanism of distribution of moderators for voting
            // As a "nonce" we use the hash of the flag transaction that the jury created.
            // We sort the moderator registration hashes and compare them with the flag hash
            // to get all the moderator IDs before and after
            Sql(R"sql(
                insert into JuryModerators (AccountId, FlagRowId)
                with
                  h as (
                    select r.String as hash, r.RowId as hashid
                    from Registry r where r.String = ?
                  ),
                  f as (
                    select f.RowId, h.hash
                    from Transactions f,
                        Jury j,
                        h
                    where f.HashId = h.hashid and j.FlagRowId = f.ROWID
                  ),
                  c as (
                    select ?/2 as cnt
                  ),
                  a as (
                    select b.AccountId, (select r.String from Registry r where r.RowId = (select t.HashId from Transactions t where t.RowId = u.TxId)) as Hash
                    from vBadges b
                    cross join Chain u on u.Uid = b.AccountId and exists (select 1 from First f where f.TxId = u.TxId)
                    where b.Badge = 3
                  ),
                  l as (
                    select a.AccountId, a.Hash, row_number() over (order by a.Hash)row_number
                    from a,f
                    where a.Hash > f.hash
                  ),
                  r as (
                    select a.AccountId, a.Hash, row_number() over (order by a.Hash desc)row_number
                    from a,f
                    where a.Hash < f.hash
                  )
                select l.AccountId, f.ROWID from l,c,f where l.row_number <= c.cnt + (c.cnt - (select count() from r where r.row_number <= c.cnt))
                union
                select r.AccountId, f.ROWID from r,c,f where r.row_number <= c.cnt + (c.cnt - (select count() from l where l.row_number <= c.cnt))
            )sql")
            .Bind(flagTxHash, juryModeratorsCount)
            .Run();
        });
    }

    void ChainRepository::RestoreModerationJury(int height)
    {
        SqlTransaction(__func__, [&]()
        {
            Sql(R"sql(
                delete
                from
                    Jury
                where
                    FlagRowId in (
                        select
                            f.RowId
                        -- TODO (optimization): indices
                        from
                            Transactions f
                        join
                            Chain c on
                                c.Height >= ?
                        where
                            f.Type = 410
                    )
            )sql")
            .Bind(height)
            .Run();

            Sql(R"sql(
                delete
                from
                    JuryModerators
                where
                    FlagRowId in (
                        select
                            f.RowId
                        -- TODO (optimization): indices
                        from
                            Transactions f
                        join
                            Chain c on
                                c.Height >= ?
                        where
                            f.Type = 410
                            
                    )
            )sql")
            .Bind(height)
            .Run();
        });
    }

    void ChainRepository::IndexModerationBan(const string& voteTxHash, int votesCount, int ban1Time, int ban2Time, int ban3Time)
    {
        SqlTransaction(__func__, [&]()
        {
            Sql(R"sql(
                -- if there is at least one negative vote, then the defendant is acquitted
                insert or ignore into
                    JuryVerdict (FlagRowId, VoteRowId, Verdict)
                select
                    f.RowId,
                    v.RowId,
                    0
                from
                    Transactions v
                    cross join Transactions f indexed by Transactions_HashId
                        on f.HashId = v.RegId2
                    cross join Transactions vv on
                        vv.Type in (420) and -- Votes
                        vv.RegId2 = f.HashId and -- JuryId over FlagTxHash
                        vv.Int1 = 0 and -- Negative verdict
                        not exists (select 1 from Last l where l.TxId = vv.RowId) -- TODO (optimization): in it needed or was used just for index?
                        
                where
                    v.HashId = (select r.RowId from Registry r where r.String = ?)
            )sql")
            .Bind(voteTxHash)
            .Run();
            
            Sql(R"sql(
                -- if there are X positive votes, then the defendant is punished
                insert or ignore into
                    JuryVerdict (FlagRowId, VoteRowId, Verdict)
                select
                    f.RowId,
                    v.RowId,
                    1
                from
                    Transactions v
                    cross join Transactions f indexed by Transactions_HashId
                        on f.HashId = v.RegId2
                where
                    v.HashId = (select r.RowId from Registry r where r.String = ?) and
                    ? <= (
                        select
                            count()
                        from
                            Transactions vv
                        where
                            vv.Type in (420) and -- Votes
                            vv.RegId2 = f.HashId and -- JuryId over FlagTxHash
                            vv.Int1 = 1 and -- Positive verdict
                            not exists (select 1 from Last l where l.TxId = vv.RowId) -- TODO (optimization): in it needed or was used just for index?
                    )
            )sql")
            .Bind(voteTxHash, votesCount)
            .Run();
            
            Sql(R"sql(
                -- If the defendant is punished, then we need to create a ban record
                insert into
                    JuryBan (VoteRowId, AccountId, Ending)
                select
                    v.RowId, /* Unique id of Vote record */
                    j.AccountId, /* Address of the content author */
                    (
                        case ( select count() from JuryBan b indexed by JuryBan_AccountId_Ending where b.AccountId = j.AccountId )
                            when 0 then ?
                            when 1 then ?
                            else ?
                        end
                    ) /* Ban period */
                from
                    Transactions v
                    join Chain cv
                        on cv.TxId = v.RowId
                    join Transactions f indexed by Transactions_HashId
                        on f.HashId = v.RegId2
                    cross join Jury j
                        on j.FlagRowId = f.ROWID
                    cross join JuryVerdict jv
                        on jv.VoteRowId = v.ROWID and
                           jv.FlagRowId = j.FlagRowId and
                           jv.Verdict = 1
                where
                    v.HashId = (select r.RowId from Registry r where r.String = ?) and
                    not exists (
                        select
                            1
                        from
                            JuryBan b indexed by JuryBan_AccountId_Ending
                        where
                            b.AccountId = j.AccountId and
                            b.Ending > cv.Height
                    )
            )sql")
            .Bind(ban1Time, ban2Time, ban3Time, voteTxHash)
            .Run();
        });
    }

    void ChainRepository::RestoreModerationBan(int height)
    {
        SqlTransaction(__func__, [&]()
        {
            Sql(R"sql(
                delete
                from
                    JuryVerdict indexed by JuryVerdict_VoteRowId_FlagRowId_Verdict
                where
                    VoteRowId in (
                        select
                            v.RowId
                        from
                            Transactions v
                        join
                            Chain c on
                                c.TxId = v.RowId and
                                c.Height >= ?
                        where
                            v.Type = 420
                    )
            )sql")
            .Bind(height)
            .Run();
            
            Sql(R"sql(
                delete
                from
                    JuryBan
                where
                    VoteRowId in (
                        select
                            v.RowId
                        from
                            Transactions v
                        join
                            Chain c on
                                c.TxId = v.RowId and
                                c.Height >= ?
                        where
                            v.Type = 420
                    )
            )sql")
            .Bind(height)
            .Run();
        });
    }

    void ChainRepository::IndexBadges(int height, const BadgeConditions& conditions)
    {
        SqlTransaction(__func__, [&]()
        {
            Sql(R"sql(
                insert into
                    Badges (AccountId, Badge, Cancel, Height)

                select
                    b.AccountId, b.Badge, 1, ?

                from
                    vBadges b

                where
                    b.Badge = ? and
                    (
                        -- Likers over root comments must be above N
                        ? > ifnull((
                            select
                                lc.Value
                            from
                                Ratings lc indexed by Ratings_Type_Uid_Last_Value
                            where
                                lc.Type in (112) and
                                lc.Last = 1 and
                                lc.Uid = b.AccountId
                        ), 0) or

                        -- Sum liker must be above N
                        ? > ifnull((
                            select
                                sum(l.Value)
                            from
                                Ratings l indexed by Ratings_Type_Uid_Last_Value
                            where
                                l.Type in (111, 112, 113) and
                                l.Last = 1 and
                                l.Uid = b.AccountId
                        ), 0) or

                        -- Account must be registered above N months
                        ? >= (? - (
                            select reg.Height
                            from Chain reg
                            cross join First f
                                on f.TxId = reg.TxId
                            where
                                reg.Uid = b.AccountId
                        )) or

                        -- Account must be active (not deleted)
                        not exists (
                            select 1
                            from Chain c
                            cross join Transactions u
                                on u.RowId = c.TxId and u.Type = 100
                            cross join Last l
                                on l.TxId = u.RowId
                            where
                                c.Uid = b.AccountId
                        )
                    )
            )sql")
            .Bind(
                height,
                conditions.Number,
                conditions.LikersComment,
                conditions.LikersAll,
                conditions.RegistrationDepth,
                height
            )
            .Run();
            
            Sql(R"sql(
                insert into
                    Badges (AccountId, Badge, Cancel, Height)

                select
                    lc.Uid, ?, 0, ?

                from
                    Ratings lc indexed by Ratings_Type_Uid_Last_Value

                where
                    not exists(select 1 from vBadges b where b.Badge = ? and b.AccountId = lc.Uid) and

                    -- The main filtering rule is performed by the main filter
                    -- Likers over root comments must be above N
                    lc.Type = 112 and
                    lc.Uid > 0 and
                    lc.Last = 1 and
                    lc.Value >= ? and

                    -- Sum liker must be above N
                    ? <= ifnull((
                        select
                            sum(l.Value)
                        from
                            Ratings l indexed by Ratings_Type_Uid_Last_Value
                        where
                            l.Type in (111, 112, 113) and
                            l.Last = 1 and
                            l.Uid = lc.Uid
                    ), 0) and

                    -- Account must be registered above N months
                    ? < (? - (
                        select reg.Height
                        from Chain reg
                        cross join First f
                            on f.TxId = reg.TxId
                        where
                            reg.Uid = lc.Uid
                    )) and

                    -- Account must be active
                    exists (
                        select 1
                        from Chain c
                        cross join Transactions u
                            on u.RowId = c.TxId and u.Type = 100
                        cross join Last l
                            on l.TxId = u.RowId
                        where
                            c.Uid = lc.Uid
                    )
            )sql")
            .Bind(
                conditions.Number,
                height,
                conditions.Number,
                conditions.LikersComment,
                conditions.LikersAll,
                conditions.RegistrationDepth,
                height
            )
            .Run();
        });
    }

    void ChainRepository::RestoreBadges(int height)
    {
        SqlTransaction(__func__, [&]()
        {
            Sql(R"sql(
                delete from
                    Badges
                where
                    Height >= ?
            )sql")
            .Bind(height)
            .Run();
        });
    }

    bool ChainRepository::ClearDatabase()
    {
        LogPrintf("Start clean database..\n");

        try
        {
            LogPrintf("Deleting all indexes\n");
            m_database.DropIndexes();

            LogPrintf("Deleting all calculated tables\n");
            SqlTransaction(__func__, [&]()
            {
                Sql(R"sql( delete from Last )sql").Run();
                Sql(R"sql( delete from First )sql").Run();
                Sql(R"sql( delete from Ratings )sql").Run();
                Sql(R"sql( delete from Balances )sql").Run();
                Sql(R"sql( delete from Chain )sql").Run();
                Sql(R"sql( delete from Jury )sql").Run();
                Sql(R"sql( delete from JuryModerators )sql").Run();
                Sql(R"sql( delete from JuryVerdict )sql").Run();
                Sql(R"sql( delete from JuryBan )sql").Run();
                Sql(R"sql( delete from Badges )sql").Run();

                // TODO (aok) : bad
                // ClearBlockingList();
            });

            m_database.CreateStructure();

            return true;
        }
        catch (exception& ex)
        {
            LogPrintf("Error: Clear database failed with message: %s\n", ex.what());
            return false;
        }
    }
    
    void ChainRepository::RestoreLast(int height)
    {
        SqlTransaction(__func__, [&]()
        {
            // Restore old Last transactions
            Sql(R"sql(
                with
                    height as (
                        select ? as value
                    ),
                    prev as (
                        select
                            c.Uid, max(cc.Height)maxHeight
                        from
                            height,
                            Chain c indexed by Chain_Height_Uid
                            cross join Last l -- primary key
                                on l.TxId = c.TxId
                            cross join Chain cc indexed by Chain_Uid_Height
                                on cc.Uid = c.Uid and cc.Height < height.value
                        where
                            c.Height >= height.value
                        group by c.Uid
                    )
                insert into
                    Last (TxId)
                select
                    cp.TxId
                from
                    prev
                    cross join Chain cp indexed by Chain_Uid_Height
                        on cp.Uid = prev.Uid and
                        cp.Height = prev.maxHeight
            )sql")
            .Bind(height)
            .Run();

            // Delete current last records
            Sql(R"sql(
                delete from Last
                where
                    TxId in (
                        select
                            c.TxId
                        from
                            Chain c indexed by Chain_Height_Uid
                        where
                            c.Height >= ?
                    )
            )sql")
            .Bind(height)
            .Run();

            // Remove not used First records
            Sql(R"sql(
                delete from First
                where
                    TxId in (
                        select
                            c.TxId
                        from
                            Chain c indexed by Chain_Height_Uid
                        where
                            c.Height >= ?
                    )
            )sql")
            .Bind(height)
            .Run();
        });
    }

    void ChainRepository::RestoreRatings(int height)
    {
        SqlTransaction(__func__, [&]()
        {
            // Restore Last for deleting ratings
            Sql(R"sql(
                update Ratings indexed by Ratings_Type_Uid_Height_Value
                    set Last=1
                from (
                    select r1.Type, r1.Uid, max(r2.Height)Height
                    from Ratings r1 indexed by Ratings_Type_Uid_Last_Height
                    join Ratings r2 indexed by Ratings_Type_Uid_Last_Height on r2.Type = r1.Type and r2.Uid = r1.Uid and r2.Last = 0 and r2.Height < ?
                    where r1.Height >= ?
                    and r1.Last = 1
                    group by r1.Type, r1.Uid
                )r
                where Ratings.Type = r.Type
                and Ratings.Uid = r.Uid
                and Ratings.Height = r.Height
            )sql")
            .Bind(height, height)
            .Run();

            // Remove ratings
            Sql(R"sql(
                delete from Ratings indexed by Ratings_Height_Last
                where Height >= ?
            )sql")
            .Bind(height)
            .Run();
        });
    }

    void ChainRepository::RestoreBalances(int height)
    {
        SqlTransaction(__func__, [&]()
        {
            Sql(R"sql(
                with
                    height as (
                        select ? as value
                    ),
                    outs as (
                        select
                            o.TxId,
                            o.Number,
                            o.AddressId,
                            (+o.Value)val
                        from
                            height,
                            Chain c indexed by Chain_Height_BlockId
                            cross join TxOutputs o indexed by TxOutputs_TxId_Number_AddressId
                                on o.TxId = c.TxId
                        where
                            c.Height >= height.value

                        union

                        select
                            o.TxId,
                            o.Number,
                            o.AddressId,
                            (-o.Value)val
                        from
                            height,
                            Chain ci indexed by Chain_Height_BlockId
                            cross join TxInputs i indexed by TxInputs_SpentTxId_TxId_Number
                                on i.SpentTxId = ci.TxId
                            cross join TxOutputs o indexed by TxOutputs_TxId_Number_AddressId
                                on o.TxId = i.TxId and o.Number = i.Number
                        where
                            ci.Height >= height.value
                    ),
                    saldo as (
                        select
                            outs.AddressId,
                            sum(outs.val)Amount
                        from
                            outs
                        group by
                            outs.AddressId
                    )

                replace into Balances (AddressId, Value)
                select
                    saldo.AddressId,
                    ifnull(b.Value, 0) - saldo.Amount
                from
                    saldo
                    left join Balances b
                        on b.AddressId = saldo.AddressId
                where
                    saldo.Amount != 0
            )sql")
            .Bind(height)
            .Run();
        });
    }

    void ChainRepository::RestoreChain(int height)
    {
        SqlTransaction(__func__, [&]()
        {
            Sql(R"sql(
                delete from Chain indexed by Chain_Height_BlockId
                where Height >= ?
            )sql")
            .Bind(height)
            .Run();
        });
    }

    // void ChainRepository::RollbackBlockingList(int height)
    // {
    //     int64_t nTime0 = GetTimeMicros();

    //     auto& delListStmt = Sql(R"sql(
    //         delete from BlockingLists where ROWID in
    //         (
    //             select bl.ROWID
    //             from Transactions b
    //             join Chain bc
    //               on bc.TxId =  b.RowId
    //                 and bc.Height >= ?
    //             join Transactions us
    //               on us.Type in (100, 170)
    //                 and us.RegId1 = b.RegId1
    //             join Chain usc
    //               on usc.TxId = us.RowId
    //             join Last usl
    //               on usl.TxId = us.RowId
    //             join Transactions ut
    //               on ut.Type in (100, 170)
    //                 and ut.Int1 in (select b.RegId2 union select l.RegId from Lists l where l.TxId = b.RowId)
    //             join Chain utc
    //               on utc.TxId = ut.RowId
    //             join Last utl
    //               on utl.TxId = ut.RowId
    //             join BlockingLists bl on bl.IdSource = usc.Uid and bl.IdTarget = utc.Uid
    //             where b.Type in (305)
    //         )
    //     )sql");
    //     delListStmt->Bind(height);
    //     delListStmt->Step();
        
    //     int64_t nTime1 = GetTimeMicros();
    //     LogPrint(BCLog::BENCH, "        - RollbackList (Delete blocking list): %.2fms\n", 0.001 * (nTime1 - nTime0));

    //     auto& insListStmt = Sql(R"sql(
    //         insert into BlockingLists
    //         (
    //             IdSource,
    //             IdTarget
    //         )
    //         select distinct
    //           usc.Uid,
    //           utc.Uid
    //         from Transactions b
    //         join Chain bc
    //           on bc.TxId = b.RowId
    //             and bc.Height >= ?
    //         join Transactions us
    //           on us.Type in (100, 170) and us.Int1 = b.Int1
    //         join Chain usc
    //           on usc.TxId = us.RowId
    //         join Last usl
    //             on usl.TxId = us.RowId
    //         join Transactions ut
    //           on ut.Type in (100, 170)
    //             --and ut.String1 = b.String2
    //             and ut.Int1 in (select b.RegId2 union select l.RegId from Lists l where l.TxId = b.RowId)
    //         join Chain utc
    //           on utc.TxId = ut.RowId
    //         join Last utl
    //           on utl.TxId = ut.RowId
    //         where b.Type in (306)
    //           and not exists (select 1 from BlockingLists bl where bl.IdSource = usc.Uid and bl.IdTarget = utc.Uid)
    //     )sql");
    //     insListStmt->Bind(height);
    //     insListStmt->Step();
        
    //     int64_t nTime2 = GetTimeMicros();
    //     LogPrint(BCLog::BENCH, "        - RollbackList (Insert blocking list): %.2fms\n", 0.001 * (nTime2 - nTime1));
    // }

    // void ChainRepository::ClearBlockingList()
    // {
    //     int64_t nTime0 = GetTimeMicros();

    //     auto stmt = Sql(R"sql(
    //         delete from BlockingLists
    //     )sql");
    //     stmt.Step();
        
    //     int64_t nTime1 = GetTimeMicros();
    //     LogPrint(BCLog::BENCH, "        - ClearBlockingList (Delete blocking list): %.2fms\n", 0.001 * (nTime1 - nTime0));
    // }


} // namespace PocketDb
