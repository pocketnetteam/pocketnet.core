// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include "pocketdb/repositories/MigrationRepository.h"
#include <node/ui_interface.h>
#include <util/translation.h>

namespace PocketDb
{
    vector<tuple<int, vector<tuple<string, int>>>> MigrationRepository::GetAllModTxs()
    {
        vector<tuple<int, vector<tuple<string, int>>>> result;
        Sql(R"sql(
            select c.Height, r.String, f.Type, BlockNum
            from Transactions f
            cross join Chain c on c.TxId = f.RowId
            cross join Registry r on r.RowId = f.RowId
            where f.Type in (410, 420)
            order by c.Height, c.BlockNum
        )sql")
        .Select([&](Cursor& cursor) {
            while (cursor.Step())
            {
                int height;
                string hash;
                int type;
                if (cursor.CollectAll(height, hash, type)){
                    result.emplace_back(height, vector<tuple<string, int>>{make_tuple(hash, type)});
                }
            }
        });
        return result;
    }

    void MigrationRepository::ClearAllJuries()
    {
        Sql(R"sql(
            delete from Jury
        )sql")
        .Run();

        Sql(R"sql(
            delete from JuryVerdict
        )sql")
        .Run();

        Sql(R"sql(
            delete from JuryBan
        )sql")
        .Run();

        Sql(R"sql(
            delete from JuryModerators
        )sql")
        .Run();
    }

    int MigrationRepository::LikersByFlag(const string& txHash, int height)
    {
        int result = 0;
        
        SqlTransaction(__func__, [&]()
        {
            Sql(R"sql(
                with
                    flag as ( select r.RowId from Registry r where r.String = ? )
                select
                   ifnull((
                        select
                            lp.Value
                        from Ratings lp
                        where
                            lp.Type in (111) and
                            lp.Uid = cu.Uid and
                            lp.Height <= ?
                        order by
                            lp.Height desc
                        limit 1
                     ), 0) +
                     ifnull((
                        select
                            lp.Value
                        from Ratings lp
                        where
                            lp.Type in (112) and
                            lp.Uid = cu.Uid and
                            lp.Height <= ?
                        order by
                            lp.Height desc
                        limit 1
                     ), 0) +
                     ifnull((
                        select
                            lp.Value
                        from Ratings lp
                        where
                            lp.Type in (113) and
                            lp.Uid = cu.Uid and
                            lp.Height <= ?
                        order by
                            lp.Height desc
                        limit 1
                     ), 0) lkrs
                from
                    flag
                cross join
                    Transactions f on
                        f.RowId = flag.RowId
                cross join
                    Transactions u indexed by Transactions_Type_RegId1_Time on
                        u.Type in (100, 170) and u.RegId1 = f.RegId3
                cross join
                    Chain cu on
                        cu.TxId = u.RowId
                cross join
                    First fu on
                        fu.TxId = u.RowId
            )sql")
            .Bind(txHash, height, height, height)
            .Select([&](Cursor& cursor) {
                if (cursor.Step())
                    cursor.CollectAll(result);
            });
        });

        return result;
    }

    int MigrationRepository::LikersByVote(const string& txHash, int height)
    {
        int result = 0;
        
        SqlTransaction(__func__, [&]()
        {
            Sql(R"sql(
                with
                    vote as ( select r.RowId from Registry r where r.String = ? )
                select
                    ifnull((
                        select
                            lp.Value
                        from Ratings lp
                        where
                            lp.Type in (111) and
                            lp.Uid = cu.Uid and
                            lp.Height <= ?
                        order by
                            lp.Height desc
                        limit 1
                     ), 0) +
                     ifnull((
                        select
                            lp.Value
                        from Ratings lp
                        where
                            lp.Type in (112) and
                            lp.Uid = cu.Uid and
                            lp.Height <= ?
                        order by
                            lp.Height desc
                        limit 1
                     ), 0) +
                     ifnull((
                        select
                            lp.Value
                        from Ratings lp
                        where
                            lp.Type in (113) and
                            lp.Uid = cu.Uid and
                            lp.Height <= ?
                        order by
                            lp.Height desc
                        limit 1
                     ), 0) lkrs
                from
                    vote
                cross join
                    Transactions v on
                        v.RowId = vote.RowId
                cross join
                    Transactions f on
                        f.RowId = v.RegId2
                cross join
                    Transactions u indexed by Transactions_Type_RegId1_Time on
                        u.Type in (100, 170) and u.RegId1 = f.RegId3
                cross join
                    Chain cu on
                        cu.TxId = u.RowId
                cross join
                    First fu on
                        fu.TxId = u.RowId
            )sql")
            .Bind(txHash, height, height, height)
            .Select([&](Cursor& cursor) {
                if (cursor.Step())
                    cursor.CollectAll(result);
            });
        });

        return result;
    }

    void MigrationRepository::IndexModerationJury(const string& flagTxHash, int flagsDepth, int topHeight, int flagsMinCount, int juryModeratorsCount)
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

                cross join First fu
                    on fu.TxId = u.RowId

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
                        from Transactions ff
                        cross join Chain cff
                            on cff.TxId = ff.RowId and cff.Height > ? and cff.Height <= ?
                        where
                            ff.Type in (410) and
                            ff.RegId3 = f.RegId3 and
                            ff.Int1 = f.Int1
                    )
            )sql")
            .Bind(flagTxHash, flagsMinCount, flagsDepth, topHeight)
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
                    from Badges b indexed by Badges_Badge_Cancel_AccountId_Height
                    cross join Chain u on u.Uid = b.AccountId and exists (select 1 from First f where f.TxId = u.TxId)
                    where b.Badge = 3 and b.Cancel=0 and b.AccountId=u.Uid and b.Height < ? and
                        not exists (
                            select
                                1
                            from
                                Badges bb indexed by Badges_Badge_Cancel_AccountId_Height
                            where
                                bb.Badge = b.Badge and
                                bb.Cancel = 1 and
                                bb.AccountId = b.AccountId and
                                bb.Height > b.Height and
                                bb.Height < ?
                        )
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
            .Bind(flagTxHash, juryModeratorsCount, topHeight, topHeight)
            .Run();
        });
    }

    void MigrationRepository::IndexModerationBan(const string& voteTxHash, int topHeight, int votesCount, int ban1Time, int ban2Time, int ban3Time)
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
                            c.TxId = vv.RowId and
                            c.Height <= ?
                where
                    v.RowId = (select r.RowId from Registry r where r.String = ?) and
                    not exists (select 1 from JuryVerdict jv where jv.FlagRowId = f.RowId)
            )sql")
            .Bind(topHeight, voteTxHash)
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
                                c.TxId = vv.RowId and c.Height <= ?
                        where
                            vv.Type in (420) and -- Votes
                            vv.RegId2 = f.RowId and -- JuryId over FlagTxHash
                            vv.Int1 = 1 -- Positive verdict
                    )
            )sql")
            .Bind(voteTxHash, votesCount, topHeight)
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
                    cross join Chain cv
                        on cv.TxId = v.RowId
                    cross join Transactions f
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




} // namespace PocketDb

