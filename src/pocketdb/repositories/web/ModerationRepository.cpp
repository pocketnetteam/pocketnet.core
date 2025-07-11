// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include "pocketdb/repositories/web/ModerationRepository.h"

namespace PocketDb
{
    void ModerationRepository::Init() {}

    void ModerationRepository::Destroy() {}

    JuryContent ModerationRepository::GetJury(const string& jury)
    {
        JuryContent result;

        SqlTransaction(
            __func__,
            [&]() -> Stmt& {
                return Sql(R"sql(
                    with
                        flag as (
                            select
                                t.RowId as id,
                                r.String as hash,
                                t.RegId2 as content_id
                            from
                                Registry r
                            cross join
                                Transactions t
                                    on t.RowId = r.RowId
                            where
                                r.String = ?
                        ),
                        juryRec as (
                            select
                                j.AccountId,
                                j.Reason
                            from
                                flag
                            cross join
                                Jury j
                                    on j.FlagRowId = flag.id
                        ),
                        account as (
                            select
                                c.Uid,
                                (select r.String from Registry r where r.RowId = u.RegId1) as AddressHash
                            from
                                juryRec
                            cross join
                                Chain c indexed by Chain_Uid_Height
                                    on c.Uid = juryRec.AccountId
                            cross join
                                First f
                                    on f.TxId = c.TxId
                            cross join
                                Transactions u
                                    on u.RowId = f.TxId
                        ),
                        content as (
                            select
                                c.Uid as content_id,
                                t.Type as content_type
                            from
                                flag
                            cross join Chain c on c.TxId = flag.content_id
                            cross join First f on f.TxId = c.TxId
                            cross join Transactions t on t.RowId = c.TxId
                        ),
                        juryVerd as (
                            select
                                jv.Verdict
                            from
                                flag
                            cross join
                                juryVerdict jv
                                    on jv.FlagRowId = flag.id
                        ),
                        ban as (
                            select
                                jb.Ending
                            from
                                flag
                            cross join JuryModerators jm on jm.FlagRowId = flag.id
                            cross join Chain c on c.Uid = jm.AccountId
                            cross join First f on f.TxId = c.TxId
                            cross join Transactions u on u.RowId = c.TxId and u.Type = 100
                            cross join Transactions v on v.Type = 420 and v.RegId1 = u.RegId1 and v.RegId2 = flag.id
                            cross join JuryBan jb on jb.VoteRowId = v.RowId
                        ),
                        likers as (
                            select
                                ifnull((
                                    select
                                        sum(lp.Value)
                                    from Ratings lp indexed by Ratings_Type_Uid_Last_Value
                                    where lp.Type in (111, 112, 113) and lp.Uid = a.Uid and lp.Last = 1
                                ),0) as value
                            from
                                account a
                        )
                    select
                        a.Uid,
                        a.AddressHash,
                        j.Reason,
                        ifnull(jv.Verdict, -1) as verdict,
                        ifnull(b.Ending, -1) as ban_ending,
                        c.content_id,
                        c.content_type,
                        l.value as likers
                    from
                        juryRec j
                        join account a
                        join likers l
                        left join content c
                        left join juryVerd jv
                        left join ban b
                )sql")
                .Bind(jury);
            },
            [&] (Stmt& stmt) {
                stmt.Select([&](Cursor& cursor) {
                    if (cursor.Step())
                    {
                        UniValue record(UniValue::VOBJ);
                        cursor.Collect<string>(0, record, "id");
                        cursor.Collect<string>(1, record, "address");
                        cursor.Collect<int>(2, record, "reason");
                        cursor.Collect<int>(3, record, "verdict");
                        cursor.Collect<int64_t>(4, record, "ban_ending");

                        int addressLikers;
                        if (auto [ok, value] = cursor.TryGetColumnInt(5); ok)
                            addressLikers = value;

                        int64_t contentId;
                        int contentType;
                        if (auto [ok, value] = cursor.TryGetColumnInt64(6); ok)
                        {
                            contentId = value;
                            contentType = 100;
                        }
                        if (auto [ok, value] = cursor.TryGetColumnInt64(5); ok)
                            contentId = value;
                        if (auto [ok, value] = cursor.TryGetColumnInt(6); ok)
                            contentType = value;

                        result = JuryContent{contentId, (TxType)contentType, addressLikers, record};
                    }
                });
            }
        );

        return result;
    }

