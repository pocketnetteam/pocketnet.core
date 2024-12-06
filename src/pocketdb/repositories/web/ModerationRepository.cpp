// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include "pocketdb/repositories/web/ModerationRepository.h"

namespace PocketDb
{
    void ModerationRepository::Init() {}

    void ModerationRepository::Destroy() {}

    UniValue ModerationRepository::GetJury(const string& jury)
    {
        UniValue result(UniValue::VOBJ);

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
                    juryVerd as (
                        select
                            jv.Verdict
                        from
                            flag
                        cross join
                            juryVerdict jv
                                on jv.FlagRowId = flag.id
                    )
                    select
                        a.AddressHash,
                        j.Reason,
                        ifnull(jv.Verdict, -1)
                    from
                        juryRec j
                        join account a
                        left join juryVerd jv
                )sql")
                .Bind(jury);
            },
            [&] (Stmt& stmt) {
                stmt.Select([&](Cursor& cursor) {
                    if (cursor.Step())
                    {
                        result.pushKV("id", jury);
                        cursor.Collect<string>(0, result, "address");
                        cursor.Collect<int>(1, result, "reason");
                        if (auto [ok, value] = cursor.TryGetColumnInt(2); ok && value > -1)
                        {
                            result.pushKV("verdict", value);
                            // TODO (aok): add object with ban information if verditc == 1
                        }
                    }
                });
            }
        );

        return result;
    }

    UniValue ModerationRepository::GetAllJury(const Pagination& pagination)
    {
        UniValue result(UniValue::VARR);

        SqlTransaction(
            __func__,
            [&]() -> Stmt& {
                return Sql(R"sql(
                    select
                        cf.Height,
                        (select r.String from Registry r where r.RowId = f.RowId),
                        (select r.String from Registry r where r.RowId = f.RegId2),
                        c.Type,
                        (select r.String from Registry r where r.RowId = f.RegId3),
                        j.Reason,
                        ifnull(jv.Verdict, -1)
                    from
                        Jury j
                    cross join Transactions f on
                        f.RowId = j.FlagRowId
                    cross join Chain cf on
                        cf.TxId = f.RowId and cf.Height )sql" + (pagination.OrderDesc ? " <= "s : " > "s) + R"sql( ?
                    cross join Transactions c on
                        c.RowId = f.RegId2
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
                        UniValue rcrd(UniValue::VOBJ);

                        cursor.Collect<int>(0, rcrd, "height");
                        cursor.Collect<string>(1, rcrd, "id");
                        cursor.Collect<string>(2, rcrd, "content_id");
                        cursor.Collect<int>(3, rcrd, "content_type");
                        cursor.Collect<string>(4, rcrd, "address");
                        cursor.Collect<int>(5, rcrd, "reason");
                        cursor.Collect<int>(6, rcrd, "verdict");

                        result.push_back(rcrd);
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
                        c.Type as ContentType
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

                        int64_t contentId; int contentType;
                        if (cursor.CollectAll(contentId, contentType)) {
                            result.push_back(JuryContent{contentId, (TxType)contentType, record});
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
                        (select r.String from Registry r where r.RowId = u.RegId1)
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
                )sql")
                .Bind(jury);
            },
            [&] (Stmt& stmt) {
                stmt.Select([&](Cursor& cursor) {
                    while (cursor.Step())
                    {
                        if (auto[ok, value] = cursor.TryGetColumnString(0); ok)
                            result.push_back(value);
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

}