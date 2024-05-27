// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include "pocketdb/repositories/ChainRepository.h"

#include "pocketdb/consensus/Base.h"

#include "util/string.h"
#include <array>

namespace {
    using namespace PocketTx;
    // TODO (losty): move to a proper place
    template <class T, size_t n>
    class StringifyableArray
    {
    public:
        template<class ...U>
        StringifyableArray(U... u) 
            : m_typesArr{{u...}},
              m_str{Join<T>({m_typesArr.begin(), m_typesArr.end()}, ",", [](const T& e) -> string {return to_string(e);})}
        {}

        std::string ToString() const { return m_str; }
        bool IsIn(T type) const { return find(m_typesArr.begin(), m_typesArr.end(), type) != m_typesArr.end(); };
    private:
        const std::array<T, n> m_typesArr;
        const string m_str;
    };
    template <class T, class ...U>
    StringifyableArray(T, U...) -> StringifyableArray<T, 1+sizeof...(U)>;

    class SocialRegistryTypes
    {
    public:
        static bool IsSatisfy(TxType type, bool isFirst)
        {
            return
                Common.IsIn(type) ||
                (FirstRequired.IsIn(type) && isFirst);
        }
        inline const static StringifyableArray FirstRequired { CONTENT_POST, CONTENT_VIDEO, CONTENT_ARTICLE };
        inline const static StringifyableArray Common { ACTION_SCORE_COMMENT, ACTION_SCORE_CONTENT, ACTION_COMPLAIN };
    };
}

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
                auto[id, lastTxId] = IndexSocial(txInfo, height);

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

                // if 'id' means that tx is social and requires last and first to be set.
                // if not 'lastTxId' means that this is first tx for this id
                if (id && !lastTxId)
                {
                    SetFirst(txInfo.Hash);
                }

                IndexSocialRegistryTx(txInfo, height, !lastTxId.has_value());

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

            EnsureAndTrimSocialRegistry(height + 1); // Count for next block

            int64_t nTime2 = GetTimeMicros();
            LogPrint(BCLog::BENCH, "    - IndexBlock: %.2fms\n", 0.001 * double(nTime2 - nTime1));
        });
    }

    tuple<bool, bool> ChainRepository::ExistsBlock(const string& blockHash, int height)
    {
        bool exists = false;
        bool last = true;

        SqlTransaction(__func__, [&]()
        {
            Sql(R"sql(
                select
                    ifnull((
                        select
                            1
                        from
                            Chain c indexed by Chain_BlockId_Height
                        where
                            c.BlockId = (
                                select
                                    r.RowId
                                from
                                    Registry r
                                where
                                    r.String = ?
                            ) and
                            c.Height = ?
                            
                        limit 1
                    ), 0),
                    ifnull((
                        select
                            1
                        from
                            Chain c indexed by Chain_Height_Uid
                        where
                            c.Height = ?
                        limit 1
                    ), 0)
            )sql")
            .Bind(blockHash, height, height + 1)
            .Select([&](Cursor& cursor) {
                if (cursor.Step())
                {
                    if (auto[ok, value] = cursor.TryGetColumnInt(0); ok && value == 1)
                        exists = true;

                    if (auto[ok, value] = cursor.TryGetColumnInt(1); ok && value == 1)
                        last = false;
                }
            });
        });

        return { exists, last };
    }

    int ChainRepository::CurrentHeight()
    {
        int result = -1;

        SqlTransaction(__func__, [&]()
        {
            Sql(R"sql(
                select
                    max(Height)
                from
                    Chain
            )sql")
            .Select([&](Cursor& cursor) {
                if (cursor.Step())
                    cursor.CollectAll(result);
            });
        });

        return result;
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
                insert or fail into Last
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
                        Chain c indexed by Chain_Height_Uid
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
                        Chain ci indexed by Chain_Height_Uid
                        cross join TxInputs i indexed by TxInputs_SpentTxId_Number_TxId
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

    void ChainRepository::IndexSocialRegistryTx(const TransactionIndexingInfo& txInfo, int height, bool isFirst)
    {
        if (!SocialRegistryTypes::IsSatisfy(txInfo.Type, isFirst))
            return;

        Sql(R"sql(
            insert or ignore into SocialRegistry (AddressId, Type, Height, BlockNum)
            with
                address as (
                    select
                        t.RegId1 as id
                    from
                        vTx t
                    where
                        t.Hash = ?
                )
            select
                address.id,
                ?,
                ?,
                ?
            from
                address
        )sql")
        .Bind(txInfo.Hash, txInfo.Type, height, txInfo.BlockNumber)
        .Run();
    }

    void ChainRepository::EnsureSocialRegistry(int height)
    {
        SqlTransaction(__func__, [&]()
        {
            EnsureAndTrimSocialRegistry(height);
        });
    }

    void ChainRepository::EnsureAndTrimSocialRegistry(int height)
    {
        int minHeight = height - PocketConsensus::BaseConsensus::GetConsensusLimit(PocketConsensus::ConsensusLimit_depth, height);
        
        // Trim from upper
        Sql(R"sql(
            delete from SocialRegistry
            where
                Height >= ?
        )sql")
        .Bind(height)
        .Run();
        
        // Trim from bottom
        Sql(R"sql(
            delete from SocialRegistry
            where
                Height < ?
        )sql")
        .Bind(minHeight)
        .Run();
        
        // Restore content and comments (First required)
        Sql(R"sql(
            insert or ignore into SocialRegistry (AddressId, Type, Height, BlockNum)
            select
                t.RegId1,
                t.Type,
                c.Height,
                c.BlockNum
            from
                Chain c indexed by Chain_Height_Uid
                cross join First f on
                    f.TxId = c.TxId
                join Transactions t on
                    t.RowId = c.TxId and
                    t.Type in ( )sql" + SocialRegistryTypes::FirstRequired.ToString() + R"sql()
            where
                c.Height between ? and (select min(Height) from SocialRegistry)
        )sql")
        .Bind(minHeight)
        .Run();
        
        // Restore actions
        Sql(R"sql(
            insert or ignore into SocialRegistry (AddressId, Type, Height, BlockNum)
            select
                t.RegId1,
                t.Type,
                c.Height,
                c.BlockNum
            from
                Chain c indexed by Chain_Height_Uid
                join Transactions t on
                    t.RowId = c.TxId and
                    t.Type in ( )sql" + SocialRegistryTypes::Common.ToString() + R"sql()
            where
                c.Height between ? and (select min(Height) from SocialRegistry)
        )sql")
        .Bind(minHeight)
        .Run();
    }

    pair<optional<int64_t>, optional<int64_t>> ChainRepository::IndexSocial(const TransactionIndexingInfo& txInfo, int height)
    {
        optional<int64_t> id;
        optional<int64_t> lastTxId;
        string sql;

        // Account and Content must have unique ID
        // Also all edited transactions must have Last=(0/1) field
        if (txInfo.IsAccount())
            IndexSocialLastTx(IndexAccount(), txInfo.Hash, id, lastTxId);
        else if (txInfo.IsAccountSetting())
            IndexSocialLastTx(IndexAccountSetting(), txInfo.Hash, id, lastTxId);
        else if (txInfo.IsContent())
            IndexSocialLastTx(IndexContent(), txInfo.Hash, id, lastTxId);
        else if (txInfo.IsComment())
            IndexSocialLastTx(IndexComment(), txInfo.Hash, id, lastTxId);
        else if (txInfo.IsBlocking())
        {
            IndexSocialLastTx(IndexBlocking(), txInfo.Hash, id, lastTxId);
            IndexBlockingList(txInfo.Hash, height);
        }
        else if (txInfo.IsSubscribe())
            IndexSocialLastTx(IndexSubscribe(), txInfo.Hash, id, lastTxId);
        // Barteron
        else if (txInfo.IsAccountBarteron())
            IndexSocialLastTx(IndexAccountBarteron(), txInfo.Hash, id, lastTxId);

        return {id, lastTxId};
    }

    void ChainRepository::IndexSocialLastTx(const string& sql, const string& txHash, optional<int64_t>& id, optional<int64_t>& lastTxId)
    {
        Sql(sql)
        .Bind(txHash)
        .Select([&](Cursor& cursor) {
            if (cursor.Step())
                cursor.CollectAll(id, lastTxId);
        });
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
                            on b.Type in (100, 170) and b.RegId1 = a.RegId1
                        join Last l -- primary key
                            on l.TxId = b.RowId
                    where
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
                            on b.Type in (103) and b.RegId1 = a.RegId1
                        join Last l -- primary key
                            on l.TxId = b.RowId
                    where
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
                        join Transactions b indexed by Transactions_Type_RegId2_RegId1
                            on b.Type in (200,201,202,209,210,211,220,207) and b.RegId2 = a.RegId2
                        join Last l -- primary key
                            on l.TxId = b.RowId
                    where
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
                        join Transactions b indexed by Transactions_Type_RegId2_RegId1
                            on b.Type in (204, 205, 206) and b.RegId2 = a.RegId2
                        join Last l -- primary key
                            on l.TxId = b.RowId
                    where
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

    // TODO (aok): Not need set last for blockings - use instead BlockingLists
    string ChainRepository::IndexBlocking()
    {
        return R"sql(
            with
                l as (
                    select
                        b.RowId
                    from
                        Transactions a -- primary key
                        left join (select TxId, json_array(RegId)ab_arr from Lists)ab on
                            ab.TxId = a.RowId
                        join Transactions b indexed by Transactions_Type_RegId1_RegId2_RegId3 on
                            b.Type in (305, 306) and
                            b.RegId1 = a.RegId1 and
                            ifnull(b.RegId2, -1) = ifnull(a.RegId2, -1) and
                            ifnull((select json_array(lst.RegId) from Lists lst where lst.TxId = b.RowId), -1) = ifnull(ab.ab_arr, -1)
                        join Last l on -- primary key
                            l.TxId = b.RowId
                    where
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

    void ChainRepository::IndexBlockingList(const string& txHash, int height)
    {
        Sql(R"sql(
            with
                tx as (
                    select RowId
                    from Registry
                    where String = ?
                )

            insert or ignore into
                BlockingLists (IdSource, IdTarget)

            select
                b.RegId1,
                b.RegId2
            from
                tx
            cross join
                Transactions b on
                    b.Type in (305) and
                    b.RowId = tx.RowId and
                    b.RegId2 is not null

            union
            select
                b.RegId1,
                l.RegId
            from
                tx
            cross join
                Transactions b on
                    b.Type in (305) and
                    b.RowId = tx.RowId and
                    b.RegId2 is null
            cross join
                Lists l on
                    l.TxId = b.RowId
        )sql")
        .Bind(txHash)
        .Run();

        Sql(R"sql(
            with
                tx as (
                    select RowId
                    from Registry
                    where String = ?
                )
            delete from
                BlockingLists
            where
                BlockingLists.ROWID in
                (
                    select
                        bl.ROWID
                    from
                        tx
                    cross join
                        Transactions b on
                            b.Type in (306) and
                            b.RowId = tx.RowId
                    cross join
                        BlockingLists bl on
                            bl.IdSource = b.RegId1 and
                            bl.IdTarget = b.RegId2
                )
        )sql")
        .Bind(txHash)
        .Run();
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
                            on b.Type in (104, 170) and b.RegId1 = a.RegId1
                        join Last l -- primary key
                            on l.TxId = b.RowId
                    where
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
        SqlTransaction(__func__, [&]()
        {
            Sql(R"sql(
                insert into Jury

                select

                    f.RowId, /* Unique id of Flag record */
                    cu.Uid, /* Account unique id of the content author */
                    f.Int1 /* Reason */

                from Transactions f

                cross join Chain cf
                    on cf.TxId = f.RowId

                cross join Transactions u indexed by Transactions_Type_RegId1_RegId2_RegId3
                    on u.Type = 100 and u.RegId1 = f.RegId3

                cross join Last lu
                    on lu.TxId = u.RowId

                cross join Chain cu on
                    cu.TxId = u.RowId

                where f.RowId = (select r.RowId from Registry r where r.String = ?)

                    -- Is there no active punishment listed on the account ?
                    and not exists (
                        select 1
                        from JuryBan b indexed by JuryBan_AccountId_Ending
                        where
                            b.AccountId = cu.Uid and
                            b.Ending > cf.Height
                    )

                    -- there is no active jury for the same reason
                    and not exists (
                        select 1
                        from Jury j indexed by Jury_AccountId_Reason
                        left join JuryVerdict jv
                            on jv.FlagRowId = j.FlagRowId
                        where
                            j.AccountId = cu.Uid and
                            j.Reason = f.Int1 and
                            jv.Verdict is null
                    )

                    -- if there are X flags of the same reason for X time
                    and ? <= (
                        select count()
                        from Transactions ff indexed by Transactions_Type_RegId3_RegId1
                        cross join Chain cff
                            on cff.TxId = ff.RowId and cff.Height > ?
                        left join Last lff
                            on lff.TxId = ff.RowId
                        where
                            ff.Type in (410) and
                            ff.RegId3 = f.RegId3 and
                            lff.ROWID is null
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
                    select r.String as hash, r.RowId as rowid
                    from Registry r where r.String = ?
                  ),
                  f as (
                    select f.RowId, h.hash
                    from Transactions f,
                        Jury j,
                        h
                    where f.RowId = h.RowId and j.FlagRowId = f.RowId
                  ),
                  c as (
                    select ?/2 as cnt
                  ),
                  a as (
                    select b.AccountId, (select r.String from Registry r where r.RowId = u.TxId) as Hash
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
        Sql(R"sql(
            delete
            from
                Jury
            where
                FlagRowId in (
                    select
                        f.RowId
                    from
                        Chain c
                    cross join
                        Transactions f on
                            f.RowId = c.TxId and
                            f.Type = 410
                    where
                        c.Height >= ?
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
                    from
                        Chain c
                    cross join
                        Transactions f on
                            f.RowId = c.TxId and
                            f.Type = 410
                    where
                        c.Height >= ?
                )
        )sql")
        .Bind(height)
        .Run();
    }

    void ChainRepository::IndexModerationBan(const string& voteTxHash, int votesCount, int ban1Time, int ban2Time, int ban3Time)
    {
        SqlTransaction(__func__, [&]()
        {
            Sql(R"sql(
                -- if there is at least one negative vote, then the defendant is acquitted
                insert or fail into
                    JuryVerdict (FlagRowId, VoteRowId, Verdict)
                select
                    f.RowId,
                    v.RowId,
                    0
                from
                    Transactions v
                    cross join
                        Transactions f on
                            f.RowId = v.RegId2
                    cross join
                        Transactions vv on
                            vv.Type in (420) and -- Votes
                            vv.RegId2 = f.RowId and -- JuryId over FlagTxHash
                            vv.Int1 = 0 -- Negative verdict
                    cross join
                        Chain c on
                            c.TxId = vv.RowId
                where
                    v.RowId = (select r.RowId from Registry r where r.String = ?) and
                    not exists (select 1 from JuryVerdict jv where jv.FlagRowId = f.RowId)
            )sql")
            .Bind(voteTxHash)
            .Run();
            
            Sql(R"sql(
                -- if there are X positive votes, then the defendant is punished
                insert or fail into
                    JuryVerdict (FlagRowId, VoteRowId, Verdict)
                select
                    f.RowId,
                    v.RowId,
                    1
                from
                    Transactions v
                    cross join
                        Transactions f on
                            f.RowId = v.RegId2
                where
                    v.RowId = (select r.RowId from Registry r where r.String = ?) and
                    not exists (select 1 from JuryVerdict jv where jv.FlagRowId = f.RowId) and
                    ? <= (
                        select
                            count()
                        from
                            Transactions vv
                        cross join
                            Chain c on
                                c.TxId = vv.RowId
                        where
                            vv.Type in (420) and -- Votes
                            vv.RegId2 = f.RowId and -- JuryId over FlagTxHash
                            vv.Int1 = 1 -- Positive verdict
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
                    join Transactions f
                        on f.RowId = v.RegId2
                    cross join Jury j
                        on j.FlagRowId = v.RegId2
                    cross join JuryVerdict jv
                        on jv.VoteRowId = v.RowId and
                           jv.FlagRowId = j.FlagRowId and
                           jv.Verdict = 1
                where
                    v.RowId = (select r.RowId from Registry r where r.String = ?) and
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
    }

    void ChainRepository::IndexBadges(int height, const BadgeConditions& conditions)
    {
        SqlTransaction(__func__, [&]()
        {
            // Firstly cancel
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
        Sql(R"sql(
            delete from
                Badges
            where
                Height >= ?
        )sql")
        .Bind(height)
        .Run();
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
                Sql(R"sql( delete from BlockingLists )sql").Run();
                Sql(R"sql( delete from SocialRegistry )sql").Run();
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
    
    void ChainRepository::Restore(int height)
    {
        SqlTransaction(__func__, [&]()
        {
            RestoreLast(height);
            RestoreRatings(height);
            RestoreBalances(height);
            RollbackBlockingList(height);
            RestoreModerationJury(height);
            RestoreModerationBan(height);
            RestoreBadges(height);
            RestoreSocialRegistry(height);

            // Rollback transactions must be lasted
            RestoreChain(height);
        });
    }

    void ChainRepository::RestoreLast(int height)
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
    }

    void ChainRepository::RestoreRatings(int height)
    {
        // Restore Last for deleting ratings
        Sql(R"sql(
            update Ratings indexed by Ratings_Type_Uid_Height_Value
                set Last=1
            from (
                select r1.Type, r1.Uid, max(r2.Height)Height
                from Ratings r1 indexed by Ratings_Height_Last
                join Ratings r2 indexed by Ratings_Type_Uid_Last_Height on r2.Type = r1.Type and r2.Uid = r1.Uid and r2.Last = 0 and r2.Height < ?
                where r1.Height >= ? and r1.Last = 1
                group by r1.Type, r1.Uid
            )r
            where
                Ratings.Type = r.Type and
                Ratings.Uid = r.Uid and
                Ratings.Height = r.Height
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
    }

    void ChainRepository::RestoreBalances(int height)
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
                        Chain c indexed by Chain_Height_Uid
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
                        Chain ci indexed by Chain_Height_Uid
                        cross join TxInputs i indexed by TxInputs_SpentTxId_Number_TxId
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
    }

    void ChainRepository::RestoreChain(int height)
    {
        Sql(R"sql(
            delete from Chain indexed by Chain_Height_Uid
            where Height >= ?
        )sql")
        .Bind(height)
        .Run();
    }

    void ChainRepository::RestoreSocialRegistry(int height)
    {
        EnsureAndTrimSocialRegistry(height);
    }

    void ChainRepository::RollbackBlockingList(int height)
    {
        Sql(R"sql(
            with
                height as ( select ? as value )

            delete from BlockingLists where ROWID in
            (
                select
                    bl.ROWID
                from
                    height
                cross join
                    Chain bc on
                        bc.Height >= height.value
                cross join Transactions b on
                    b.RowId = bc.TxId and
                    b.Type in (305) and
                    b.RegId2 is not null
                cross join
                    BlockingLists bl on
                        bl.IdSource = b.RegId1 and
                        bl.IdTarget = b.RegId2

                union
                select
                    bl.ROWID
                from
                    height
                cross join
                    Chain bc on
                        bc.Height >= height.value
                cross join Transactions b on
                    b.RowId = bc.TxId and
                    b.Type in (305) and
                    b.RegId2 is null
                cross join
                    Lists l on
                        l.TxId = b.RowId
                cross join
                    BlockingLists bl on
                        bl.IdSource = b.RegId1 and
                        bl.IdTarget = l.RegId
            )
        )sql")
        .Bind(height)
        .Run();
        
        Sql(R"sql(
            with
                height as ( select ? as value )

            insert or ignore into BlockingLists (IdSource, IdTarget)
            select distinct
                b.RegId1,
                b.RegId2
            from
                height
            cross join
                Chain bc on
                    bc.Height >= height.value
            cross join
                Transactions b on
                    b.RowId = bc.TxId and
                    b.Type in (306)
        )sql")
        .Bind(height)
        .Run();
    }

} // namespace PocketDb