    vector<JuryContent> ModerationRepository::GetAllJury(const Pagination& pagination)
    {
        vector<JuryContent> result;

        SqlTransaction(
            __func__,
            [&]() -> Stmt& {
                return Sql(R"sql(
                    select
                        cf.Height,
                        (select r.String from Registry r where r.RowId = f.RowId),
                        (select r.String from Registry r where r.RowId = f.RegId2),
                        (select r.String from Registry r where r.RowId = f.RegId3),
                        j.Reason,
                        ifnull(jv.Verdict, -1),
                        cc.Uid,
                        c.Type,
                        (
                            select count()
                            from Transactions v
                            cross join Chain vc on vc.TxId = v.RowId
                            where v.Type = 420 and v.RegId2 = j.FlagRowId
                        ) as votes,
                        ifnull((
                            select
                                sum(lp.Value)
                            from Ratings lp indexed by Ratings_Type_Uid_Last_Value
                            where lp.Type in (111, 112, 113) and lp.Uid = cu.Uid and lp.Last = 1
                        ),0) as likers
                    from
                        Jury j
                    cross join Transactions f on
                        f.RowId = j.FlagRowId
                    cross join Chain cf on
                        cf.TxId = f.RowId and cf.Height )sql" + (pagination.OrderDesc ? " <= "s : " > "s) + R"sql( ?
                    cross join Transactions c on
                        c.RowId = f.RegId2
                    cross join Chain cc on
                        cc.TxId = c.RowId
                    cross join Transactions u on
                        u.Type = 100 and u.RegId1 = f.RegId3
                    cross join First fu on
                        fu.TxId = u.RowId
                    cross join Chain cu on
                        cu.TxId = fu.TxId
                    left join JuryVerdict jv on
                        jv.FlagRowId = j.FlagRowId
                    order by cf.Height )sql" + (pagination.OrderDesc ? " desc "s : " asc "s) + R"sql(
                    limit ? offset ?
                )sql")
                .Bind(
                    pagination.TopHeight,
                    pagination.PageSize,
                    pagination.PageStart * pagination.PageSize
                );
            },
            [&] (Stmt& stmt) {
                stmt.Select([&](Cursor& cursor) {
                    while (cursor.Step())
                    {
                        int64_t contentId;
                        cursor.Collect<int64_t>(6, contentId);
                        int contentType;
                        cursor.Collect<int>(7, contentType);
                        int addressLikers;
                        cursor.Collect<int>(9, addressLikers);

                        UniValue record(UniValue::VOBJ);
                        cursor.Collect<int64_t>(0, record, "height");
                        cursor.Collect<string>(1, record, "juryid");
                        cursor.Collect<string>(2, record, "content_id");
                        record.pushKV("content_type", contentType);
                        cursor.Collect<string>(3, record, "address");
                        cursor.Collect<int>(4, record, "reason");
                        cursor.Collect<int>(5, record, "verdict");
                        cursor.Collect<int>(8, record, "votes");

                        result.push_back(JuryContent{contentId, (TxType)contentType, addressLikers, record});
                    }
                });
            }
        );

        return result;
    }

    vector<JuryContent> ModerationRepository::GetJuryAssigned(const string& address, bool verdict, const Pagination& pagination)
    {
        vector<JuryContent> result;

        SqlTransaction(
            __func__,
            [&]() -> Stmt& {
                return Sql(R"sql(
                    with
                    addr as (
                        select
                            r.RowId as id,
                            r.String as hash
                        from
                            Registry r
                        where
                            r.String = ?
                    )
                    select
                        (select r.String from Registry r where r.RowId = f.RowId) as FlagHash,
                        cf.Height as FlagHeight,
                        f.Int1 as Reason,
                        ifnull(v.Int1, -1),
                        ifnull(jv.Verdict, -1),
                        cc.Uid as ContentId,
                        c.Type as ContentType,
                        ifnull((
                            select
                                sum(lp.Value)
                            from Ratings lp indexed by Ratings_Type_Uid_Last_Value
                            where lp.Type in (111, 112, 113) and lp.Uid = fcu.Uid and lp.Last = 1
                        ),0) as likers
                    from
                        addr
                    cross join
                        Transactions u indexed by Transactions_Type_RegId1_RegId2_RegId3
                            on u.Type in (100) and u.RegId1 = addr.id
                    cross join
                        Last lu
                            on lu.TxId = u.RowId
                    cross join
                        Chain cu
                            on cu.TxId = u.RowId
                    cross join JuryModerators jm indexed by JuryModerators_AccountId_FlagRowId
                        on jm.AccountId = cu.Uid
                    cross join Transactions f
                        on f.RowId = jm.FlagRowId
                    cross join
                        Chain cf indexed by Chain_TxId_Height
                            on cf.TxId = f.RowId and cf.Height <= ?
                    cross join Transactions c
                        on c.RowId = f.RegId2
                    cross join
                        Chain cc
                            on cc.TxId = c.RowId
                    cross join Transactions fu on
                        fu.Type = 100 and fu.RegId1 = f.RegId3
                    cross join First ffu on
                        ffu.TxId = fu.RowId
                    cross join Chain fcu on
                        fcu.TxId = ffu.TxId

                    left join Transactions v indexed by Transactions_Type_RegId1_RegId2_RegId3
                        on v.Type in (420) and v.RegId1 = u.RegId1 and v.RegId2 = f.RowId and exists (select 1 from Chain cv where cv.TxId = v.RowId)
                    left join JuryVerdict jv
                        on jv.FlagRowId = jm.FlagRowId
                    where
                        (
                            v.RowId is )sql" + (verdict ? "not"s : ""s) + R"sql( null )sql" + (verdict ? "or"s : "and"s) + R"sql(
                            jv.FlagRowId is )sql" + (verdict ? "not"s : ""s) + R"sql( null
                        )
                    order by cf.Height )sql" + (pagination.OrderDesc ? " desc "s : " asc "s) + R"sql(
                    limit ? offset ?
                )sql")
                .Bind(
                    address,
                    pagination.TopHeight,
                    pagination.PageSize,
                    pagination.PageStart * pagination.PageSize
                );
            },
            [&] (Stmt& stmt) {
                stmt.Select([&](Cursor& cursor) {
                    while (cursor.Step())
                    {
                        UniValue record(UniValue::VOBJ);

                        cursor.Collect<string>(0, record, "juryid");
                        cursor.Collect<int64_t>(1, record, "height");
                        cursor.Collect<int>(2, record, "reason");
                        if (auto [ok, value] = cursor.TryGetColumnInt(3); ok && value > -1)
                            record.pushKV("vote", value);
                        if (auto [ok, value] = cursor.TryGetColumnInt(4); ok && value > -1)
                            record.pushKV("verdict", value);

                        int64_t contentId; int contentType; int addressLikers;
                        if (cursor.CollectAll(contentId, contentType, addressLikers)) {
                            result.push_back(JuryContent{contentId, (TxType)contentType, addressLikers, record});
                        }
                    }
                });
            }
        );

        return result;
    }

    UniValue ModerationRepository::GetJuryModerators(const string& jury)
    {
        UniValue result(UniValue::VARR);

        SqlTransaction(
            __func__,
            [&]() -> Stmt& {
                return Sql(R"sql(
                    with
                        flag as (
                            select
                                t.RowId as id,
                                r.String as hash
                            from
                                Registry r
                            cross join
                                Transactions t
                                    on t.RowId = r.RowId
                            where
                                r.String = ?
                        )
                    select
                        (select r.String from Registry r where r.RowId = u.RegId1) as address,
                        v.Int1 as vote
                    from
                        flag
                    cross join
                        JuryModerators jm
                            on jm.FlagRowId = flag.id
                    cross join
                        Chain c indexed by Chain_Uid_Height
                            on c.Uid = jm.AccountId
                    cross join
                        First f
                            on f.TxId = c.TxId
                    cross join
                        Transactions u
                            on u.RowId = f.TxId
                    left join
                        Transactions v on
                            v.Type = 420 and v.RegId1 = u.RegId1 and v.RegId2 = flag.id
                )sql")
                .Bind(jury);
            },
            [&] (Stmt& stmt) {
                stmt.Select([&](Cursor& cursor) {
                    while (cursor.Step())
                    {
                        UniValue record(UniValue::VOBJ);

                        cursor.Collect<string>(0, record, "address");
                        if (auto[ok, value] = cursor.TryGetColumnInt(1); ok)
                            record.pushKV("vote", value);

                        result.push_back(record);
                    }
                });
            }
        );

        return result;
    }

    UniValue ModerationRepository::GetBans(const string& address)
    {
        UniValue result(UniValue::VARR);

        SqlTransaction(
            __func__,
            [&]() -> Stmt& {
                return Sql(R"sql(
                    with
                    addr as (
                        select
                            r.RowId as id,
                            r.String as hash
                        from
                            Registry r
                        where
                            r.String = ?
                    )
                    select
                        (select r.String from Registry r where r.RowId = f.RowId) as JuryId,
                        (select r.String from Registry r where r.RowId = f.RegId2) as ContentId,
                        f.Int1 as Reason,
                        b.Ending
                    from
                        addr
                    cross join
                        Transactions u indexed by Transactions_Type_RegId1_RegId2_RegId3
                            on u.Type in (100) and u.RegId1 = addr.id
                    cross join
                        Last lu
                            on lu.TxId = u.RowId
                    cross join
                        Chain cu
                            on cu.TxId = u.RowId
                    cross join
                        JuryBan b indexed by JuryBan_AccountId_Ending
                            on b.AccountId = cu.Uid
                    cross join Transactions v
                        on v.RowId = b.VoteRowId
                    cross join Transactions f
                        on f.RowId = v.RegId2
                )sql")
                .Bind(address);
            },
            [&] (Stmt& stmt) {
                stmt.Select([&](Cursor& cursor) {
                    while (cursor.Step())
                    {
                        UniValue record(UniValue::VOBJ);
                        
                        if (auto[ok, value] = cursor.TryGetColumnString(0); ok)
                            record.pushKV("juryId", value);
                        if (auto[ok, value] = cursor.TryGetColumnString(1); ok)
                            record.pushKV("contentId", value);
                        if (auto[ok, value] = cursor.TryGetColumnInt(2); ok)
                            record.pushKV("reason", value);
                        if (auto[ok, value] = cursor.TryGetColumnInt64(3); ok)
                            record.pushKV("ending", value);
                        
                        result.push_back(record);
                    }
                });
            }
        );

        return result;
    }

    UniValue ModerationRepository::GetBadgeHistory(const string& address, BadgeType badge)
    {
        UniValue result(UniValue::VARR);

        SqlTransaction(
            __func__,
            [&]() -> Stmt& {
                return Sql(R"sql(
                    with
                        addr as (select r.RowId as id
                                from Registry r
                                where r.String in (?)),
                        badge as (select ? as value)
                    select
                        b.Badge,
                        b.Cancel,
                        b.Height
                    from
                        addr,
                        badge,
                        Transactions u
                    cross join
                        Last l on
                            l.TxId = u.RowId
                    cross join
                        Chain c on
                            c.TxId = u.RowId
                    cross join
                        Badges b indexed by Badges_Badge_Cancel_AccountId_Height on
                            b.Badge = badge.value and
                            b.Cancel in (0, 1) and
                            b.AccountId = c.Uid
                    where
                        u.Type = 100 and
                        u.RegId1 = addr.id
                    order by
                        b.Height desc
                )sql")
                .Bind(address, (int)badge);
            },
            [&] (Stmt& stmt) {
                stmt.Select([&](Cursor& cursor) {
                    while (cursor.Step())
                    {
                        UniValue record(UniValue::VOBJ);

                        if (auto[ok, value] = cursor.TryGetColumnInt(0); ok)
                            record.pushKV("badge", BadgeSet::BadgeTypeToString(value));
                        if (auto[ok, value] = cursor.TryGetColumnInt(1); ok)
                            record.pushKV("cancel", value);
                        if (auto[ok, value] = cursor.TryGetColumnInt(2); ok)
                            record.pushKV("height", value);
                        
                        result.push_back(record);
                    }
                });
            }
        );

        return result;
    }

}