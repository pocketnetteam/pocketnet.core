// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include "pocketdb/repositories/web/WebRpcRepository.h"
#include "pocketdb/repositories/ConsensusRepository.h"
#include <functional>

namespace PocketDb
{
    UniValue WebRpcRepository::GetAddressId(const string& address)
    {
        UniValue result(UniValue::VOBJ);

        SqlTransaction(
            __func__,
            [&]() -> Stmt& {
                return Sql(R"sql(
                    with str1 as (
                        select
                            String as value,
                            RowId as id
                        from
                            Registry
                        where
                            String = ?
                    )
                    select
                        str1.value,
                        c.Uid
                    from
                        str1,
                        Transactions t
                        join Chain c on
                            c.TxId = t.RowId
                    where
                        t.Type in (100,170) and
                        t.RegId1 = str1.id and
                        exists (select 1 from Last l where l.TxId = t.RowId)
                )sql")
                .Bind(address);
            },
            [&](Stmt& stmt) {
                stmt.Select([&](Cursor& cursor) {
                    if (cursor.Step())
                    {
                        if (auto[ok, value] = cursor.TryGetColumnString(0); ok) result.pushKV("address", value);
                        if (auto[ok, value] = cursor.TryGetColumnInt64(1); ok) result.pushKV("id", value);
                    }
                });
            }
        );

        return result;
    }

    UniValue WebRpcRepository::GetAddressId(int64_t id)
    {
        UniValue result(UniValue::VOBJ);

        SqlTransaction(
            __func__,
            [&]() -> Stmt& {
                return Sql(R"sql(
                    select
                        s.String1,
                        c.Uid
                    from
                        Transactions t
                        join Chain c on
                            c.TxId = t.RowId and
                            c.Uid = ?
                        join vTxStr s on
                            s.RowId = t.RowId
                    where
                        t.Type in (100,170) and
                        exists (select 1 from Last l where l.TxId = t.RowId)
                )sql")
                .Bind(id);
            },
            [&](Stmt& stmt) {
                stmt.Select([&](Cursor& cursor) {
                    if (cursor.Step())
                    {
                        if (auto[ok, value] = cursor.TryGetColumnString(0); ok) result.pushKV("address", value);
                        if (auto[ok, value] = cursor.TryGetColumnInt64(1); ok) result.pushKV("id", value);
                    }
                });
            }
        );

        return result;
    }

    UniValue WebRpcRepository::GetUserAddress(const string& name)
    {
        UniValue result(UniValue::VARR);
        
        if (name.empty()) return result;

        auto _name = EscapeValue(name);

        SqlTransaction(
            __func__,
            [&]() -> Stmt& {
                return Sql(R"sql(
                    select
                        p.String2,
                        s.String1
                    from
                        Payload p indexed by Payload_String2_nocase
                        cross join Transactions u on
                            u.RowId = p.TxId and
                            u.Type = 100
                        cross join vTxStr s on
                            s.RowId = p.TxId
                    where
                        p.String2 like ? escape '\' and
                        -- Checking last against payload id for optimization reasons
                        exists(select 1 from Last l where l.TxId = p.TxId)
                    limit 1
                )sql")
                .Bind(_name);
            },
            [&] (Stmt& stmt) {
                stmt.Select([&](Cursor& cursor) {
                    while (cursor.Step())
                    {
                        UniValue record(UniValue::VOBJ);

                        cursor.Collect<string>(0, record, "name");
                        cursor.Collect<string>(1, record, "address");

                        result.push_back(record);
                    }
                });
            }
        );

        return result;
    }

    UniValue WebRpcRepository::GetAddressesRegistrationDates(const vector<string>& addresses)
    {
        auto result = UniValue(UniValue::VARR);

        if (addresses.empty())
            return result;

        SqlTransaction(
            __func__,
            [&]() -> Stmt& {
                return Sql(R"sql(
                    with addr as (
                        select
                            RowId as id,
                            String as hash
                        from
                            Registry
                        where
                            String in ( )sql" + join(vector<string>(addresses.size(), "?"), ",") + R"sql( )
                    )
                    select
                        addr.hash,
                        u.Time,
                        (select r.String from Registry r where r.RowId = u.RowId)
                    from
                        addr
                    cross join
                        Transactions u indexed by Transactions_Type_RegId1_RegId2_RegId3
                            on u.Type in (100) and u.RegId1 = addr.id
                    cross join
                        First f
                            on f.TxId = u.RowId
                )sql")
                .Bind(addresses);
            },
            [&] (Stmt& stmt) {
                stmt.Select([&](Cursor& cursor) {
                    while (cursor.Step())
                    {
                        UniValue record(UniValue::VOBJ);

                        cursor.Collect<string>(0, record, "address");
                        cursor.Collect<int64_t>(1, record, "time");
                        cursor.Collect<string>(2, record, "txid");

                        result.push_back(record);
                    }
                });
            }
        );

        return result;
    }

    UniValue WebRpcRepository::GetTopAddresses(int count)
    {
        UniValue result(UniValue::VARR);

        auto sql = R"sql(
            select
                (select String from Registry where RowId = AddressId),
                Value
            from
                Balances indexed by Balances_Value
            order by
                Value desc
            limit ?
        )sql";

        SqlTransaction(
            __func__,
            [&]() -> Stmt& {
                return Sql(sql)
                .Bind(count);
            },
            [&] (Stmt& stmt) {
                stmt.Select([&](Cursor& cursor) {
                    while (cursor.Step())
                    {
                        UniValue addr(UniValue::VOBJ);

                        cursor.Collect<string>(0, addr, "addresses");
                        cursor.Collect<int64_t>(1, addr, "balance");

                        result.push_back(addr);
                    }
                });
            }
        );

        return result;
    }

    UniValue WebRpcRepository::GetAccountState(const string& address, int heightWindow)
    {
        UniValue result(UniValue::VOBJ);

        SqlTransaction(
            __func__,
            [&]() -> Stmt& {
                return Sql(R"sql(
                    with
                    height as (
                        select ? as val
                    ),
                    addr as (
                        select
                            RowId as id,
                            String as hash
                        from
                            Registry
                        where
                            String = ?
                    )
                    select
                        cu.Uid as AddressId,
                        addr.hash as Address,
                        u.Type,

                        (
                            select count()
                            from Transactions p indexed by Transactions_Type_RegId1_RegId2_RegId3
                            left join Chain c indexed by Chain_TxId_Height on c.TxId = p.RowId
                            left join Mempool m on m.TxId = p.RowId
                            where p.Type in (200) and p.RegId1 = u.RegId1 and (c.Height >= height.val or m.TxId is not null)
                        ) as PostSpent,
                        (
                            select count()
                            from Transactions p indexed by Transactions_Type_RegId1_RegId2_RegId3
                            left join Chain c indexed by Chain_TxId_Height on c.TxId = p.RowId
                            left join Mempool m on m.TxId = p.RowId
                            where p.Type in (201) and p.RegId1 = u.RegId1 and (c.Height >= height.val or m.TxId is not null)
                        ) as VideoSpent,
                        (
                            select count()
                            from Transactions p indexed by Transactions_Type_RegId1_RegId2_RegId3
                            left join Chain c indexed by Chain_TxId_Height on c.TxId = p.RowId
                            left join Mempool m on m.TxId = p.RowId
                            where p.Type in (202) and p.RegId1 = u.RegId1 and (c.Height >= height.val or m.TxId is not null)
                        ) as ArticleSpent,
                        (
                            select count()
                            from Transactions p indexed by Transactions_Type_RegId1_RegId2_RegId3
                            left join Chain c indexed by Chain_TxId_Height on c.TxId = p.RowId
                            left join Mempool m on m.TxId = p.RowId
                            where p.Type in (209) and p.RegId1 = u.RegId1 and (c.Height >= height.val or m.TxId is not null)
                        ) as StreamSpent,
                        (
                            select count()
                            from Transactions p indexed by Transactions_Type_RegId1_RegId2_RegId3
                            left join Chain c indexed by Chain_TxId_Height on c.TxId = p.RowId
                            left join Mempool m on m.TxId = p.RowId
                            where p.Type in (210) and p.RegId1 = u.RegId1 and (c.Height >= height.val or m.TxId is not null)
                        ) as AudioSpent,
                        (
                            select count()
                            from Transactions p indexed by Transactions_Type_RegId1_RegId2_RegId3
                            left join Chain c indexed by Chain_TxId_Height on c.TxId = p.RowId
                            left join Mempool m on m.TxId = p.RowId
                            where p.Type in (204) and p.RegId1 = u.RegId1 and (c.Height >= height.val or m.TxId is not null)
                        ) as CommentSpent,
                        (
                            select count()
                            from Transactions p indexed by Transactions_Type_RegId1_RegId2_RegId3
                            left join Chain c indexed by Chain_TxId_Height on c.TxId = p.RowId
                            left join Mempool m on m.TxId = p.RowId
                            where p.Type in (300) and p.RegId1 = u.RegId1 and (c.Height >= height.val or m.TxId is not null)
                        ) as ScoreSpent,
                        (
                            select count()
                            from Transactions p indexed by Transactions_Type_RegId1_RegId2_RegId3
                            left join Chain c indexed by Chain_TxId_Height on c.TxId = p.RowId
                            left join Mempool m on m.TxId = p.RowId
                            where p.Type in (301) and p.RegId1 = u.RegId1 and (c.Height >= height.val or m.TxId is not null)
                        ) as ScoreCommentSpent,
                        (
                            select count()
                            from Transactions p indexed by Transactions_Type_RegId1_RegId2_RegId3
                            left join Chain c indexed by Chain_TxId_Height on c.TxId = p.RowId
                            left join Mempool m on m.TxId = p.RowId
                            where p.Type in (307) and p.RegId1 = u.RegId1 and (c.Height >= height.val or m.TxId is not null)
                        ) as ComplainSpent,
                        (
                            select count()
                            from Transactions p indexed by Transactions_Type_RegId1_RegId2_RegId3
                            left join Chain c indexed by Chain_TxId_Height on c.TxId = p.RowId
                            left join Mempool m on m.TxId = p.RowId
                            where p.Type in (410) and p.RegId1 = u.RegId1 and (c.Height >= height.val or m.TxId is not null)
                        ) as FlagsSpent
                    from
                        height,
                        addr
                    cross join
                        Transactions u indexed by Transactions_Type_RegId1_RegId2_RegId3
                            on u.Type in (100, 170) and u.RegId1 = addr.id
                    cross join
                        Last lu
                            on lu.TxId = u.RowId
                    cross join
                        Chain cu
                            on cu.TxId = u.RowId
                )sql")
                .Bind(heightWindow, address);
            },
            [&] (Stmt& stmt) {
                stmt.Select([&](Cursor& cursor) {
                    if (cursor.Step())
                    {
                        int i = 0;
                        cursor.Collect<int64_t>(i++, result, "address_id");
                        cursor.Collect<string>(i++, result, "address");

                        bool isDeleted = false;
                        cursor.Collect(i++, [&](int Type) {
                            isDeleted = (Type==TxType::ACCOUNT_DELETE);
                            if (isDeleted) result.pushKV("deleted", true);
                        });

                        if (!isDeleted) {
                            cursor.Collect<int64_t>(i++, result, "post_spent");
                            cursor.Collect<int64_t>(i++, result, "video_spent");
                            cursor.Collect<int64_t>(i++, result, "article_spent");
                            cursor.Collect<int64_t>(i++, result, "stream_spent");
                            cursor.Collect<int64_t>(i++, result, "audio_spent");
                            cursor.Collect<int64_t>(i++, result, "comment_spent");
                            cursor.Collect<int64_t>(i++, result, "score_spent");
                            cursor.Collect<int64_t>(i++, result, "comment_score_spent");
                            cursor.Collect<int64_t>(i++, result, "complain_spent");
                            cursor.Collect<int64_t>(i++, result, "mod_flag_spent");
                        }
                    }
                });
            }
        );

        return result;
    }

    UniValue WebRpcRepository::GetAccountSetting(const string& address)
    {
        string result;

        SqlTransaction(
            __func__,
            [&]() -> Stmt& {
                return Sql(R"sql(
                    with
                    addr as (
                        select
                            r.RowId as id
                        from
                            Registry r
                        where
                            r.String = ?
                    )
                    select
                        p.String1
                    from
                        addr
                        join Transactions t indexed by Transactions_Type_RegId1_RegId2_RegId3 on
                            t.Type in (103) and t.RegId1 = addr.id
                        join Last l on
                            l.TxId = t.RowId
                        join Payload p on
                            p.TxId = t.RowId
                    limit 1
                )sql")
                .Bind(address);
            },
            [&] (Stmt& stmt) {
                stmt.Select([&](Cursor& cursor) {
                    while (cursor.Step())
                    {
                        if (auto[ok, value] = cursor.TryGetColumnString(0); ok)
                            result = value;
                    }
                });
            }
        );

        return result;
    }

    UniValue WebRpcRepository::GetUserStatistic(const vector<string>& addresses, const int nHeight, const int depthR, const int depthC, const int cntC)
    {
        UniValue result(UniValue::VARR);
        if (addresses.empty())
            return  result;

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
                            r.String in ( )sql" + join(vector<string>(addresses.size(), "?"), ",") + R"sql( )
                    )
                    select
                        addr.hash,

                        ifnull((
                            select
                                count()
                            from Transactions ref
                            cross join First f on
                                f.TxId = ref.RowId
                            cross join Chain c on
                                c.TxId = ref.RowId and
                                c.Height <= ? and
                                c.Height > ?
                            where ref.Type in (100) and
                                ref.RegId2 = addr.id
                        ), 0) as ReferralsCountHist,

                        (
                            select
                                count()
                            from (
                                select
                                    c.RegId1
                                from Transactions p indexed by Transactions_Type_RegId1_RegId2_RegId3
                                cross join Last lp on
                                    lp.TxId = p.RowId
                                cross join Transactions c indexed by Transactions_Type_RegId3_RegId1 on
                                    c.Type in (204) and c.RegId3 = p.RegId2 and c.RegId1 != addr.id
                                cross join First fc on
                                    fc.TxId = c.RowId
                                cross join Chain cc indexed by Chain_TxId_Height on
                                    cc.TxId = c.RowId and cc.Height <= ? and cc.Height > ?
                                where
                                    p.Type in (200,201,202,209,210) and
                                    p.RegId1 = addr.id
                                group by c.RegId1
                                having count() > ?
                            )
                        ) as CommentatorsCountHist

                    from
                        addr
                        join Transactions t indexed by Transactions_Type_RegId1_RegId2_RegId3 on
                            t.Type in (100) and t.RegId1 = addr.id
                        join Last l on
                            l.TxId = t.RowId
                )sql")
                .Bind(addresses, nHeight, nHeight - depthR, nHeight, nHeight - depthC, cntC);
            },
            [&] (Stmt& stmt) {
                stmt.Select([&](Cursor& cursor) {
                    if (cursor.Step())
                    {
                        UniValue record(UniValue::VOBJ);
                        auto[ok0, address] = cursor.TryGetColumnString(0);
                        auto[ok1, ReferralsCountHist] = cursor.TryGetColumnInt(1);
                        auto[ok2, CommentatorsCountHist] = cursor.TryGetColumnInt(2);

                        record.pushKV("address", address);
                        record.pushKV("histreferals", ReferralsCountHist);
                        record.pushKV("commentators", CommentatorsCountHist);
                        result.push_back(record);
                    }
                });
            }
        );

        return result;
    }

    vector<tuple<string, int64_t, UniValue>> WebRpcRepository::GetAccountProfiles(
        const vector<string>& addresses,
        const vector<int64_t>& ids,
        bool shortForm,
        int firstFlagsDepth)
    {
        vector<tuple<string, int64_t, UniValue>> result{};

        if (addresses.empty() && ids.empty())
            return result;
        if (!addresses.empty() && !ids.empty())
            return result;
        
        string sql;

        if (!addresses.empty())
        {
            sql = R"sql(
                with
                firstFlagsDepth as ( select (? * 1440) as value ),
                addr as (
                    select
                        r.RowId as id,
                        r.String as hash
                    from
                        Registry r
                    where
                        r.String in ( )sql" + join(vector<string>(addresses.size(), "?"), ",") + R"sql( )
                )
            )sql";
        }

        if (!ids.empty())
        {
            sql = R"sql(
                with
                firstFlagsDepth as ( select (? * 1440) as value ),
                addr as (
                    select
                        r.RowId as id,
                        r.String as hash
                    from
                        Chain c
                    cross join
                        Transactions t
                            on t.RowId = c.TxId
                    cross join
                        First f
                            on f.TxId = t.RowId
                    cross join
                        Registry r
                            on r.RowId = t.RegId1
                    where
                        c.Uid in ( )sql" + join(vector<string>(ids.size(), "?"), ",") + R"sql( )
                )
            )sql";
        }

        sql += GetAccountProfilesSqlShort();

        string fullPart = "";
        if (!shortForm)
            fullPart = GetAccountProfilesSqlFull();
        sql = sql.replace(sql.find("<FULLPART>"), 10, fullPart);

        SqlTransaction(
            __func__,
            [&]() -> Stmt& {
                return
                    Sql(sql)
                    .Bind(firstFlagsDepth, addresses, ids);
            },
            [&] (Stmt& stmt) {
                stmt.Select([&](Cursor& cursor) {
                    // Fetch data
                    while (cursor.Step())
                    {
                        UniValue record(UniValue::VOBJ);

                        int i = 0;
                        auto[ok0, hash] = cursor.TryGetColumnString(i++);
                        auto[ok1, address] = cursor.TryGetColumnString(i++);
                        auto[ok2, id] = cursor.TryGetColumnInt64(i++);
                        auto[ok3, Type] = cursor.TryGetColumnInt(i++);
                        bool isDeleted = (Type==TxType::ACCOUNT_DELETE);

                        record.pushKV("hash", hash);
                        record.pushKV("address", address);
                        record.pushKV("id", id);
                        if (IsDeveloper(address)) record.pushKV("dev", true);
                        if (isDeleted) record.pushKV("deleted", true);

                        if(!isDeleted) {
                            if (auto [ok, value] = cursor.TryGetColumnString(i++); ok) record.pushKV("name", value);
                            if (auto [ok, value] = cursor.TryGetColumnString(i++); ok) record.pushKV("i", value);
                            if (auto [ok, value] = cursor.TryGetColumnString(i++); ok) record.pushKV("b", value);
                            if (auto [ok, value] = cursor.TryGetColumnString(i++); ok) record.pushKV("r", value);
                            if (auto [ok, value] = cursor.TryGetColumnInt(i++); ok) record.pushKV("postcnt", value);
                            if (auto [ok, value] = cursor.TryGetColumnInt(i++); ok) record.pushKV("dltdcnt", value);
                            if (auto [ok, value] = cursor.TryGetColumnInt(i++); ok) record.pushKV("reputation", value / 10.0);
                            if (auto [ok, value] = cursor.TryGetColumnInt(i++); ok) record.pushKV("subscribes_count", value);
                            if (auto [ok, value] = cursor.TryGetColumnInt(i++); ok) record.pushKV("subscribers_count", value);
                            if (auto [ok, value] = cursor.TryGetColumnInt(i++); ok) record.pushKV("blockings_count", value);
                            if (auto [ok, value] = cursor.TryGetColumnInt(i++); ok) record.pushKV("blockers_count", value);
                            if (auto [ok, value] = cursor.TryGetColumnInt(i++); ok) record.pushKV("likers_count", value);
                            if (auto [ok, value] = cursor.TryGetColumnString(i++); ok) record.pushKV("k", value);
                            if (auto [ok, value] = cursor.TryGetColumnString(i++); ok) record.pushKV("a", value);
                            if (auto [ok, value] = cursor.TryGetColumnString(i++); ok) record.pushKV("l", value);
                            if (auto [ok, value] = cursor.TryGetColumnString(i++); ok) record.pushKV("s", value);
                            if (auto [ok, value] = cursor.TryGetColumnInt64(i++); ok) record.pushKV("update", value);
                            if (auto [ok, value] = cursor.TryGetColumnInt64(i++); ok) record.pushKV("regdate", value);

                            if (auto [ok, value] = cursor.TryGetColumnString(i++); ok) {
                                UniValue flags(UniValue::VOBJ);
                                flags.read(value);
                                record.pushKV("flags", flags);
                            }

                            if (auto [ok, value] = cursor.TryGetColumnString(i++); ok) {
                                UniValue flags(UniValue::VOBJ);
                                flags.read(value);
                                record.pushKV("firstFlags", flags);
                            }

                            if (auto [ok, value] = cursor.TryGetColumnInt64(i++); ok)
                                record.pushKV("actions", value);

                            if (auto [ok, value] = cursor.TryGetColumnString(i++); ok) {
                                UniValue flags(UniValue::VOBJ);
                                flags.read(value);
                                record.pushKV("bans", flags);
                            }

                            if (auto [ok, value] = cursor.TryGetColumnString(i++); ok) {
                                UniValue badges(UniValue::VARR);
                                if (badges.read(value) && badges.isArray()) {
                                    BadgeSet badgeSet;
                                    badgeSet.Developer = IsDeveloper(address);

                                    for (unsigned int i = 0; i < badges.size(); i++)
                                    {
                                        if (!badges[i].isNum())
                                            continue;

                                        badgeSet.Set(badges[i].get_int());
                                    }

                                    record.pushKV("badges", badgeSet.ToJson());
                                }
                            }

                            if (!shortForm) {

                                if (auto [ok, value] = cursor.TryGetColumnString(i++); ok) {
                                    UniValue subscribes(UniValue::VARR);
                                    subscribes.read(value);
                                    record.pushKV("subscribes", subscribes);
                                }

                                if (auto [ok, value] = cursor.TryGetColumnString(i++); ok) {
                                    UniValue subscribes(UniValue::VARR);
                                    subscribes.read(value);
                                    record.pushKV("subscribers", subscribes);
                                }

                                if (auto [ok, value] = cursor.TryGetColumnString(i++); ok) {
                                    UniValue blockings(UniValue::VARR);
                                    blockings.read(value);
                                    record.pushKV("blocking", blockings);
                                }

                                if (auto [ok, value] = cursor.TryGetColumnString(i++); ok) {
                                    UniValue content(UniValue::VOBJ);
                                    content.read(value);
                                    record.pushKV("content", content);
                                }
                            }
                        }

                        result.emplace_back(address, id, record);
                    }
                });
            }
        );

        return result;
    }

    string WebRpcRepository::GetAccountProfilesSqlFull()
    {
        return R"sql(
            ,(
                select
                    json_group_array(
                        json_object(
                            'adddress', rsubs.String,
                            'private', case when subs.Type == 303 then 'true' else 'false' end
                        )
                    )
                from Transactions subs indexed by Transactions_Type_RegId1_RegId2_RegId3
                cross join Last lsubs
                    on lsubs.TxId = subs.RowId
                cross join Transactions uas indexed by Transactions_Type_RegId1_RegId2_RegId3
                on uas.Type = 100 and uas.RegId1 = subs.RegId2
                cross join Last luas
                    on luas.TxId = uas.RowId
                cross join Registry rsubs
                    on rsubs.RowId = subs.RegId2
                where
                    subs.Type in (302, 303) and
                    subs.RegId1 = addr.id
            ) as Subscribes
            ,(
                select
                    json_group_array(rsubs.String)
                from Transactions subs indexed by Transactions_Type_RegId2_RegId1
                cross join Last lsubs
                    on lsubs.TxId = subs.RowId
                cross join Transactions uas indexed by Transactions_Type_RegId1_RegId2_RegId3
                on uas.Type in (100) and uas.RegId1 = subs.RegId1
                cross join Last luas
                    on luas.TxId = uas.RowId
                cross join Registry rsubs
                    on rsubs.RowId = subs.RegId1
                where
                    subs.Type in (302, 303) and
                    subs.RegId2 = addr.id
            ) as Subscribers
            ,(
                select
                    json_group_array(rub.String)
                from
                    BlockingLists bl
                cross join
                    Registry rub on
                        rub.RowId = bl.IdTarget
                where
                    bl.IdSource = u.RegId1
            ) as Blockings
            ,(
                select json_group_object(gr.Type, gr.Cnt)
                from (
                    select
                        f.Type as Type,
                        count() as Cnt
                    from Transactions f indexed by Transactions_Type_RegId1_RegId2_RegId3
                    join Last lf
                        on lf.TxId = f.RowId
                    where
                        f.Type in (200,201,202,209,210,220,207) and f.RegId1 = addr.id
                    group by
                        f.Type
                )gr
            ) as ContentJson
        )sql";
    }

    string WebRpcRepository::GetAccountProfilesSqlShort()
    {
        return R"sql(
            select

                (select r.String from Registry r where r.RowId = u.RowId) as AccountTxHash
                ,(select r.String from Registry r where r.RowId = u.RegId1) as AddressHash
                ,cu.Uid
                ,u.Type
                ,ifnull(p.String2,'') as Name
                ,ifnull(p.String3,'') as Avatar
                ,ifnull(p.String7,'') as Donations
                ,ifnull((select r.String from Registry r where r.RowId = u.RegId2),'') as Referrer
                ,ifnull((select Data from web.AccountStatistic a where a.AccountRegId = addr.id and a.Type = 1), 0) as PostsCount
                ,ifnull((select Data from web.AccountStatistic a where a.AccountRegId = addr.id and a.Type = 2), 0) as DelCount
                ,ifnull(r.Value, 0) as Reputation
                ,ifnull((select Data from web.AccountStatistic a where a.AccountRegId = addr.id and a.Type = 3), 0) as SubscribesCount
                ,ifnull((select Data from web.AccountStatistic a where a.AccountRegId = addr.id and a.Type = 4), 0) as SubscribersCount
                ,(
                    select
                        count()
                    from
                        BlockingLists bl
                    where
                        bl.IdSource = u.RegId1
                ) as BlockingsCount
                ,(
                    select
                        count()
                    from
                        BlockingLists bl
                    where
                        bl.IdTarget = u.RegId1
                ) as BlockersCount
                ,ifnull((
                    select
                        sum(lkr.Value)
                    from Ratings lkr indexed by Ratings_Type_Uid_Last_Value
                    where
                        lkr.Type in (111,112,113) and lkr.Uid = cu.Uid and lkr.Last = 1
                ), 0) as Likers
                ,ifnull(p.String6,'') as Pubkey
                ,ifnull(p.String4,'') as About
                ,ifnull(p.String1,'') as Lang
                ,ifnull(p.String5,'') as Url
                ,u.Time
                ,reg.Time as RegistrationDate
                ,ifnull((select Data from web.AccountStatistic a where a.AccountRegId = addr.id and a.Type = 5), '{}') as FlagsJson
                ,ifnull((select Data from web.AccountStatistic a where a.AccountRegId = addr.id and a.Type = 6), '{}') as FirstFlagsCount
                ,ifnull((select Data from web.AccountStatistic a where a.AccountRegId = addr.id and a.Type = 7), 0) as ActionsCount
                ,(select json_group_object((select r.String from Registry r where r.RowId = jb.VoteRowId), jb.Ending) from JuryBan jb where jb.AccountId = cu.Uid) as Bans,
                (
                    select
                        json_group_array(b.Badge)
                    from Badges b
                    where
                        b.AccountId = cu.Uid and
                        b.Cancel = 0
                    order by
                        b.Height desc
                ) as badges
                <FULLPART>
            from
                addr,
                firstFlagsDepth
            cross join
                Transactions u indexed by Transactions_Type_RegId1_RegId2_RegId3 on
                    u.Type in (100, 170) and
                    u.RegId1 = addr.id
            cross join
                Last lu on
                    lu.TxId = u.RowId
            cross join
                Chain cu on
                    cu.TxId = u.RowId
            cross join
                Transactions reg indexed by Transactions_Type_RegId1_RegId2_RegId3 on
                    reg.Type in (100) and
                    reg.RegId1 = addr.id
            cross join
                First freg on
                    freg.TxId = reg.RowId
            left join
                Payload p on
                    p.TxId = u.RowId
            left join
                Ratings r indexed by Ratings_Type_Uid_Last_Value on
                    r.Type = 0 and
                    r.Uid = cu.Uid and
                    r.Last = 1     
        )sql";
    }

    map<string, UniValue> WebRpcRepository::GetAccountProfiles(const vector<string>& addresses, bool shortForm, int firstFlagsDepth)
    {
        map<string, UniValue> result{};

        auto _result = GetAccountProfiles(addresses, {}, shortForm, firstFlagsDepth);
        for (auto const& [address, id, record] : _result)
            result.insert_or_assign(address, record);

        return result;
    }

    map<int64_t, UniValue> WebRpcRepository::GetAccountProfiles(const vector<int64_t>& ids, bool shortForm, int firstFlagsDepth)
    {
        map<int64_t, UniValue> result{};

        auto _result = GetAccountProfiles({}, ids, shortForm, firstFlagsDepth);
        for (auto const& [address, id, record] : _result)
            result.insert_or_assign(id, record);

        return result;
    }

    UniValue WebRpcRepository::GetLastComments(int count, int height, const string& lang)
    {
        auto result = UniValue(UniValue::VARR);

        SqlTransaction(
            __func__,
            [&]() -> Stmt& {
                return Sql(R"sql(
                    select
                        (select r.String from Registry r where r.RowId = t.RegId2) as CommentTxHash,
                        (select r.String from Registry r where r.RowId = p.RegId2) as ContentTxHash,
                        (select r.String from Registry r where r.RowId = t.RegId1) as CommentAddress,
                        t.Time as CommentTime,
                        c.Height as CommentHeight,
                        pc.String1 as CommentMessage,
                        (select r.String from Registry r where r.RowId = t.RegId4) as CommentParent,
                        (select r.String from Registry r where r.RowId = t.RegId5) as CommentAnswer,
                        (select r.String from Registry r where r.RowId = p.RegId1) as AddressContent,
                        ifnull((select (select r.String from Registry r where r.RowId = tt.RegId1) from Transactions tt where tt.RowId = t.RegId4), '') as AddressCommentParent,
                        ifnull((select (select r.String from Registry r where r.RowId = tt.RegId1) from Transactions tt where tt.RowId = t.RegId5), '') as AddressCommentAnswer,

                        (
                            select count()
                            from Transactions sc indexed by Transactions_Type_RegId2_RegId1
                            cross join Chain csc on csc.TxId = sc.RowId
                            where sc.Type = 301 and sc.RegId2 = t.RegId2 and sc.Int1 = 1
                        ) as ScoreUp,

                        (
                            select count()
                            from Transactions sc indexed by Transactions_Type_RegId2_RegId1
                            cross join Chain csc on csc.TxId = sc.RowId
                            where sc.Type = 301 and sc.RegId2 = t.RegId2 and sc.Int1 = -1
                        ) as ScoreDown,

                        rc.Value    as CommentRating,

                        (case when t.RowId != t.RegId2 then 1 else 0 end) as CommentEdit,

                        (
                            select o.Value
                            from TxOutputs o indexed by TxOutputs_AddressId_TxIdDesc_Number
                            where o.TxId = t.RowId and o.AddressId = p.RegId1 and o.AddressId != t.RegId1
                            limit 1
                        ) as Donate

                    from
                        Chain c

                    -- Comment
                    cross join
                        Transactions t
                            on t.RowId = c.TxId and t.Type in (204, 205)
                    cross join
                        Last lt
                            on lt.TxId = t.RowId
                    cross join
                        Payload pc
                            on pc.TxId = t.RowId
                    cross join
                        Ratings rc indexed by Ratings_Type_Uid_Last_Value
                            on rc.Type = 3 and rc.Last = 1 and rc.Uid = c.Uid and rc.Value >= 0

                    -- Author of comment
                    cross join
                        Transactions ua indexed by Transactions_Type_RegId1_RegId2_RegId3
                            on ua.Type = 100 and ua.RegId1 = t.RegId1
                    cross join
                        Last lua
                            on lua.TxId = ua.RowId

                    -- Content
                    cross join
                        Transactions p indexed by Transactions_Type_RegId2_RegId1 on
                            p.Type in (200, 201, 202, 209, 210) and p.RegId2 = t.RegId3
                    cross join
                        Last lp
                            on lp.TxId = p.RowId
                    cross join
                        Chain cp
                            on cp.TxId = p.RowId
                    cross join
                        Payload pp
                            on pp.TxId = p.RowId and pp.String1 = ?

                    where
                        c.Height > (? - 600) and
                        not exists (select 1 from BlockingLists bl where bl.IdSource = p.RegId1 and bl.IdTarget = t.RegId1) and
                        not exists (select 1 from BlockingLists bl where bl.IdSource = t.RegId1 and bl.IdTarget = p.RegId1)

                    order by c.Height desc
                    limit ?
                )sql")
                .Bind(lang, height, count);
            },
            [&] (Stmt& stmt) {
                stmt.Select([&](Cursor& cursor) {
                    while (cursor.Step())
                    {
                        UniValue record(UniValue::VOBJ);

                        int i = 0;
                        if (auto[ok, value] = cursor.TryGetColumnString(i++); ok) record.pushKV("id", value);
                        if (auto[ok, value] = cursor.TryGetColumnString(i++); ok) record.pushKV("postid", value);
                        if (auto[ok, value] = cursor.TryGetColumnString(i++); ok) record.pushKV("address", value);
                        if (auto[ok, value] = cursor.TryGetColumnString(i++); ok)
                        {
                            record.pushKV("time", value);
                            record.pushKV("timeUpd", value);
                        }
                        if (auto[ok, value] = cursor.TryGetColumnString(i++); ok) record.pushKV("block", value);
                        if (auto[ok, value] = cursor.TryGetColumnString(i++); ok) record.pushKV("msg", value);
                        if (auto[ok, value] = cursor.TryGetColumnString(i++); ok) record.pushKV("parentid", value);
                        if (auto[ok, value] = cursor.TryGetColumnString(i++); ok) record.pushKV("answerid", value);
                        if (auto[ok, value] = cursor.TryGetColumnString(i++); ok) record.pushKV("addressContent", value);
                        if (auto[ok, value] = cursor.TryGetColumnString(i++); ok) record.pushKV("addressCommentParent", value);
                        if (auto[ok, value] = cursor.TryGetColumnString(i++); ok) record.pushKV("addressCommentAnswer", value);
                        if (auto[ok, value] = cursor.TryGetColumnInt(i++); ok) record.pushKV("scoreUp", value);
                        if (auto[ok, value] = cursor.TryGetColumnInt(i++); ok) record.pushKV("scoreDown", value);
                        if (auto[ok, value] = cursor.TryGetColumnInt(i++); ok) record.pushKV("reputation", value);
                        if (auto[ok, value] = cursor.TryGetColumnInt(i++); ok) record.pushKV("edit", value == 1);
                        if (auto[ok, value] = cursor.TryGetColumnInt64(i++); ok)
                        {
                            record.pushKV("donation", "true");
                            record.pushKV("amount", value);
                        }

                        result.push_back(record);
                    }
                });
            }
        );

        return result;
    }

    map<int64_t, UniValue> WebRpcRepository::GetLastComments(const vector<int64_t>& ids, const string& address)
    {
        map<int64_t, UniValue> result;

        string sql = R"sql(
            select
                cmnt.contentId,
                cc.Uid                                                                      as commentId,
                c.Type,
                (select r.String from Registry r where r.RowId = c.RegId2)                  as RootTxHash,
                (select r.String from Registry r where r.RowId = c.RegId3)                  as PostTxHash,
                (select r.String from Registry r where r.RowId = c.RegId1)                  as AddressHash,
                (select corig.Time from Transactions corig where corig.RowId = c.RegId2)    as TimeOrigin,
                c.Time                                                                      as TimeUpdate,
                cc.Height,
                (select p.String1 from Payload p where p.TxId = c.RowId)                    as Message,
                (select r.String from Registry r where r.RowId = c.RegId4)                  as ParentTxHash,
                (select r.String from Registry r where r.RowId = c.RegId5)                  as AnswerTxHash,

                (select count() from Transactions sc indexed by Transactions_Type_RegId2_RegId1
                    join Chain csc on csc.TxId = sc.RowId
                    where sc.Type=301 and sc.RegId2 = c.RowId and sc.Int1 = 1)              as ScoreUp,

                (select count() from Transactions sc indexed by Transactions_Type_RegId2_RegId1
                    join Chain csc on csc.TxId = sc.RowId
                    where sc.Type=301 and sc.RegId2 = c.RowId and sc.Int1 = -1)             as ScoreDown,

                (select r.Value from Ratings r indexed by Ratings_Type_Uid_Last_Height
                    where r.Uid = c.RowId AND r.Type=3 and r.Last=1)                        as Reputation,

                (select count() from Transactions ch indexed by Transactions_Type_RegId4_RegId1
                    join Chain cch on cch.TxId = ch.RowId
                    where ch.Type in (204,205,206) and ch.RegId4 = c.RegId2)                as ChildrensCount,

                ifnull((
                    select scr.Int1
                    from Transactions scr indexed by Transactions_Type_RegId1_RegId2_RegId3
                    join Chain cscr on cscr.TxId = scr.RowId
                    where scr.Type = 301
                        and scr.RegId1 = (select RowId from Registry where String = ?)
                        and scr.RegId2 = c.RegId2
                ), 0) as MyScore,

                (
                    select
                        o.Value
                    from
                        TxOutputs o
                    where
                        o.TxId = c.RowId and
                        o.AddressId = cmnt.ContentAddressId and
                        o.AddressId != c.RegId1
                    limit 1
                ) as Donate,
                (select 1 from BlockingLists bl where bl.IdSource = cmnt.ContentAddressId and bl.IdTarget = c.RegId1 limit 1)ContentBlockedComment,
                (select 1 from BlockingLists bl where bl.IdSource = c.RegId1 and bl.IdTarget = cmnt.ContentAddressId limit 1)CommentBlockedContent,
                (
                    select
                        json_group_object(ff.reason, ff.cnt)
                    from (
                        select
                            f.Int1 as reason,
                            count() as cnt
                        from
                            Transactions f indexed by Transactions_Type_RegId2_RegId1
                        where
                            f.Type in (410) and
                            f.RegId2 = c.RowId
                        group by f.Int1
                    ) ff
                ) as Flags

            from (
                select

                    c.Uid    as contentId,
                    t.RegId1 as ContentAddressId,

                    -- Last comment for content record
                    (
                        select
                            c1.RowId
                        from Transactions c1 indexed by Transactions_Type_RegId3_RegId1
                        cross join Chain cc1 on cc1.TxId = c1.RowId
                        cross join Last lc1 on lc1.TxId = c1.RowId
                        cross join Transactions uac indexed by Transactions_Type_RegId1_RegId2_RegId3
                          on uac.Type = 100 and uac.RegId1 = c1.RegId1
                        cross join Chain cuac on cuac.TxId = uac.RowId
                        cross join Last luac on luac.TxId = uac.RowId
                        left join TxOutputs o
                            on o.TxId = c1.RowId and o.AddressId = t.RegId1 and o.AddressId != c1.RegId1 and o.Value > ?
                        left join
                            BlockingLists bl_cnt_cmt on
                                bl_cnt_cmt.IdSource = t.RegId1 and bl_cnt_cmt.IdTarget = c1.RegId1
                        left join
                            BlockingLists bl_cmt_cnt on
                                bl_cmt_cnt.IdSource = c1.RegId1 and bl_cmt_cnt.IdTarget = t.RegId1
                        where c1.Type in (204, 205)
                          and c1.RegId3 = t.RegId2
                          and c1.RegId4 is null
                          
                        order by bl_cnt_cmt.IdSource, bl_cmt_cnt.IdSource, o.Value desc, c1.RowId desc

                        limit 1
                    )commentRowId

                from Chain c indexed by Chain_Uid_Height
                cross join Transactions t on t.RowId = c.TxId
                cross join Last l on l.TxId = t.RowId
                cross join Transactions ua indexed by Transactions_Type_RegId1_RegId2_RegId3
                  on ua.RegId1 = t.RegId1 and ua.Type = 100
                cross join Chain cua on cua.TxId = ua.RowId
                cross join Last lua on lua.TxId = ua.RowId

                where c.Uid in ( )sql" + join(vector<string>(ids.size(), "?"), ",") + R"sql( )

            ) cmnt

            join Transactions c on c.RowId = cmnt.commentRowId
            join Chain cc on cc.TxId = c.RowId
        )sql";

        SqlTransaction(
            __func__,
            [&]() -> Stmt& {
                return Sql(sql)
                .Bind(address, (int64_t)(0.5 * COIN), ids);
            },
            [&] (Stmt& stmt) {
                stmt.Select([&](Cursor& cursor) {
                    // ---------------------------
                    while (cursor.Step())
                    {
                        UniValue record(UniValue::VOBJ);

                        auto[okContentId, contentId] = cursor.TryGetColumnInt(0);
                        auto[okCommentId, commentId] = cursor.TryGetColumnInt(1);
                        auto[okType, txType] = cursor.TryGetColumnInt(2);
                        auto[okRoot, rootTxHash] = cursor.TryGetColumnString(3);

                        record.pushKV("id", rootTxHash);
                        record.pushKV("cid", commentId);
                        record.pushKV("edit", (TxType)txType == CONTENT_COMMENT_EDIT);
                        record.pushKV("deleted", (TxType)txType == CONTENT_COMMENT_DELETE);

                        if (auto[ok, value] = cursor.TryGetColumnString(4); ok) record.pushKV("postid", value);
                        if (auto[ok, value] = cursor.TryGetColumnString(5); ok) record.pushKV("address", value);
                        if (auto[ok, value] = cursor.TryGetColumnInt64(6); ok) record.pushKV("time", value);
                        if (auto[ok, value] = cursor.TryGetColumnInt64(7); ok) record.pushKV("timeUpd", value);
                        if (auto[ok, value] = cursor.TryGetColumnInt(8); ok) record.pushKV("block", value);
                        if (auto[ok, value] = cursor.TryGetColumnString(9); ok) record.pushKV("msg", value);
                        if (auto[ok, value] = cursor.TryGetColumnString(10); ok) record.pushKV("parentid", value);
                        if (auto[ok, value] = cursor.TryGetColumnString(11); ok) record.pushKV("answerid", value);
                        if (auto[ok, value] = cursor.TryGetColumnInt(12); ok) record.pushKV("scoreUp", value);
                        if (auto[ok, value] = cursor.TryGetColumnInt(13); ok) record.pushKV("scoreDown", value);
                        if (auto[ok, value] = cursor.TryGetColumnString(14); ok) record.pushKV("reputation", value);
                        if (auto[ok, value] = cursor.TryGetColumnInt(15); ok) record.pushKV("children", value);
                        if (auto[ok, value] = cursor.TryGetColumnInt(16); ok && !address.empty()) record.pushKV("myScore", value);
                        if (auto[ok, value] = cursor.TryGetColumnInt64(17); ok)
                        {
                            record.pushKV("donation", "true");
                            record.pushKV("amount", value);
                        }
                        if (auto[ok, value] = cursor.TryGetColumnInt(18); ok && value > 0)
                            record.pushKV("blck_cnt_cmt", 1);
                        if (auto[ok, value] = cursor.TryGetColumnInt(19); ok && value > 0)
                            record.pushKV("blck_cmt_cnt", 1);

                        if (auto[ok, value] = cursor.TryGetColumnString(20); ok)
                        {
                            UniValue flags(UniValue::VOBJ);
                            flags.read(value);
                            record.pushKV("flags", flags);
                        };
                                        
                        result.emplace(contentId, record);
                    }
                });
            }
        );

        return result;
    }

    vector<string> WebRpcRepository::GetContentComments(const vector<string>& contentHashes)
    {
        vector<string> result;
        result.reserve(contentHashes.size());

        SqlTransaction(__func__, [&]()
        {
            Sql(R"sql(
                with
                cnt as (
                    select
                        r.RowId as id
                    from
                        Registry r
                    where
                        r.String in ( )sql" + join(vector<string>(contentHashes.size(), "?"), ",") + R"sql( )
                )
                select
                    r.String
                from
                    cnt,
                    Transactions t indexed by Transactions_Type_RegId3_RegId4_RegId5
                    cross join Last l on
                        l.TxId = t.RowId
                    cross join Registry r on
                        r.RowId = t.RowId
                where
                    t.Type in (204, 205, 206) and
                    t.RegId3 = cnt.id
            )sql")
            .Bind(contentHashes)
            .Select([&](Cursor& cursor) {
                while (cursor.Step()) {
                    if (auto[ok, val] = cursor.TryGetColumnString(0); ok) {
                        result.emplace_back(val);
                    }
                }
            });
        });

        return result;
    }

    vector<string> WebRpcRepository::GetContentScores(const vector<string>& contentHashes)
    {
        vector<string> result;
        result.reserve(contentHashes.size());

        SqlTransaction(__func__, [&]()
        {
            Sql(R"sql(
                with
                cnt as (
                    select
                        r.RowId as id
                    from
                        Registry r
                    where
                        r.String in ( )sql" + join(vector<string>(contentHashes.size(), "?"), ",") + R"sql( )
                )
                select
                    r.String
                from
                    cnt,
                    Transactions t
                    cross join Registry r on
                        r.RowId = t.RowId
                where
                    t.Type = 300 and
                    t.RegId2 = cnt.id
            )sql")
            .Bind(contentHashes)
            .Select([&](Cursor& cursor) {
                while (cursor.Step()) {
                    if (auto[ok, val] = cursor.TryGetColumnString(0); ok) {
                        result.emplace_back(val);
                    }
                }
            });
        });

        return result;
    }

    vector<string> WebRpcRepository::GetLastContentHashesByRootTxHashes(const vector<string>& rootHashes)
    {
        vector<string> result;
        result.reserve(rootHashes.size());

        SqlTransaction(__func__, [&]()
        {
            Sql(R"sql(
                with
                cnt as (
                    select
                        c.Uid as uid
                    from
                        vTx t
                        cross join Chain c on
                            c.TxId = t.RowId
                    where
                        t.Hash in ( )sql" + join(vector<string>(rootHashes.size(), "?"), ",") + R"sql( )
                )
                select
                    hash.String
                from
                    cnt,
                    Chain c
                    cross join Last l on
                        l.TxId = c.TxId
                    cross join Registry hash on
                        hash.RowId = c.TxId
                where
                    c.Uid = cnt.uid
            )sql")
            .Bind(rootHashes)
            .Select([&](Cursor& cursor) {
                while (cursor.Step()) {
                    if (auto [ok, val] = cursor.TryGetColumnString(0); ok) {
                        result.emplace_back(val);
                    }
                }
            });
        });

        return result;
    }

    vector<string> WebRpcRepository::GetCommentScores(const vector<string>& commentHashes)
    {
        vector<string> result;
        result.reserve(commentHashes.size());

        SqlTransaction(__func__, [&]()
        {
            Sql(R"sql(
                with
                cnt as (
                    select
                        r.RowId as id
                    from
                        Registry r
                    where
                        r.String in ( )sql" + join(vector<string>(commentHashes.size(), "?"), ",") + R"sql( )
                )
                select
                    r.String
                from
                    cnt,
                    Transactions t
                    cross join Registry r on
                        r.RowId = t.RowId
                where
                    t.Type = 301 and
                    t.RegId2 = cnt.id
            )sql")
            .Bind(commentHashes)
            .Select([&](Cursor& cursor) {
                while (cursor.Step()) {
                    if (auto[ok, val] = cursor.TryGetColumnString(0); ok) {
                        result.emplace_back(val);
                    }
                }
            });
        });

        return result;
    }

    vector<string> WebRpcRepository::GetAddresses(const vector<string>& txHashes)
    {
        vector<string> result;

        SqlTransaction(__func__, [&]()
        {
            Sql(R"sql(
                select distinct
                    r.String
                from
                    vTx t
                    cross join Registry r on
                        r.RowId = t.RegId1
                where
                    t.Hash in ( )sql" + join(vector<string>(txHashes.size(), "?"), ",") + R"sql( )
            )sql")
            .Bind(txHashes)
            .Select([&](Cursor& cursor) {
                while (cursor.Step()) {
                    if (auto[ok, val] = cursor.TryGetColumnString(0); ok) {
                        result.emplace_back(val);
                    }
                }
            });

        });

        return result;
    }

    vector<string> WebRpcRepository::GetAccountsIds(const vector<string>& addresses)
    {

        vector<string> result;

        SqlTransaction(
            __func__,
            [&]() -> Stmt& {
                return Sql(R"sql(
                    with
                    addr as (
                        select
                            RowId as id
                        from
                            Registry
                        where
                            String in ( )sql" + join(vector<string>(addresses.size(), "?"), ",") + R"sql( )
                    )
                    select
                        (select r.String from Registry r where r.RowId = a.RowId)
                    from
                        addr
                    cross join
                        Transactions a
                            on a.Type in (100) and a.RegId1 = addr.id
                    cross join
                        Last l
                            on l.TxId = a.RowId
                )sql")
                .Bind(addresses);
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

    UniValue WebRpcRepository::GetCommentsByPost(const string& postHash, const string& parentHash, const string& addressHash)
    {
        auto result = UniValue(UniValue::VARR);

        string parentWhere = " and c.RegId4 is null ";
        if (!parentHash.empty())
            parentWhere = " and c.RegId4 in (select r.RowId from Registry r where r.String = ?) ";

        auto sql = R"sql(
            with
            tx as (
                select
                    r.RowId as id,
                    r.String as hash
                from
                    Registry r
                where
                    r.String = ?
            ),
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
                c.Type,
                (select r.String from Registry r where r.RowId = c.RowId),
                (select r.String from Registry r where r.RowId = c.RegId2) as RootTxHash,
                (select r.String from Registry r where r.RowId = c.RegId3) as PostTxHash,
                (select r.String from Registry r where r.RowId = c.RegId1) as AddressHash,
                r.Time AS RootTime,
                c.Time,
                cc.Height,
                pl.String1 AS Msg,
                (select r.String from Registry r where r.RowId = c.RegId4) as ParentTxHash,
                (select r.String from Registry r where r.RowId = c.RegId5) as AnswerTxHash,
                (
                    select count()
                    from Transactions sc indexed by Transactions_Type_RegId2_RegId1
                    cross join Chain csc on csc.TxId = sc.RowId
                    where sc.Type=301 and sc.RegId2 = c.RegId2 and sc.Int1 = 1
                ) as ScoreUp,
                (
                    select count()
                    from Transactions sc indexed by Transactions_Type_RegId2_RegId1
                    cross join Chain csc on csc.TxId = sc.RowId
                    where sc.Type=301 and sc.RegId2 = c.RegId2 and sc.Int1 = -1
                ) as ScoreDown,
                (
                    select r.Value
                    from Ratings r indexed by Ratings_Type_Uid_Last_Value
                    where r.Type = 3 and r.Uid = cc.Uid and r.Last = 1
                ) as Reputation,
                ifnull(sc.Int1, 0) as MyScore,
                (
                    select
                        count()
                    from
                        Transactions s indexed by Transactions_Type_RegId4_RegId1
                    cross join
                        Last ls
                            on ls.TxId = s.RowId
                    cross join
                        Chain cs
                            on cs.TxId = s.RowId
                    -- exclude deleted accounts TODO (aok, block): need?
                    cross join
                        Transactions uac indexed by Transactions_Type_RegId1_RegId2_RegId3
                            on uac.Type = 100 and uac.RegId1 = s.RegId1
                    cross join
                        Last luac
                            on luac.TxId = uac.RowId
                    where
                        s.Type in (204, 205, 206) and
                        s.RegId4 = c.RegId2
                ) as ChildrenCount,
                o.Value as Donate,
                (select 1 from BlockingLists bl where bl.IdSource = t.RegId1 and bl.IdTarget = c.RegId1 limit 1)ContentBlockedComment,
                (select 1 from BlockingLists bl where bl.IdSource = c.RegId1 and bl.IdTarget = t.RegId1 limit 1)CommentBlockedContent,
                (
                    select
                        json_group_object(ff.reason, ff.cnt)
                    from (
                        select
                            f.Int1 as reason,
                            count() as cnt
                        from
                            Transactions f indexed by Transactions_Type_RegId2_RegId1
                        where
                            f.Type in (410) and
                            f.RegId2 = c.RowId
                        group by f.Int1
                    ) ff
                ) as Flags
            from
                tx,
                addr
            cross join
                Transactions c indexed by Transactions_Type_RegId3_RegId1
                    on c.Type in (204, 205, 206) and c.RegId3 = tx.id
            cross join
                Last lc
                    on lc.TxId = c.RowId
            cross join
                Chain cc
                    on cc.TxId = c.RowId
            cross join
                Transactions ua indexed by Transactions_Type_RegId1_RegId2_RegId3
                    on ua.Type = 100 and ua.RegId1 = c.RegId1
            cross join
                Last lua
                    on lua.TxId = ua.RowId
            cross join
                Transactions r
                    on r.RowId = c.RegId2
            left join
                Payload pl
                    on pl.TxId = c.RowId
            cross join
                Transactions t indexed by Transactions_Type_RegId2_RegId1
                    on t.Type in (200, 201, 202, 209, 210) and t.RegId2 = c.RegId3
            cross join
                Last lt
                    on lt.TxId = t.RowId
            cross join
                Chain ct
                    on ct.TxId = t.RowId
            left join
                Transactions sc indexed by Transactions_Type_RegId2_RegId1
                    on sc.Type in (301) and sc.RegId2 = c.RegId2 and sc.RegId1 = addr.id and exists (select 1 from Chain csc where csc.TxId = sc.RowId)
            left join
                TxOutputs o indexed by TxOutputs_AddressId_TxIdDesc_Number
                    on o.TxId = r.RowId and o.AddressId = t.RegId1 and o.AddressId != c.RegId1
            where 1=1
                )sql" + parentWhere + R"sql(
        )sql";

        SqlTransaction(
            __func__,
            [&]() -> Stmt& {
                auto& stmt = Sql(sql);
                stmt.Bind(postHash, addressHash);
                if (!parentHash.empty())
                    stmt.Bind(parentHash);
                    
                return stmt;
            },
            [&] (Stmt& stmt) {
                stmt.Select([&](Cursor& cursor) {
                    while (cursor.Step())
                    {
                        UniValue record(UniValue::VOBJ);

                        auto[ok1, rootTxHash] = cursor.TryGetColumnString(2);
                        record.pushKV("id", rootTxHash);

                        if (auto[ok, value] = cursor.TryGetColumnString(3); ok)
                            record.pushKV("postid", value);

                        if (auto[ok, value] = cursor.TryGetColumnString(4); ok) record.pushKV("address", value);
                        if (auto[ok, value] = cursor.TryGetColumnInt64(5); ok) record.pushKV("time", to_string(value));
                        if (auto[ok, value] = cursor.TryGetColumnInt64(6); ok) record.pushKV("timeUpd", to_string(value));
                        if (auto[ok, value] = cursor.TryGetColumnInt64(7); ok) record.pushKV("block", to_string(value));
                        if (auto[ok, value] = cursor.TryGetColumnString(8); ok) record.pushKV("msg", value);
                        if (auto[ok, value] = cursor.TryGetColumnString(9); ok) record.pushKV("parentid", value);
                        if (auto[ok, value] = cursor.TryGetColumnString(10); ok) record.pushKV("answerid", value);
                        if (auto[ok, value] = cursor.TryGetColumnInt64(11); ok) record.pushKV("scoreUp", to_string(value));
                        if (auto[ok, value] = cursor.TryGetColumnInt64(12); ok) record.pushKV("scoreDown", to_string(value));
                        if (auto[ok, value] = cursor.TryGetColumnInt64(13); ok) record.pushKV("reputation", to_string(value));
                        if (auto[ok, value] = cursor.TryGetColumnInt64(14); ok && !addressHash.empty()) record.pushKV("myScore", to_string(value));
                        if (auto[ok, value] = cursor.TryGetColumnInt64(15); ok) record.pushKV("children", to_string(value));

                        if (auto[ok, value] = cursor.TryGetColumnInt64(16); ok)
                        {
                            record.pushKV("amount", value);
                            record.pushKV("donation", "true");
                        }

                        if (auto[ok, value] = cursor.TryGetColumnInt(17); ok && value > 0)
                            record.pushKV("blck_cnt_cmt", 1);
                        if (auto[ok, value] = cursor.TryGetColumnInt(18); ok && value > 0)
                            record.pushKV("blck_cmt_cnt", 1);

                        if (auto[ok, value] = cursor.TryGetColumnString(19); ok)
                        {
                            UniValue flags(UniValue::VOBJ);
                            flags.read(value);
                            record.pushKV("flags", flags);
                        };

                        if (auto[ok, value] = cursor.TryGetColumnInt(0); ok)
                        {
                            switch (static_cast<TxType>(value))
                            {
                                case PocketTx::CONTENT_COMMENT:
                                    record.pushKV("deleted", false);
                                    record.pushKV("edit", false);
                                    break;
                                case PocketTx::CONTENT_COMMENT_EDIT:
                                    record.pushKV("deleted", false);
                                    record.pushKV("edit", true);
                                    break;
                                case PocketTx::CONTENT_COMMENT_DELETE:
                                    record.pushKV("deleted", true);
                                    record.pushKV("edit", true);
                                    break;
                                default:
                                    break;
                            }
                        }

                        result.push_back(record);
                    }
                });
            }
        );

        return result;
    }

    map<string, UniValue> WebRpcRepository::GetCommentsByHashes(const vector<string>& cmntHashes, const string& addressHash)
    {
        map<string, UniValue> result{};

        auto _result = GetComments(cmntHashes, {}, addressHash);
        for (auto const& [hash, id, record] : _result)
            result.insert_or_assign(hash, record);

        return result;
    }

    map<int64_t, UniValue> WebRpcRepository::GetCommentsByIds(const vector<int64_t>& cmntIds, const string& addressHash)
    {
        map<int64_t, UniValue> result{};

        auto _result = GetComments({}, cmntIds, addressHash);
        for (auto const& [hash, id, record] : _result)
            result.insert_or_assign(id, record);

        return result;
    }

    vector<tuple<string, int64_t, UniValue>> WebRpcRepository::GetComments(const vector<string>& cmntHashes, const vector<int64_t>& cmntIds, const string& addressHash)
    {
        vector<tuple<string, int64_t, UniValue>> result{};

        if (cmntHashes.empty() && cmntIds.empty())
            return result;
        if (!cmntHashes.empty() && !cmntIds.empty())
            return result;

        string with;
        if (!cmntHashes.empty())
        {
            with = R"sql(
                txs as (
                    select
                        r.RowId as id,
                        r.String as hash
                    from
                        Registry r
                    where
                        r.String in ( )sql" + join(vector<string>(cmntHashes.size(), "?"), ",") + R"sql( )
                )
            )sql";
        }
        if (!cmntIds.empty())
        {
            with = R"sql(
                txs as (
                    select
                        r.RowId as id,
                        r.String as hash
                    from
                        Chain c
                    cross join
                        Transactions t
                            on t.RowId = c.TxId
                    cross join
                        Last l
                            on l.TxId = t.RowId
                    cross join
                        Registry r
                            on r.RowId = t.RowId
                    where
                        c.Uid in ( )sql" + join(vector<string>(cmntIds.size(), "?"), ",") + R"sql( )
                )
            )sql";
        }

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
                    ),
                    )sql" + with + R"sql(

                    select

                        c.Type,
                        (select r.String from Registry r where r.RowId = c.RowId),
                        (select r.String from Registry r where r.RowId = c.RegId2) as RootTxHash,
                        (select r.String from Registry r where r.RowId = c.RegId3) as PostTxHash,
                        (select r.String from Registry r where r.RowId = c.RegId1) as AddressHash,
                        r.Time AS RootTime,
                        c.Time,
                        cc.Height,
                        pl.String1 AS Msg,
                        (select r.String from Registry r where r.RowId = c.RegId4) as ParentTxHash,
                        (select r.String from Registry r where r.RowId = c.RegId5) as AnswerTxHash,
                        (
                            select count()
                            from Transactions sc indexed by Transactions_Type_RegId2_RegId1
                            cross join Chain csc on csc.TxId = sc.RowId
                            where sc.Type=301 and sc.RegId2 = c.RegId2 and sc.Int1 = 1
                        ) as ScoreUp,
                        (
                            select count()
                            from Transactions sc indexed by Transactions_Type_RegId2_RegId1
                            cross join Chain csc on csc.TxId = sc.RowId
                            where sc.Type=301 and sc.RegId2 = c.RegId2 and sc.Int1 = -1
                        ) as ScoreDown,
                        (
                            select r.Value
                            from Ratings r indexed by Ratings_Type_Uid_Last_Value
                            where r.Type = 3 and r.Uid = cc.Uid and r.Last = 1
                        ) as Reputation,
                        ifnull(sc.Int1, 0) as MyScore,
                        (
                            select count()
                            from
                                Transactions s indexed by Transactions_Type_RegId4_RegId1
                            cross join
                                Last ls
                                    on ls.TxId = s.RowId
                            cross join
                                Chain cs
                                    on cs.TxId = s.RowId
                            -- exclude deleted accounts
                            cross join
                                Transactions uac indexed by Transactions_Type_RegId1_RegId2_RegId3
                                    on uac.Type = 100 and uac.RegId1 = s.RegId1
                            cross join
                                Last luac
                                    on luac.TxId = uac.RowId
                            where
                                s.Type in (204, 205, 206) and
                                s.RegId4 = c.RegId2
                        ) AS ChildrenCount,
                        o.Value as Donate,
                        (select 1 from BlockingLists bl where bl.IdSource = t.RegId1 and bl.IdTarget = c.RegId1 limit 1)ContentBlockedComment,
                        (select 1 from BlockingLists bl where bl.IdSource = c.RegId1 and bl.IdTarget = t.RegId1 limit 1)CommentBlockedContent,
                        cc.Uid,
                        (
                            select
                                json_group_object(ff.reason, ff.cnt)
                            from (
                                select
                                    f.Int1 as reason,
                                    count() as cnt
                                from
                                    Transactions f indexed by Transactions_Type_RegId2_RegId1
                                where
                                    f.Type in (410) and
                                    f.RegId2 = c.RowId
                                group by f.Int1
                            ) ff
                        ) as Flags
                    from
                        txs,
                        addr
                    cross join
                        Transactions c indexed by Transactions_Type_RegId2_RegId1
                            on c.Type in (204, 205, 206) and c.RegId2 = txs.id
                    cross join
                        Last lc
                            on lc.TxId = c.RowId
                    cross join
                        Chain cc
                            on cc.TxId = c.RowId
                    cross join
                        Transactions r
                            on r.RowId = c.RegId2
                    left join
                        Payload pl
                            on pl.TxId = c.RowId
                    cross join
                        Transactions t indexed by Transactions_Type_RegId2_RegId1
                            on t.Type in (200, 201, 202, 209, 210) and t.RegId2 = c.RegId3
                    cross join
                        Last lt
                            on lt.TxId = t.RowId
                    cross join
                        Chain ct
                            on ct.TxId = t.RowId
                    left join
                        Transactions sc indexed by Transactions_Type_RegId1_RegId2_RegId3
                            on sc.Type in (301) and sc.RegId1 = addr.id and sc.RegId2 = c.RegId2 and exists (select 1 from Chain csc where csc.TxId = sc.RowId)
                    left join
                        TxOutputs o indexed by TxOutputs_AddressId_TxIdDesc_Number
                            on o.TxId = r.RowId and o.AddressId = t.RegId1 and o.AddressId != c.RegId1
                )sql")
                .Bind(addressHash, cmntHashes, cmntIds);
            },
            [&] (Stmt& stmt) {
                stmt.Select([&](Cursor& cursor) {
                    while (cursor.Step())
                    {
                        UniValue record(UniValue::VOBJ);

                        if (auto[ok, value] = cursor.TryGetColumnInt(0); ok) record.pushKV("type", value);
                        if (auto[ok, value] = cursor.TryGetColumnString(2); ok) record.pushKV("id", value);
                        if (auto[ok, value] = cursor.TryGetColumnString(3); ok) record.pushKV("postid", value);
                        if (auto[ok, value] = cursor.TryGetColumnString(4); ok) record.pushKV("address", value);
                        if (auto[ok, value] = cursor.TryGetColumnInt64(5); ok) record.pushKV("time", value);
                        if (auto[ok, value] = cursor.TryGetColumnInt64(6); ok) record.pushKV("timeUpd", value);
                        if (auto[ok, value] = cursor.TryGetColumnInt64(7); ok) record.pushKV("block", value);
                        if (auto[ok, value] = cursor.TryGetColumnString(8); ok) record.pushKV("msg", value);
                        if (auto[ok, value] = cursor.TryGetColumnString(9); ok) record.pushKV("parentid", value);
                        if (auto[ok, value] = cursor.TryGetColumnString(10); ok) record.pushKV("answerid", value);
                        if (auto[ok, value] = cursor.TryGetColumnInt(11); ok) record.pushKV("scoreUp", value);
                        if (auto[ok, value] = cursor.TryGetColumnInt(12); ok) record.pushKV("scoreDown", value);
                        if (auto[ok, value] = cursor.TryGetColumnInt(13); ok) record.pushKV("reputation", value);
                        if (auto[ok, value] = cursor.TryGetColumnInt(14); ok && !addressHash.empty()) record.pushKV("myScore", value);
                        if (auto[ok, value] = cursor.TryGetColumnInt(15); ok) record.pushKV("children", value);

                        if (auto[ok, value] = cursor.TryGetColumnInt64(16); ok)
                        {
                            record.pushKV("amount", value);
                            record.pushKV("donation", "true");
                        }

                        if (auto[ok, value] = cursor.TryGetColumnInt(17); ok && value > 0)
                            record.pushKV("blck_cnt_cmt", 1);
                        if (auto[ok, value] = cursor.TryGetColumnInt(18); ok && value > 0)
                            record.pushKV("blck_cmt_cnt", 1);

                        if (auto[ok, value] = cursor.TryGetColumnInt(0); ok)
                        {
                            switch (static_cast<TxType>(value))
                            {
                                case PocketTx::CONTENT_COMMENT:
                                    record.pushKV("deleted", false);
                                    record.pushKV("edit", false);
                                    break;
                                case PocketTx::CONTENT_COMMENT_EDIT:
                                    record.pushKV("deleted", false);
                                    record.pushKV("edit", true);
                                    break;
                                case PocketTx::CONTENT_COMMENT_DELETE:
                                    record.pushKV("deleted", true);
                                    record.pushKV("edit", true);
                                    break;
                                default:
                                    break;
                            }
                        }

                        if (auto[ok, value] = cursor.TryGetColumnString(20); ok)
                        {
                            UniValue flags(UniValue::VOBJ);
                            flags.read(value);
                            record.pushKV("flags", flags);
                        };

                        auto[ok_hash, hash] = cursor.TryGetColumnString(1);
                        auto[ok_id, id] = cursor.TryGetColumnInt64(19);
                        result.emplace_back(hash, id, record);
                    }
                });
            }
        );

        return result;
    }

    UniValue WebRpcRepository::GetPagesScores(const vector<string>& postHashes, const vector<string>& commentHashes, const string& addressHash)
    {
        UniValue result(UniValue::VARR);

        if (!postHashes.empty())
        {
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
                        ),
                        tx as (
                            select
                                r.RowId as id,
                                r.String as hash
                            from
                                Registry r
                            where
                                r.String in ( )sql" + join(vector<string>(postHashes.size(), "?"), ",") + R"sql( )
                        )
                        select
                            tx.hash as ContentTxHash,
                            ifnull(sc.Int1, 0) as MyScoreValue

                        from
                            addr,
                            tx
                        cross join
                            Transactions sc
                                on sc.Type in (300) and sc.RegId1 = addr.id and sc.RegId2 = tx.id
                        cross join
                            Chain c
                                on c.TxId = sc.RowId
                    )sql")
                    .Bind(addressHash, postHashes);
                },
                [&] (Stmt& stmt) {
                    stmt.Select([&](Cursor& cursor) {
                        while (cursor.Step())
                        {
                            UniValue record(UniValue::VOBJ);

                            if (auto[ok, value] = cursor.TryGetColumnString(0); ok) record.pushKV("posttxid", value);
                            if (auto[ok, value] = cursor.TryGetColumnInt(1); ok && !addressHash.empty()) record.pushKV("value", value);

                            result.push_back(record);
                        }
                    });
                }
            );
        }

        if (!commentHashes.empty())
        {
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
                        ),
                        tx as (
                            select
                                r.RowId as id,
                                r.String as hash
                            from
                                Registry r
                            where
                                r.String in ( )sql" + join(vector<string>(commentHashes.size(), "?"), ",") + R"sql( )
                        )
                        select
                            tx.hash as RootTxHash,
                            (
                                select count()
                                from Transactions sc indexed by Transactions_Type_RegId2_RegId1
                                cross join Chain csc on csc.TxId = sc.RowId
                                where sc.Type in (301) and sc.RegId2 = c.RowId and sc.Int1 = 1
                            ) as ScoreUp,
                            (
                                select count()
                                from Transactions sc indexed by Transactions_Type_RegId2_RegId1
                                cross join Chain csc on csc.TxId = sc.RowId
                                where sc.Type in (301) and sc.RegId2 = c.RowId and sc.Int1 = -1
                            ) as ScoreDown,
                            (
                                select r.Value
                                from Ratings r indexed by Ratings_Type_Uid_Last_Value
                                where r.Uid = cc.Uid and r.Type = 3 and r.Last = 1
                            ) as Reputation,
                            ifnull(msc.Int1, 0) AS MyScore
                        from
                            addr,
                            tx
                        cross join
                            Transactions c indexed by Transactions_Type_RegId2_RegId1
                                on c.Type in (204, 205) and c.RegId2 = tx.id
                        cross join
                            Last lc
                                on lc.TxId = c.RowId
                        cross join
                            Chain cc
                                on cc.TxId = c.RowId
                        left join Transactions msc indexed by Transactions_Type_RegId2_RegId1
                            on msc.Type in (301) and msc.RegId2 = c.RegId2 and msc.RegId1 = addr.id and exists (select 1 from Chain cmsc where cmsc.TxId = msc.RowId)
                    )sql")
                    .Bind(addressHash, commentHashes);
                },
                [&] (Stmt& stmt) {
                    stmt.Select([&](Cursor& cursor) {
                        while (cursor.Step())
                        {
                            UniValue record(UniValue::VOBJ);

                            if (auto[ok, value] = cursor.TryGetColumnString(0); ok) record.pushKV("cmntid", value);
                            if (auto[ok, value] = cursor.TryGetColumnInt(1); ok) record.pushKV("scoreUp", value);
                            if (auto[ok, value] = cursor.TryGetColumnInt(2); ok) record.pushKV("scoreDown", value);
                            if (auto[ok, value] = cursor.TryGetColumnInt(3); ok) record.pushKV("reputation", value);
                            if (auto[ok, value] = cursor.TryGetColumnInt(4); ok && !addressHash.empty()) record.pushKV("myScore", value);

                            result.push_back(record);
                        }
                    });
                }
            );
        }

        return result;
    }

    UniValue WebRpcRepository::GetPostScores(const string& postTxHash)
    {
        UniValue result(UniValue::VARR);

        SqlTransaction(
            __func__,
            [&]() -> Stmt& {
                return Sql(R"sql(
                    with
                    tx as (
                        select
                            r.RowId as id,
                            r.String as hash
                        from
                            Registry r
                        where
                            r.String = ?
                    )
                    select
                        tx.hash as ContentTxHash,
                        (select r.String from Registry r where r.RowId = s.RegId1) as ScoreAddressHash,
                        p.String2 as AccountName,
                        p.String3 as AccountAvatar,
                        r.Value as AccountReputation,
                        s.Int1 as ScoreValue
                    from
                        tx
                    cross join
                        Transactions s indexed by Transactions_Type_RegId2_RegId1
                            on s.Type in (300) and s.RegId2 = tx.id
                    cross join
                        Chain cs
                            on cs.TxId = s.RowId
                    cross join
                        Transactions u indexed by Transactions_Type_RegId1_RegId2_RegId3
                            on u.Type in (100, 170) and u.RegId1 = s.RegId1
                    cross join
                        Last lu
                            on lu.TxId = u.RowId
                    cross join
                        Chain cu
                            on cu.TxId = u.RowId
                    left join
                        Ratings r indexed by Ratings_Type_Uid_Last_Value
                            on r.Type = 0 and r.Last = 1 and r.Uid = cu.Uid
                    left join
                        Payload p
                            on p.TxId = u.RowId
                )sql")
                .Bind(postTxHash);
            },
            [&] (Stmt& stmt) {
                stmt.Select([&](Cursor& cursor) {
                    while (cursor.Step())
                    {
                        UniValue record(UniValue::VOBJ);

                        if (auto[ok, value] = cursor.TryGetColumnString(0); ok) record.pushKV("posttxid", value);
                        if (auto[ok, value] = cursor.TryGetColumnString(1); ok) record.pushKV("address", value);
                        if (auto[ok, value] = cursor.TryGetColumnString(2); ok) record.pushKV("name", value);
                        if (auto[ok, value] = cursor.TryGetColumnString(3); ok) record.pushKV("avatar", value);
                        if (auto[ok, value] = cursor.TryGetColumnInt(4); ok) record.pushKV("reputation", value);
                        if (auto[ok, value] = cursor.TryGetColumnInt(5); ok) record.pushKV("value", value);

                        result.push_back(record);
                    }
                });
            }
        );

        return result;
    }

    UniValue WebRpcRepository::GetAddressScores(const vector<string>& postHashes, const string& address)
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
                    ),
                    posts as (
                        select
                            r.RowId as id,
                            r.String as hash
                        from
                            Registry r
                        where
                            r.String in ( )sql" + join(vector<string>(postHashes.size(), "?"), ",") + R"sql( )
                    )
                    select
                        (select r.String from Registry r where r.RowId = s.RegId2) as posttxid,
                        addr.hash as address,
                        up.String2 as name,
                        up.String3 as avatar,
                        ur.Value as reputation,
                        s.Int1 as value
                    from
                        addr,
                        posts
                    cross join
                        Transactions s indexed by Transactions_Type_RegId1_RegId2_RegId3
                            on s.Type in (300) and s.RegId1 = addr.id and ( ? or s.RegId2 = posts.id )
                    cross join
                        Chain cs
                            on cs.TxId = s.RowId
                    cross join
                        Transactions u indexed by Transactions_Type_RegId1_RegId2_RegId3
                            on u.Type in (100) and u.RegId1 = s.RegId1
                    cross join
                        Last lu
                            on lu.TxId = u.RowId
                    cross join
                        Chain cu
                            on cu.TxId = u.RowId
                    cross join
                        Payload up
                            on up.TxId = u.RowId
                    left join
                        Ratings ur indexed by Ratings_Type_Uid_Last_Value
                            on ur.Type = 0 and ur.Uid = cu.Uid and ur.Last = 1
                )sql")
                .Bind(address, postHashes, postHashes.empty());
            },
            [&] (Stmt& stmt) {
                stmt.Select([&](Cursor& cursor) {
                    while (cursor.Step())
                    {
                        UniValue record(UniValue::VOBJ);

                        if (auto[ok, value] = cursor.TryGetColumnString(0); ok) record.pushKV("posttxid", value);
                        if (auto[ok, value] = cursor.TryGetColumnString(1); ok) record.pushKV("address", value);
                        if (auto[ok, value] = cursor.TryGetColumnString(2); ok) record.pushKV("name", value);
                        if (auto[ok, value] = cursor.TryGetColumnString(3); ok) record.pushKV("avatar", value);
                        if (auto[ok, value] = cursor.TryGetColumnInt(4); ok) record.pushKV("reputation", value);
                        if (auto[ok, value] = cursor.TryGetColumnInt(5); ok) record.pushKV("value", value);

                        result.push_back(record);
                    }
                });
            }
        );

        return result;
    }

    UniValue WebRpcRepository::GetAccountRaters(const string& address)
    {
        UniValue result(UniValue::VARR);

        SqlTransaction(
            __func__,
            [&]() -> Stmt& {
                return Sql(R"sql(
                    with
                        addr as (
                            select
                                RowId as id,
                                String as hash
                            from
                                Registry
                            where
                                String = ?
                        ),
                        raters as (
                            select
                                r.RegId1,
                                count() as scores
                            from
                                addr
                            cross join
                                Transactions c indexed by Transactions_Type_RegId1_RegId2_RegId3 on
                                    c.Type in (200, 201, 202, 204, 209, 210) and c.RegId1 = addr.id
                            cross join
                                First fc on
                                    fc.TxId = c.RowId
                            cross join
                                Transactions r indexed by Transactions_Type_RegId2_RegId1 on
                                    r.Type in (300, 301) and
                                    r.RegId2 = c.RowId and
                                    ( (r.Type = 300 and r.Int1 = 5) or (r.Type = 301 and r.Int1 = 1) )
                            cross join
                                Chain cr on
                                    cr.TxId = r.RowId
                            group by
                                r.RegId1
                        )
                    select
                        cru.Uid,
                        (select String from Registry r where r.RowId = r.RegId1),
                        pru.String2,
                        pru.String3,
                        pru.String4,
                        ruf.Time,
                        ifnull(rru.Value, 0),
                        r.scores
                    from
                        raters r
                    cross join
                        Transactions ru indexed by Transactions_Type_RegId1_RegId2_RegId3 on
                            ru.Type in (100) and
                            ru.RegId1 = r.RegId1
                    cross join
                        Last lru on
                            lru.TxId = ru.RowId
                    cross join
                        First fru on
                            fru.TxId = ru.RowId
                    cross join
                        Transactions ruf on
                            ruf.RowId = fru.TxId
                    cross join
                        Chain cru on
                            cru.TxId = ru.RowId
                    cross join
                        Payload pru on
                            pru.TxId = ru.RowId
                    left join
                        Ratings rru indexed by Ratings_Type_Uid_Last_Value on
                            rru.Type = 0 and rru.Uid = cru.Uid and rru.Last = 1
                )sql")
                .Bind(address);
            },
            [&] (Stmt& stmt) {
                stmt.Select([&](Cursor& cursor) {
                    while (cursor.Step())
                    {
                        UniValue record(UniValue::VOBJ);
                        
                        cursor.Collect<int>(0, record, "id");
                        cursor.Collect<string>(1, record, "address");
                        cursor.Collect<string>(2, record, "name");
                        cursor.Collect<string>(3, record, "avatar");
                        cursor.Collect<string>(4, record, "about");
                        cursor.Collect<int64_t>(5, record, "regdate");
                        cursor.Collect<int64_t>(6, record, "reputation");
                        cursor.Collect<int64_t>(7, record, "ratingscnt");

                        result.push_back(record);
                    }
                });
            }
        );

        return result;
    }

    UniValue WebRpcRepository::GetSubscribesAddresses(const string& address, const vector<TxType>& types, const string& orderBy, bool orderDesc, int offset, int limit)
    {
        UniValue result(UniValue::VARR);

        string sql = R"sql(
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
                (select r.String from Registry r where r.RowId = s.RegId2),
                case
                    when s.Type = 303 then 1
                    else 0
                end,
                ifnull(r.Value,0),
                cs.Height
            from
                addr
            cross join
                Transactions s indexed by Transactions_Type_RegId1_RegId2_RegId3
                    on s.Type in ( )sql" + join(types | transformed(static_cast<string(*)(int)>(to_string)), ",") + R"sql( ) and
                    s.RegId1 = addr.id
            cross join
                Last ls
                    on ls.TxId = s.RowId
            cross join
                Chain cs
                    on cs.TxId = s.RowId
            cross join
                Transactions u indexed by Transactions_Type_RegId1_RegId2_RegId3
                    on u.Type in (100, 170) and u.RegId1 = s.RegId2
            cross join
                Last lu
                    on lu.TxId = u.RowId
            cross join
                Chain cu
                    on cu.TxId = u.RowId
            left join
                Ratings r indexed by Ratings_Type_Uid_Last_Value
                    on r.Type = 0 and r.Uid = cu.Uid and r.Last = 1
        )sql";

        if (orderBy == "reputation")
            sql += " order by r.Value "s + (orderDesc ? " desc "s : ""s);
        if (orderBy == "height")
            sql += " order by cs.Height "s + (orderDesc ? " desc "s : ""s);
        
        if (limit > 0)
        {
            sql += " limit ? "s;
            sql += " offset ? "s;
        }
        
        SqlTransaction(
            __func__,
            [&]() -> Stmt& {
                auto& stmt = Sql(sql);
                stmt.Bind(address);
                if (limit > 0)
                    stmt.Bind(limit, offset);
                return stmt;
            },
            [&] (Stmt& stmt) {
                stmt.Select([&](Cursor& cursor) {
                    while (cursor.Step())
                    {
                        UniValue record(UniValue::VOBJ);

                        if (auto[ok, value] = cursor.TryGetColumnString(0); ok)
                            record.pushKV("adddress", value);

                        if (auto[ok, value] = cursor.TryGetColumnInt(1); ok)
                            record.pushKV("private", value);

                        if (auto[ok, value] = cursor.TryGetColumnInt(2); ok)
                            record.pushKV("reputation", value);

                        if (auto[ok, value] = cursor.TryGetColumnInt(3); ok)
                            record.pushKV("height", value);
                
                        result.push_back(record);
                    }
                });
            }
        );

        return result;
    }

    UniValue WebRpcRepository::GetSubscribersAddresses(const string& address, const vector<TxType>& types, const string& orderBy, bool orderDesc, int offset, int limit)
    {
        UniValue result(UniValue::VARR);

        string sql = R"sql(
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
                (select r.String from Registry r where r.RowId = s.RegId1),
                case
                    when s.Type = 303 then 1
                    else 0
                end,
                ifnull(r.Value,0),
                cs.Height
            from
                addr
            cross join
                Transactions s indexed by Transactions_Type_RegId2_RegId1
                    on s.Type in ( )sql" + join(types | transformed(static_cast<string(*)(int)>(to_string)), ",") + R"sql( ) and s.RegId2 = addr.id
            cross join
                Last ls
                    on ls.TxId = s.RowId
            cross join
                Chain cs
                    on cs.TxId = s.RowId
            cross join
                Transactions u indexed by Transactions_Type_RegId1_RegId2_RegId3
                    on u.Type in (100, 170) and u.RegId1 = s.RegId1
            cross join
                Last lu
                    on lu.TxId = u.RowId
            cross join
                Chain cu
                    on cu.TxId = u.RowId
            left join
                Ratings r indexed by Ratings_Type_Uid_Last_Value
                    on r.Type = 0 and r.Uid = cu.Uid and r.Last = 1
        )sql";

        if (orderBy == "reputation")
            sql += " order by r.Value "s + (orderDesc ? " desc "s : ""s);
        if (orderBy == "height")
            sql += " order by cs.Height "s + (orderDesc ? " desc "s : ""s);
        
        if (limit > 0)
        {
            sql += " limit ? "s;
            sql += " offset ? "s;
        }
        
        SqlTransaction(
            __func__,
            [&]() -> Stmt& {
                auto& stmt = Sql(sql);
                stmt.Bind(address);
                if (limit > 0)
                    stmt.Bind(limit, offset);
                return stmt;
            },
            [&] (Stmt& stmt) {
                stmt.Select([&](Cursor& cursor) {
                    while (cursor.Step())
                    {
                        UniValue record(UniValue::VOBJ);

                        if (auto[ok, value] = cursor.TryGetColumnString(0); ok)
                            record.pushKV("address", value);

                        if (auto[ok, value] = cursor.TryGetColumnInt(1); ok)
                            record.pushKV("private", value);

                        if (auto[ok, value] = cursor.TryGetColumnInt(2); ok)
                            record.pushKV("reputation", value);

                        if (auto[ok, value] = cursor.TryGetColumnInt(3); ok)
                            record.pushKV("height", value);

                        result.push_back(record);
                    }
                });
            }
        );

        return result;
    }

    UniValue WebRpcRepository::GetBlockings(const string& address, bool useAddresses)
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
                        c.Uid,
                        r.String
                    from
                        addr
                    cross join
                        BlockingLists bl on
                            bl.IdSource = addr.id
                    cross join
                        Transactions t indexed by Transactions_Type_RegId1_RegId2_RegId3 on
                            t.Type in (100) and
                            t.RegId1 = bl.IdTarget
                    cross join
                        First f on
                            f.TxId = t.RowId
                    cross join
                        Chain c on
                            c.TxId = t.RowId
                    cross join
                        Registry r on
                            r.RowId = bl.IdTarget
                )sql")
                .Bind(address);
            },
            [&] (Stmt& stmt) {
                stmt.Select([&](Cursor& cursor) {
                    while (cursor.Step()) {
                        if (useAddresses) {
                            if (auto[ok, value] = cursor.TryGetColumnString(1); ok) {
                                result.push_back(value);
                            }
                        }
                        else {
                            if (auto[ok, value] = cursor.TryGetColumnInt64(0); ok) {
                                result.push_back(value);
                            }
                        }
                    }
                });
            }
        );

        return result;
    }
    
    UniValue WebRpcRepository::GetBlockers(const string& address, bool useAddresses)
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
                        c.Uid,
                        r.String
                    from
                        addr
                    cross join
                        BlockingLists bl on
                            bl.IdTarget = addr.id
                    cross join
                        Transactions t indexed by Transactions_Type_RegId1_RegId2_RegId3 on
                            t.Type in (100) and
                            t.RegId1 = bl.IdSource
                    cross join
                        First f on
                            f.TxId = t.RowId
                    cross join
                        Chain c on
                            c.TxId = t.RowId
                    cross join
                        Registry r on
                            r.RowId = bl.IdSource
                )sql")
                .Bind(address);
            },
            [&] (Stmt& stmt) {
                stmt.Select([&](Cursor& cursor) {
                    while (cursor.Step()) {
                        if (useAddresses) {
                            if (auto[ok, value] = cursor.TryGetColumnString(1); ok) {
                                result.push_back(value);
                            }
                        }
                        else {
                            if (auto[ok, value] = cursor.TryGetColumnInt64(0); ok) {
                                result.push_back(value);
                            }
                        }
                    }
                });
            }
        );

        return result;
    }

    // TODO (aok, api): implement
    vector<string> WebRpcRepository::GetTopAccounts(int topHeight, int countOut, const string& lang,
        const vector<string>& tags, const vector<int>& contentTypes,
        const vector<string>& addrsExcluded, const vector<string>& tagsExcluded, int depth,
        int badReputationLimit)
    {
        vector<string> result;

        if (contentTypes.empty())
            return result;

        // --------------------------------------------

        string contentTypesWhere = " ( " + join(vector<string>(contentTypes.size(), "?"), ",") + " ) ";

        string langFilter;
        if (!lang.empty())
            langFilter += " cross join Payload p indexed by Payload_String1_TxHash on p.TxHash = u.Hash and p.String1 = ? ";

        string sql = R"sql(
            select t.String1

            from Transactions t indexed by Transactions_Last_Id_Height

            cross join Ratings cr indexed by Ratings_Type_Id_Last_Value
                on cr.Type = 2 and cr.Last = 1 and cr.Id = t.Id and cr.Value > 0

            cross join Transactions u indexed by Transactions_Type_Last_String1_Height_Id
                on u.Type in (100) and u.Last = 1 and u.Height > 0 and u.String1 = t.String1

            )sql" + langFilter + R"sql(

            left join Ratings ur indexed by Ratings_Type_Id_Last_Height
                on ur.Type = 0 and ur.Last = 1 and ur.Id = u.Id

            where t.Type in )sql" + contentTypesWhere + R"sql(
                and t.Last = 1
                and t.String3 is null
                and t.Height > ?
                and t.Height <= ?

                -- Do not show posts from users with low reputation
                and ifnull(ur.Value,0) > ?
        )sql";

        if (!tags.empty())
        {
            sql += R"sql(
                and t.id in (
                    select tm.ContentId
                    from web.Tags tag
                    join web.TagsMap tm indexed by TagsMap_TagId_ContentId
                        on tag.Id = tm.TagId
                    where tag.Value in ( )sql" + join(vector<string>(tags.size(), "?"), ",") + R"sql( )
                        )sql" + (!lang.empty() ? " and tag.Lang = ? " : "") + R"sql(
                )
            )sql";
        }

        if (!addrsExcluded.empty()) sql += " and t.String1 not in ( " + join(vector<string>(addrsExcluded.size(), "?"), ",") + " ) ";
        if (!tagsExcluded.empty())
        {
            sql += R"sql( and t.Id not in (
                select tmEx.ContentId
                from web.Tags tagEx
                join web.TagsMap tmEx indexed by TagsMap_TagId_ContentId
                    on tagEx.Id=tmEx.TagId
                where tagEx.Value in ( )sql" + join(vector<string>(tagsExcluded.size(), "?"), ",") + R"sql( )
                    )sql" + (!lang.empty() ? " and tagEx.Lang = ? " : "") + R"sql(
             ) )sql";
        }

        sql += " group by t.String1 ";
        sql += " order by count(*) desc ";
        sql += " limit ? ";

        // ---------------------------------------------

        SqlTransaction(
            __func__,
            [&]() -> Stmt& {
                auto& stmt = Sql(sql);

                if (!lang.empty()) stmt.Bind(lang);

                stmt.Bind(contentTypes, topHeight - depth, topHeight, badReputationLimit);

                if (!tags.empty())
                {
                    stmt.Bind(tags);

                    if (!lang.empty())
                        stmt.Bind(lang);
                }

                stmt.Bind(addrsExcluded);

                if (!tagsExcluded.empty())
                {
                    stmt.Bind(tagsExcluded);

                    if (!lang.empty())
                        stmt.Bind(lang);
                }

                stmt.Bind(countOut);

                return stmt;
            },
            [&] (Stmt& stmt) {
                stmt.Select([&](Cursor& cursor) {
                    // ---------------------------------------------
                    while (cursor.Step())
                    {
                        if (auto[ok, value] = cursor.TryGetColumnString(0); ok) result.push_back(value);
                    }
                });
            }
        );

        return result;
    }

    UniValue WebRpcRepository::GetTags(const string& lang, int pageSize, int pageStart)
    {
        UniValue result(UniValue::VARR);

        SqlTransaction(
            __func__,
            [&]() -> Stmt&  {
                return Sql(R"sql(
                    select
                        t.Value,
                        t.Count
                    from
                        web.Tags t
                    where
                        t.Lang = ?
                    order by
                        t.Count desc
                    limit ?
                    offset ?
                )sql")
                .Bind(lang, pageSize, pageStart);
            },
            [&] (Stmt& stmt) {
                stmt.Select([&](Cursor& cursor) {
                    while (cursor.Step())
                    {
                        UniValue record(UniValue::VOBJ);
                        
                        cursor.Collect<string>(0, record, "tag");
                        cursor.Collect<int>(1, record, "count");

                        result.push_back(record);
                    }
                });
            }
        );

        return result;
    }

    vector<int64_t> WebRpcRepository::GetContentIds(const vector<string>& txHashes)
    {
        vector<int64_t> result;

        if (txHashes.empty())
            return result;

        SqlTransaction(
            __func__,
            [&]() -> Stmt&  {
                return Sql(R"sql(
                    with
                    tx as (
                        select
                            r.RowId as id,
                            r.String as hash
                        from
                            Registry r
                        where
                            r.String in ( )sql" + join(vector<string>(txHashes.size(), "?"), ",") + R"sql( )
                    )

                    select
                        c.Uid
                    from
                        tx
                    cross join
                        Transactions t
                            on t.RowId = tx.id
                    cross join
                        Chain c
                            on c.TxId = t.RowId
                )sql")
                .Bind(txHashes);
            },
            [&] (Stmt& stmt) {
                stmt.Select([&](Cursor& cursor) {
                    while (cursor.Step())
                    {
                        if (auto[ok, value] = cursor.TryGetColumnInt64(0); ok)
                            result.push_back(value);
                    }
                });
            }
        );

        return result;
    }

    map<string,string> WebRpcRepository::GetContentsAddresses(const vector<string>& txHashes)
    {
        map<string, string> result;

        if (txHashes.empty())
            return result;

        SqlTransaction(
            __func__,
            [&]() -> Stmt& {
                return Sql(R"sql(
                    with
                    tx as (
                        select
                            r.RowId as id,
                            r.String as hash
                        from
                            Registry r
                        where
                            r.String in ( )sql" + join(vector<string>(txHashes.size(), "?"), ",") + R"sql( )
                    )

                    select
                        tx.hash,
                        (select r.String from Registry r where r.RowId = t.RegId1)
                    from
                        tx
                    cross join
                        Transactions t
                            on t.RowId = tx.id
                    cross join
                        Chain c
                            on c.TxId = t.RowId
                )sql")
                .Bind(txHashes);
            },
            [&] (Stmt& stmt) {
                stmt.Select([&](Cursor& cursor) {
                    while (cursor.Step())
                    {
                        auto[ok0, contenthash] = cursor.TryGetColumnString(0);
                        auto[ok1, contentaddress] = cursor.TryGetColumnString(1);
                        if(ok0 && ok1)
                            result.emplace(contenthash,contentaddress);
                    }
                });
            }
        );

        return result;
    }

    UniValue WebRpcRepository::GetUnspents(const vector<string>& addresses, int height, int confirmations, vector<pair<string, uint32_t>>& mempoolInputs)
    {
        UniValue result(UniValue::VARR);

        SqlTransaction(
            __func__,
            [&]() -> Stmt& {
                return Sql(R"sql(
                    with
                        addr as (
                            select
                                r.RowId as id
                            from
                                Registry r
                            where
                                r.String in ( )sql" + join(vector<string>(addresses.size(), "?"), ",") + R"sql( )
                        )
                    select
                        (select String from Registry where RowId=t.RowId),
                        o.Number,
                        (select String from Registry where RowId=o.AddressId),
                        o.Value,
                        (select String from Registry where RowId=o.ScriptPubKeyId),
                        t.Type,
                        c.Height
                    from addr
                    cross join TxOutputs o indexed by TxOutputs_AddressId_TxIdDesc_Number on
                        o.AddressId = addr.id and not exists (select 1 from TxInputs i where i.TxId = o.TxId and i.Number = o.Number)
                    cross join Chain c indexed by Chain_TxId_Height on
                        c.TxId = o.TxId and c.Height <= ?
                    cross join Transactions t on
                        t.RowId = o.TxId
                    order by c.Height asc
                )sql")
                .Bind(addresses, height - confirmations);
            },
            [&] (Stmt& stmt) {
                stmt.Select([&](Cursor& cursor) {
                    while (cursor.Step())
                    {
                        UniValue record(UniValue::VOBJ);

                        auto[ok0, txHash] = cursor.TryGetColumnString(0);
                        auto[ok1, txOut] = cursor.TryGetColumnInt(1);

                        string _txHash = txHash;
                        int _txOut = txOut;
                        // Exclude outputs already used as inputs in mempool
                        if (!ok0 || !ok1 || find_if(
                            mempoolInputs.begin(),
                            mempoolInputs.end(),
                            [&](const pair<string, int>& itm)
                            {
                                return itm.first == _txHash && itm.second == _txOut;
                            })  != mempoolInputs.end())
                            continue;

                        record.pushKV("txid", txHash);
                        record.pushKV("vout", txOut);

                        if (auto[ok, value] = cursor.TryGetColumnString(2); ok) record.pushKV("address", value);
                        if (auto[ok, value] = cursor.TryGetColumnInt64(3); ok)
                        {
                            record.pushKV("amount", ValueFromAmount(value));
                            record.pushKV("amountSat", value);
                        }
                        if (auto[ok, value] = cursor.TryGetColumnString(4); ok) record.pushKV("scriptPubKey", value);
                        if (auto[ok, value] = cursor.TryGetColumnInt(5); ok)
                        {
                            record.pushKV("coinbase", value == 2 || value == 3);
                            record.pushKV("pockettx", value > 3);
                        }
                        if (auto[ok, value] = cursor.TryGetColumnInt(6); ok)
                        {
                            record.pushKV("confirmations", height - value);
                            record.pushKV("height", value);
                        }

                        result.push_back(record);
                    }
                });
            }
        );

        return result;
    }

    UniValue WebRpcRepository::GetAccountEarning(const string& address, int height, int depth)
    {
        UniValue result(UniValue::VARR);

        SqlTransaction(
            __func__,
            [&]() -> Stmt& {
                return Sql(R"sql(
                    with
                        addr as (
                            select
                                RowId as id,
                                String as hash
                            from
                                Registry
                            where
                                String = ?
                        ),
                        txs as (
                            select
                                ifnull(t.Type, 0) as type,
                                ifnull(o.Value, 0) as amount
                            from
                                addr
                            cross join
                                TxOutputs o indexed by TxOutputs_AddressId_TxIdDesc_Number on
                                    o.AddressId = addr.id
                            cross join
                                Chain c on
                                    c.TxId = o.TxId and
                                    c.Height <= ? and
                                    c.Height >= ?
                            cross join
                                Transactions t on
                                    t.RowId = c.TxId
                            cross join
                                TxInputs i indexed by TxInputs_SpentTxId_Number_TxId on
                                    i.SpentTxId = o.TxId
                            cross join
                                TxOutputs oi indexed by TxOutputs_TxId_Number_AddressId on
                                    oi.TxId = i.TxId and
                                    oi.Number = i.Number and
                                    oi.AddressId != o.AddressId
                            group by o.TxId, t.Type
                        )
                    select
                        txs.type,
                        sum(txs.amount) as amount
                    from txs
                    group by
                        txs.type
                )sql")
                .Bind(address, height, height - depth);
            },
            [&] (Stmt& stmt) {
                stmt.Select([&](Cursor& cursor) {
                    int64_t amountLottery = 0;
                    int64_t amountDonation = 0;
                    int64_t amountTransfer = 0;

                    while (cursor.Step())
                    {
                        int64_t amount = 0;
                        if (auto[ok, type] = cursor.TryGetColumnInt(0); ok)
                        {
                            if (auto[ok, value] = cursor.TryGetColumnInt64(1); ok) amount = value;

                            switch (type) {
                                case 1:
                                    amountTransfer += amount;
                                    break;
                                case 3:
                                    amountLottery += amount;
                                    break;
                                case 204:
                                    amountDonation += amount;
                                    break;
                                default:
                                    break;
                            }
                        }
                    }

                    UniValue record(UniValue::VOBJ);
                    record.pushKV("address", address);
                    record.pushKV("amountLottery", amountLottery);
                    record.pushKV("amountDonation", amountDonation);
                    record.pushKV("amountTransfer", amountTransfer);
                    result.push_back(record);
                });
            }
        );

        return result;
    }

    tuple<int, UniValue> WebRpcRepository::GetContentLanguages(int height)
    {
        int resultCount = 0;
        UniValue resultData(UniValue::VOBJ);

        SqlTransaction(
            __func__,
            [&]() -> Stmt& {
                return Sql(R"sql(
                    select
                        c.Type,
                        p.String1 as lang,
                        count() as cnt
                    from
                        Chain cc indexed by Chain_Height_Uid
                    cross join Transactions c
                            on c.RowId = cc.TxId and c.Type in (200, 201, 202, 209, 210)
                    cross join
                        Last lc
                            on lc.TxId = c.RowId
                    cross join
                        Payload p on
                            p.TxId = c.RowId
                    where
                        cc.Height > ? and
                        cc.Uid is not null
                    group by
                        c.Type, p.String1
                )sql")
                .Bind(height);
            },
            [&] (Stmt& stmt) {
                stmt.Select([&](Cursor& cursor) {
                    while (cursor.Step())
                    {
                        auto[okType, typeInt] = cursor.TryGetColumnInt(0);
                        auto[okLang, lang] = cursor.TryGetColumnString(1);
                        auto[okCount, count] = cursor.TryGetColumnInt(2);
                        if (!okType || !okLang || !okCount)
                            continue;

                        auto type = TransactionHelper::TxStringType((TxType) typeInt);

                        if (resultData.At(type).isNull())
                            resultData.pushKV(type, UniValue(UniValue::VOBJ));

                        resultData.At(type).pushKV(lang, count);
                        resultCount += count;
                    }
                });
            }
        );

        return {resultCount, resultData};
    }

    tuple<int, UniValue> WebRpcRepository::GetLastAddressContent(const string& address, int height, int count)
    {
        int resultCount = 0;
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
                        count()
                    from
                        addr
                    cross join
                        Transactions t indexed by Transactions_Type_RegId1_RegId2_RegId3
                            on t.Type in (200, 201, 202, 209, 210) and t.RegId1 = addr.id
                    cross join
                        Last l
                            on l.TxId = t.RowId
                    cross join
                        Chain c indexed by Chain_TxId_Height
                            on c.TxId = l.TxId and c.Height > ?
                )sql")
                .Bind(address, height);
            },
            [&] (Stmt& stmt) {
                stmt.Select([&](Cursor& cursor) {
                    if (cursor.Step())
                        if (auto[ok, value] = cursor.TryGetColumnInt(0); ok)
                            resultCount = value;
                });
            }
        );

        // Try get last N records
        UniValue resultData(UniValue::VARR);
        if (resultCount > 0)
        {
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
                            (select r.String from Registry r where r.RowId = t.RegId2) as txHash,
                            t.Time,
                            ct.Height,
                            addr.hash as addrFrom,
                            p.String2 as nameFrom,
                            p.String3 as avatarFrom
                        from
                            addr
                        cross join
                            Transactions t
                                on t.Type in (200, 201, 202, 209, 210) and t.RegId1 = addr.id
                        cross join
                            Last lt
                                on lt.TxId = t.RowId
                        cross join
                            Chain ct
                                on ct.TxId = t.RowId and ct.Height > ?
                        cross join
                            Transactions u indexed by Transactions_Type_RegId1_RegId2_RegId3
                            on u.Type in (100) and u.RegId1 = t.RegId1
                        cross join
                            Last lu
                                on lu.TxId = u.RowId
                        cross join
                            Payload p
                                on p.TxId = u.RowId
                        order by
                            ct.Height desc
                        limit ?
                    )sql")
                    .Bind(address, height, count);
                },
                [&] (Stmt& stmt) {
                    stmt.Select([&](Cursor& cursor) {
                        while (cursor.Step())
                        {
                            auto[okHash, hash] = cursor.TryGetColumnString(0);
                            auto[okTime, time] = cursor.TryGetColumnInt64(1);
                            auto[okHeight, block] = cursor.TryGetColumnInt(2);
                            if (!okHash || !okTime || !okHeight)
                                continue;

                            UniValue record(UniValue::VOBJ);
                            record.pushKV("txid", hash);
                            record.pushKV("time", time);
                            record.pushKV("nblock", block);
                            if (auto[ok, value] = cursor.TryGetColumnString(3); ok) record.pushKV("addrFrom", value);
                            if (auto[ok, value] = cursor.TryGetColumnString(4); ok) record.pushKV("nameFrom", value);
                            if (auto[ok, value] = cursor.TryGetColumnString(5); ok) record.pushKV("avatarFrom", value);
                            resultData.push_back(record);
                        }
                    });
                }
            );
        }

        return {resultCount, resultData};
    }
    
    UniValue WebRpcRepository::GetContentsForAddress(const string& address)
    {
        UniValue result(UniValue::VARR);

        if (address.empty())
            return result;

        SqlTransaction(
            __func__,
            [&]() -> Stmt& {
                return Sql(R"sql(
                    with
                    addr as (
                        select
                            r.RowId as id
                        from
                            Registry r
                        where
                            r.String = ?
                    )
                    select
                        ct.Uid,
                        (select r.String from Registry r where r.RowId = t.RegId2) as RootTxHash,
                        t.Time,
                        p.String2 as Caption,
                        p.String3 as Message,
                        p.String6 as Settings,
                        ifnull(r.Value,0) as Reputation,
                        (
                            select
                                count()
                            from
                                Transactions scr indexed by Transactions_Type_RegId2_RegId1
                            cross join
                                Chain cs
                                    on cs.TxId = scr.RowId
                            where
                                scr.Type in (300) and
                                scr.RegId2 = t.RegId2
                        ) as ScoresCount,

                        ifnull((
                            select
                                sum(scr.Int1)
                            from
                                Transactions scr indexed by Transactions_Type_RegId2_RegId1
                            cross join
                                Chain cs
                                    on cs.TxId = scr.RowId
                            where
                                scr.Type in (300) and
                                scr.RegId2 = t.RegId2
                        ), 0) as ScoresSum

                    from
                        addr
                    cross join
                        Transactions t indexed by Transactions_Type_RegId1_RegId2_RegId3
                            on t.Type in (200, 201, 202, 209, 210) and t.RegId1 = addr.id
                    cross join
                        Last lt
                            on lt.TxId = t.RowId
                    cross join
                        Chain ct indexed by Chain_TxId_Height
                            on ct.TxId = t.RowId
                    cross join
                        Payload p
                            on p.TxId = t.RowId
                    left join
                        Ratings r indexed by Ratings_Type_Uid_Last_Value
                            on r.Type = 2 and r.Last = 1 and r.Uid = ct.Uid
                    order by ct.Height desc
                    limit 50
                )sql")
                .Bind(address);
            },
            [&] (Stmt& stmt) {
                stmt.Select([&](Cursor& cursor) {
                    while (cursor.Step())
                    {
                        UniValue record(UniValue::VOBJ);

                        auto[ok0, id] = cursor.TryGetColumnInt64(0);
                        auto[ok1, hash] = cursor.TryGetColumnString(1);
                        auto[ok2, time] = cursor.TryGetColumnString(2);
                        auto[ok3, caption] = cursor.TryGetColumnString(3);
                        auto[ok4, message] = cursor.TryGetColumnString(4);
                        auto[ok5, settings] = cursor.TryGetColumnString(5);
                        auto[ok6, reputation] = cursor.TryGetColumnString(6);
                        auto[ok7, scoreCnt] = cursor.TryGetColumnString(7);
                        auto[ok8, scoreSum] = cursor.TryGetColumnString(8);
                        
                        if (ok3) record.pushKV("content", HtmlUtils::UrlDecode(caption));
                        else record.pushKV("content", HtmlUtils::UrlDecode(message).substr(0, 100));

                        record.pushKV("txid", hash);
                        record.pushKV("time", time);
                        record.pushKV("reputation", reputation);
                        record.pushKV("settings", settings);
                        record.pushKV("scoreSum", scoreSum);
                        record.pushKV("scoreCnt", scoreCnt);

                        result.push_back(record);
                    }
                });
            }
        );

        return result;
    }

    // ------------------------- Missed ---------------------

    vector<UniValue> WebRpcRepository::GetMissedRelayedContent(const string& address, int height)
    {
        vector<UniValue> result;

        SqlTransaction(
            __func__,
            [&]() -> Stmt& {
                return Sql(R"sql(
                    with
                    addr as (
                        select
                            RowId as id
                        from
                            Registry
                        where
                            String = ?
                    ),
                    height as ( select ? as value )
                    select
                        (select rr.String from Registry rr where rr.RowId = r.RegId2) as RootTxHash,
                        (select rr.String from Registry rr where rr.RowId = r.RegId3) as RelayTxHash,
                        (select rr.String from Registry rr where rr.RowId = r.RegId1) as AddressHash,
                        r.Time,
                        c.Height
                    from
                        addr,
                        height
                    cross join
                        Chain c
                            on c.Height > height.value
                    cross join
                        Transactions r
                            on r.RowId = c.TxId and r.Type in (200, 201, 202, 209, 210) and r.RegId3 is not null
                    cross join
                        Last l
                            on l.TxId = r.RowId
                    cross join
                        Transactions p
                            on p.RowId = r.RegId3 and p.Type in (200, 201) and p.RegId1 = addr.id
                )sql")
                .Bind(address, height);
            },
            [&] (Stmt& stmt) {
                stmt.Select([&](Cursor& cursor) {
                    while (cursor.Step())
                    {
                        UniValue record(UniValue::VOBJ);

                        record.pushKV("msg", "reshare");
                        if (auto[ok, val] = cursor.TryGetColumnString(0); ok) record.pushKV("txid", val);
                        if (auto[ok, val] = cursor.TryGetColumnString(1); ok) record.pushKV("txidRepost", val);
                        if (auto[ok, val] = cursor.TryGetColumnString(2); ok) record.pushKV("addrFrom", val);
                        if (auto[ok, val] = cursor.TryGetColumnInt64(3); ok) record.pushKV("time", val);
                        if (auto[ok, val] = cursor.TryGetColumnInt(4); ok) record.pushKV("nblock", val);

                        result.push_back(record);
                    }
                });
            }
        );

        return result;
    }

    vector<UniValue> WebRpcRepository::GetMissedContentsScores(const string& address, int height, int limit)
    {
        vector<UniValue> result;

        SqlTransaction(
            __func__,
            [&]() -> Stmt& {
                return Sql(R"sql(
                    with
                    addr as (
                        select
                            RowId as id
                        from
                            Registry
                        where
                            String = ?
                    ),
                    height as ( select ? as value )
                    select
                        (select r.String from Registry r where r.RowId = s.RegId1) as address,
                        (select r.String from Registry r where r.RowId = s.RowId),
                        s.Time,
                        (select r.String from Registry r where r.RowId = s.RegId2) as posttxid,
                        s.Int1 as value,
                        cs.Height
                    from
                        addr,
                        height
                    cross join
                        Transactions c indexed by Transactions_Type_RegId1_RegId2_RegId3
                            on c.Type in (200, 201, 202, 209, 210) and c.RegId1 = addr.id
                    cross join
                        Last lc
                            on lc.TxId = c.RowId
                    cross join
                        Transactions s indexed by Transactions_Type_RegId2_RegId1
                            on s.Type in (300) and s.RegId2 = c.RegId2
                    cross join
                        Chain cs
                            on cs.TxId = s.RowId and cs.Height > height.value
                    order by cs.Height desc
                    limit ?
                )sql")
                .Bind(address, height, limit);
            },
            [&] (Stmt& stmt) {
                stmt.Select([&](Cursor& cursor) {
                    while (cursor.Step())
                    {
                        UniValue record(UniValue::VOBJ);

                        record.pushKV("addr", address);
                        record.pushKV("msg", "event");
                        record.pushKV("mesType", "upvoteShare");
                        if (auto[ok, value] = cursor.TryGetColumnString(0); ok) record.pushKV("addrFrom", value);
                        if (auto[ok, value] = cursor.TryGetColumnString(1); ok) record.pushKV("txid", value);
                        if (auto[ok, value] = cursor.TryGetColumnInt64(2); ok) record.pushKV("time", value);
                        if (auto[ok, value] = cursor.TryGetColumnString(3); ok) record.pushKV("posttxid", value);
                        if (auto[ok, value] = cursor.TryGetColumnInt(4); ok) record.pushKV("upvoteVal", value);
                        if (auto[ok, value] = cursor.TryGetColumnInt(5); ok) record.pushKV("nblock", value);

                        result.push_back(record);
                    }
                });
            }
        );

        return result;
    }

    vector<UniValue> WebRpcRepository::GetMissedCommentsScores(const string& address, int height, int limit)
    {
        vector<UniValue> result;

        SqlTransaction(
            __func__,
            [&]() -> Stmt& {
                return Sql(R"sql(
                    with
                    addr as (
                        select
                            RowId as id
                        from
                            Registry
                        where
                            String = ?
                    ),
                    height as ( select ? as value )
                    select
                        (select r.String from Registry r where r.RowId = s.RegId1) as address,
                        (select r.String from Registry r where r.RowId = s.RowId),
                        s.Time,
                        (select r.String from Registry r where r.RowId = s.RegId2) as posttxid,
                        s.Int1 as value,
                        cs.Height
                    from
                        addr,
                        height
                    cross join
                        Transactions c indexed by Transactions_Type_RegId1_RegId2_RegId3
                            on c.Type in (204, 205) and c.RegId1 = addr.id
                    cross join
                        Last lc
                            on lc.TxId = c.RowId
                    cross join
                        Transactions s indexed by Transactions_Type_RegId2_RegId1
                            on s.Type in (301) and s.RegId2 = c.RegId2
                    cross join
                        Chain cs
                            on cs.TxId = s.RowId and cs.Height > height.value
                    order by cs.Height desc
                    limit ?
                )sql")
                .Bind(address, height, limit);
            },
            [&] (Stmt& stmt) {
                stmt.Select([&](Cursor& cursor) {
                    while (cursor.Step())
                    {
                        UniValue record(UniValue::VOBJ);

                        record.pushKV("addr", address);
                        record.pushKV("msg", "event");
                        record.pushKV("mesType", "cScore");
                        if (auto[ok, value] = cursor.TryGetColumnString(0); ok) record.pushKV("addrFrom", value);
                        if (auto[ok, value] = cursor.TryGetColumnString(1); ok) record.pushKV("txid", value);
                        if (auto[ok, value] = cursor.TryGetColumnInt64(2); ok) record.pushKV("time", value);
                        if (auto[ok, value] = cursor.TryGetColumnString(3); ok) record.pushKV("commentid", value);
                        if (auto[ok, value] = cursor.TryGetColumnInt(4); ok) record.pushKV("upvoteVal", value);
                        if (auto[ok, value] = cursor.TryGetColumnInt(5); ok) record.pushKV("nblock", value);

                        result.push_back(record);
                    }
                });
            }
        );

        return result;
    }

    map<string, UniValue> WebRpcRepository::GetMissedTransactions(const string& address, int height, int count)
    {
        map<string, UniValue> result;

        SqlTransaction(
            __func__,
            [&]() -> Stmt& {
                return Sql(R"sql(
                    with
                    addr as (
                        select
                            RowId as id
                        from
                            Registry
                        where
                            String = ?
                    ),
                    height as ( select ? as value )
                    select
                        (select r.String from Registry r where r.RowId = t.RowId),
                        t.Time,
                        o.Value,
                        c.Height,
                        t.Type
                    from
                        addr,
                        height
                    cross join
                        TxOutputs o indexed by TxOutputs_AddressId_TxIdDesc_Number
                            on o.AddressId = addr.id
                    cross join
                        Chain c indexed by Chain_TxId_Height
                            on c.TxId = o.TxId and c.Height > height.value
                    cross join
                        Transactions t
                            on t.RowId = o.TxId
                    order by c.Height desc
                    limit ?
                )sql")
                .Bind(address, height, count);
            },
            [&] (Stmt& stmt) {
                stmt.Select([&](Cursor& cursor) {
                    while (cursor.Step())
                    {
                        auto[okTxHash, txHash] = cursor.TryGetColumnString(0);
                        if (!okTxHash) continue;

                        UniValue record(UniValue::VOBJ);
                        record.pushKV("txid", txHash);
                        record.pushKV("addr", address);
                        record.pushKV("msg", "transaction");
                        if (auto[ok, value] = cursor.TryGetColumnInt64(1); ok) record.pushKV("time", value);
                        if (auto[ok, value] = cursor.TryGetColumnInt64(2); ok) record.pushKV("amount", value);
                        if (auto[ok, value] = cursor.TryGetColumnInt(3); ok) record.pushKV("nblock", value);

                        if (auto[ok, value] = cursor.TryGetColumnInt(4); ok)
                        {
                            auto stringType = TransactionHelper::TxStringType((TxType) value);
                            if (!stringType.empty())
                                record.pushKV("type", stringType);
                        }

                        result.emplace(txHash, record);
                    }
                });
            }
        );

        return result;
    }

    vector<UniValue> WebRpcRepository::GetMissedCommentAnswers(const string& address, int height, int count)
    {
        vector<UniValue> result;

        SqlTransaction(
            __func__,
            [&]() -> Stmt& {
                return Sql(R"sql(
                    with
                    addr as (
                        select
                            RowId as id
                        from
                            Registry
                        where
                            String = ?
                    ),
                    height as ( select ? as value )
                    select
                        (select r.String from Registry r where r.RowId = a.RegId2) as RootTxHash,
                        a.Time,
                        ca.Height,
                        (select r.String from Registry r where r.RowId = a.RegId1) as addrFrom,
                        (select r.String from Registry r where r.RowId = a.RegId3) as posttxid,
                        (select r.String from Registry r where r.RowId = a.RegId4) as parentid,
                        (select r.String from Registry r where r.RowId = a.RegId5) as answerid
                    from
                        addr,
                        height
                    cross join
                        Transactions c indexed by Transactions_Type_RegId1_RegId2_RegId3
                            on c.Type in (204, 205) and c.RegId1 = addr.id
                    cross join
                        Transactions a indexed by Transactions_Type_RegId5_RegId1
                            on a.Type in (204, 205) and a.RegId5 = c.RegId2 and a.RegId1 != c.RegId1
                    cross join
                        Last la
                            on la.TxId = a.RowId
                    cross join
                        Chain ca
                            on ca.TxId = a.RowId and ca.Height > height.value
                    order by ca.Height desc
                    limit ?
                )sql")
                .Bind(address, height, count);
            },
            [&] (Stmt& stmt) {
                stmt.Select([&](Cursor& cursor) {
                    while (cursor.Step())
                    {
                        UniValue record(UniValue::VOBJ);

                        record.pushKV("addr", address);
                        record.pushKV("msg", "comment");
                        record.pushKV("mesType", "answer");
                        record.pushKV("reason", "answer");
                        if (auto[ok, value] = cursor.TryGetColumnString(0); ok) record.pushKV("txid", value);
                        if (auto[ok, value] = cursor.TryGetColumnInt64(1); ok) record.pushKV("time", value);
                        if (auto[ok, value] = cursor.TryGetColumnInt(2); ok) record.pushKV("nblock", value);
                        if (auto[ok, value] = cursor.TryGetColumnString(3); ok) record.pushKV("addrFrom", value);
                        if (auto[ok, value] = cursor.TryGetColumnString(4); ok) record.pushKV("posttxid", value);
                        if (auto[ok, value] = cursor.TryGetColumnString(5); ok) record.pushKV("parentid", value);
                        if (auto[ok, value] = cursor.TryGetColumnString(6); ok) record.pushKV("answerid", value);

                        result.push_back(record);
                    }
                });
            }
        );

        return result;
    }

    vector<UniValue> WebRpcRepository::GetMissedPostComments(const string& address, const vector<string>& excludePosts, int height, int count)
    {
        vector<UniValue> result;

        SqlTransaction(
            __func__,
            [&]() -> Stmt& {
                return Sql(R"sql(
                    with
                        addr as (
                            select
                                RowId as id
                            from
                                Registry
                            where
                                String = ?
                        ),
                        height as (
                            select ? as value
                        ),
                        excludeContent as (
                            select
                                RowId as id,
                                String as hash
                            from
                                Registry
                            where
                                String in ( )sql" + join(vector<string>(excludePosts.size(), "?"), ",") + R"sql( )
                        )
                    select
                        (select r.String from Registry r where r.RowId = c.RegId2) as RootTxHash,
                        c.Time,
                        cc.Height,
                        (select r.String from Registry r where r.RowId = c.RegId1) as addrFrom,
                        (select r.String from Registry r where r.RowId = c.RegId3) as posttxid,
                        (select r.String from Registry r where r.RowId = c.RegId4) as  parentid,
                        (select r.String from Registry r where r.RowId = c.RegId5) as  answerid,
                        (
                            select o.Value
                            from TxOutputs o indexed by TxOutputs_AddressId_TxIdDesc_Number
                            where o.TxId = c.RowId and o.AddressId = p.RegId1 and o.AddressId != c.RegId1
                        ) as Donate
                    from
                        addr,
                        height
                    cross join
                        Transactions p indexed by Transactions_Type_RegId1_RegId2_RegId3 on
                            p.Type in (200, 201, 202, 209, 210) and
                            p.RegId1 = addr.id
                    cross join
                        Last lp
                            on lp.TxId = p.RowId
                    cross join
                        Transactions c indexed by Transactions_Type_RegId3_RegId1
                            on c.Type in (204, 205) and c.RegId3 = p.RegId2 and c.RegId1 != p.RegId1
                    cross join
                        Last lc
                            on lc.TxId = c.RowId
                    cross join
                        Chain cc
                            on cc.TxId = lc.TxId and cc.Height > height.value
                    left join
                        excludeContent on
                            excludeContent.id = p.RegId2
                    where
                        excludeContent.id is null
                    order by
                        cc.Height desc
                    limit ?
                )sql")
                .Bind(address, height, excludePosts, count);
            },
            [&] (Stmt& stmt) {
                stmt.Select([&](Cursor& cursor) {
                    while (cursor.Step())
                    {
                        UniValue record(UniValue::VOBJ);

                        record.pushKV("addr", address);
                        record.pushKV("msg", "comment");
                        record.pushKV("mesType", "post");
                        record.pushKV("reason", "post");
                        if (auto[ok, value] = cursor.TryGetColumnString(0); ok) record.pushKV("txid", value);
                        if (auto[ok, value] = cursor.TryGetColumnInt64(1); ok) record.pushKV("time", value);
                        if (auto[ok, value] = cursor.TryGetColumnInt(2); ok) record.pushKV("nblock", value);
                        if (auto[ok, value] = cursor.TryGetColumnString(3); ok) record.pushKV("addrFrom", value);
                        if (auto[ok, value] = cursor.TryGetColumnString(4); ok) record.pushKV("posttxid", value);
                        if (auto[ok, value] = cursor.TryGetColumnString(5); ok) record.pushKV("parentid", value);
                        if (auto[ok, value] = cursor.TryGetColumnString(6); ok) record.pushKV("answerid", value);
                        if (auto[ok, value] = cursor.TryGetColumnInt64(7); ok)
                        {
                            record.pushKV("donation", "true");
                            record.pushKV("amount", value);
                        }

                        result.push_back(record);
                    }
                });
            }
        );

        return result;
    }

    vector<UniValue> WebRpcRepository::GetMissedSubscribers(const string& address, int height, int count)
    {
        vector<UniValue> result;

        SqlTransaction(
            __func__,
            [&]() -> Stmt& {
                return Sql(R"sql(
                    with
                    addr as (
                        select
                            RowId as id
                        from
                            Registry
                        where
                            String = ?
                    ),
                    height as ( select ? as value )
                    select
                        subs.Type,
                        (select r.String from Registry r where r.RowId = subs.RowId),
                        subs.Time,
                        cs.Height,
                        (select r.String from Registry r where r.RowId = subs.RegId1) as addrFrom,
                        p.String2 as nameFrom,
                        p.String3 as avatarFrom
                    from
                        addr,
                        height
                    cross join
                        Transactions subs indexed by Transactions_Type_RegId2_RegId1
                            on subs.Type in (302, 303, 304) and subs.RegId2 = addr.id
                    cross join
                        Last ls
                            on ls.TxId = subs.RowId
                    cross join
                        Chain cs indexed by Chain_TxId_Height
                            on cs.TxId = subs.RowId and cs.Height > height.value
                    cross join
                        Transactions u indexed by Transactions_Type_RegId1_RegId2_RegId3
                            on u.Type in (100) and u.RegId1 = subs.RegId1
                    cross join
                        Last l
                            on l.TxId = u.RowId
                    cross join
                        Payload p
                            on p.TxId = u.RowId
                    order by cs.Height desc
                    limit ?
                )sql")
                .Bind(address, height, count);
            },
            [&] (Stmt& stmt) {
                stmt.Select([&](Cursor& cursor) {
                    while (cursor.Step())
                    {
                        UniValue record(UniValue::VOBJ);

                        record.pushKV("addr", address);
                        record.pushKV("msg", "event");
                        if (auto[ok, value] = cursor.TryGetColumnInt(0); ok)
                        {
                            switch (value)
                            {
                                case 302: record.pushKV("mesType", "subscribe"); break;
                                case 303: record.pushKV("mesType", "subscribePrivate"); break;
                                case 304: record.pushKV("mesType", "unsubscribe"); break;
                                default: record.pushKV("mesType", "unknown"); break;
                            }
                        }
                        if (auto[ok, value] = cursor.TryGetColumnString(1); ok) record.pushKV("txid", value);
                        if (auto[ok, value] = cursor.TryGetColumnInt64(2); ok) record.pushKV("time", value);
                        if (auto[ok, value] = cursor.TryGetColumnInt(3); ok) record.pushKV("nblock", value);
                        if (auto[ok, value] = cursor.TryGetColumnString(4); ok) record.pushKV("addrFrom", value);
                        if (auto[ok, value] = cursor.TryGetColumnString(5); ok) record.pushKV("nameFrom", value);
                        if (auto[ok, value] = cursor.TryGetColumnString(6); ok) record.pushKV("avatarFrom", value);

                        result.push_back(record);
                    }
                });
            }
        );

        return result;
    }

    vector<UniValue> WebRpcRepository::GetMissedBoosts(const string& address, int height, int count)
    {
        vector<UniValue> result;

        SqlTransaction(
            __func__,
            [&]() -> Stmt& {
                return Sql(R"sql(
                    with
                    addr as (
                        select
                            RowId as id
                        from
                            Registry
                        where
                            String = ?
                    ),
                    height as ( select ? as value )
                    select
                        (select r.String from Registry r where r.RowId = tBoost.RegId1) as boostAddress,
                        (select r.String from Registry r where r.RowId = tBoost.RowId),
                        tBoost.Time,
                        cb.Height,
                        (select r.String from Registry r where r.RowId = tBoost.RegId2) as contenttxid,
                        (
                            (
                                select
                                    sum(io.Value)
                                from
                                    TxInputs i indexed by TxInputs_SpentTxId_Number_TxId
                                cross join TxOutputs io indexed by TxOutputs_TxId_Number_AddressId on
                                    io.TxId = i.TxId and
                                    io.Number = i.Number
                                where
                                    i.SpentTxId = tBoost.RowId
                            )
                            -
                            (
                                select
                                    sum(o.Value)
                                from
                                    TxOutputs o indexed by TxOutputs_TxId_Number_AddressId
                                where
                                    o.TxId = tBoost.RowId
                            )
                        ) as boostAmount,
                        p.String2 as boostName,
                        p.String3 as boostAvatar
                    from
                        addr,
                        height
                    cross join
                        Transactions tBoost indexed by Transactions_Type_RegId1_RegId2_RegId3
                            on tBoost.Type in (208)
                    cross join
                        Chain cb
                            on cb.TxId = tBoost.RowId and cb.Height > Height.value
                    cross join
                        Transactions tContent indexed by Transactions_Type_RegId2_RegId1
                            on tContent.Type in (200, 201, 202, 209, 210) and tContent.RegId2 = tBoost.RegId2 and tContent.RegId1 = addr.id
                    cross join
                        Transactions u indexed by Transactions_Type_RegId1_RegId2_RegId3
                            on  u.Type in (100) and u.RegId1 = tBoost.RegId1
                    cross join
                        Last lu
                            on lu.TxId = u.RowId
                    cross join
                        Payload p
                            on p.TxId = u.RowId
                    order by cb.Height desc
                    limit ?
                )sql")
                .Bind(address, height, count);
            },
            [&] (Stmt& stmt) {
                stmt.Select([&](Cursor& cursor) {
                    while (cursor.Step())
                    {
                        UniValue record(UniValue::VOBJ);

                        record.pushKV("addr", address);
                        record.pushKV("msg", "event");
                        record.pushKV("mesType", "contentBoost");
                        if (auto[ok, value] = cursor.TryGetColumnString(0); ok) record.pushKV("addrFrom", value);
                        if (auto[ok, value] = cursor.TryGetColumnString(1); ok) record.pushKV("txid", value);
                        if (auto[ok, value] = cursor.TryGetColumnInt64(2); ok) record.pushKV("time", value);
                        if (auto[ok, value] = cursor.TryGetColumnInt(3); ok) record.pushKV("nblock", value);
                        if (auto[ok, value] = cursor.TryGetColumnString(4); ok) record.pushKV("posttxid", value);
                        if (auto[ok, value] = cursor.TryGetColumnInt64(5); ok) record.pushKV("boostAmount", value);
                        if (auto[ok, value] = cursor.TryGetColumnString(6); ok) record.pushKV("nameFrom", value);
                        if (auto[ok, value] = cursor.TryGetColumnString(7); ok) record.pushKV("avatarFrom", value);

                        result.push_back(record);
                    }
                });
            }
        );

        return result;
    }

    // ------------------------- Other ---------------------

    UniValue WebRpcRepository::SearchLinks(const vector<string>& links, const vector<int>& contentTypes, const int nHeight, const int countOut)
    {
        UniValue result(UniValue::VARR);

        if (links.empty())
            return result;

        vector<int64_t> ids;
        SqlTransaction(
            __func__,
            [&]() -> Stmt& {
                return Sql(R"sql(
                    with
                        height as (
                            select ? as value
                        )
                    select
                        c.Uid
                    from
                        height
                    cross join
                        Payload p on
                            p.String7 in ( )sql" + join(vector<string>(links.size(), "?"), ",") + R"sql( )
                    cross join
                        Transactions t on
                            t.RowId = p.TxId and
                            t.Type in ( )sql" + join(vector<string>(contentTypes.size(), "?"), ",") + R"sql( )
                    cross join
                        Last l on
                            l.TxId = t.RowId
                    cross join
                        Chain c indexed by Chain_TxId_Height on
                            c.TxId = t.RowId and
                            c.Height <= height.value
                    limit ?
                )sql")
                .Bind(nHeight, links, contentTypes, countOut);
            },
            [&] (Stmt& stmt) {
                stmt.Select([&](Cursor& cursor) {
                    while (cursor.Step())
                    {
                        if (auto[ok, value] = cursor.TryGetColumnInt64(0); ok)
                            ids.push_back(value);
                    }
                });
            }
        );

        if (ids.empty())
            return result;

        auto contents = GetContentsData({}, ids, "");
        result.push_backV(contents);

        return result;
    }

    UniValue WebRpcRepository::GetContentsStatistic(const vector<string>& addresses, const vector<int>& contentTypes)
    {
        UniValue result(UniValue::VARR);

        if (addresses.empty())
            return result;

        SqlTransaction(
            __func__,
            [&]() -> Stmt& {
                return Sql(R"sql(
                    with
                        addr as (
                            select
                                RowId as id,
                                String as hash
                            from
                                Registry
                            where
                                String in ( )sql" + join(vector<string>(addresses.size(), "?"), ",") + R"sql( )
                        ),
                        stat as (
                            select
                                addr.id,
                                sum(s.Int1) as scrSum,
                                count() as scrCnt,
                                count(distinct s.RegId1) as countLikers
                            from
                                addr
                            cross join
                                Transactions v indexed by Transactions_Type_RegId1_RegId2_RegId3 on
                                    v.Type in ( )sql" + join(vector<string>(contentTypes.size(), "?"), ",") + R"sql( ) and v.RegId1 = addr.id
                            cross join
                                Last lv on
                                    lv.TxId = v.RowId
                            cross join
                                Transactions s indexed by Transactions_Type_RegId2_RegId1 on
                                    s.Type in (300) and s.RegId2 = v.RegId2
                            cross join
                                Chain cs on
                                    cs.TxId = s.RowId
                        )
                    select
                        a.hash,
                        ifnull(s.scrSum, 0),
                        ifnull(s.scrCnt, 0),
                        ifnull(s.countLikers, 0)
                    from
                        addr a
                    left join
                        stat s on
                            s.id = a.id
                    group by
                        a.hash
                )sql")
                .Bind(addresses, contentTypes);
            },
            [&] (Stmt& stmt) {
                stmt.Select([&](Cursor& cursor) {
                    while (cursor.Step())
                    {
                        UniValue record(UniValue::VOBJ);

                        cursor.Collect<string>(0, record, "address");
                        cursor.Collect<int>(1, record, "scoreSum");
                        cursor.Collect<int>(2, record, "scoreCnt");
                        cursor.Collect<int>(3, record, "countLikers");

                        result.push_back(record);
                    }
                });
            }
        );

        return result;
    }

    // ------------------------- Feeds ---------------------

    UniValue WebRpcRepository::GetHotPosts(int countOut, const int depth, const int nHeight, const string& lang,
        const vector<int>& contentTypes, const string& address, int badReputationLimit)
    {
        UniValue result(UniValue::VARR);

        vector<int64_t> ids;
        SqlTransaction(
            __func__,
            [&]() -> Stmt& {
                return Sql(R"sql(
                    with
                        lang as ( select ? as value)
                    select
                        ct.Uid
                    from
                        lang
                    cross join
                        Chain ct indexed by Chain_Height_Uid on
                            ct.Height <= ? and
                            ct.Height > ?
                    cross join
                        Transactions t on
                            t.RowId = ct.TxId and
                            t.Type in ( )sql" + join(vector<string>(contentTypes.size(), "?"), ",") + R"sql( ) and
                            t.RegId3 is null
                    cross join
                        Payload p  on
                            p.TxId = t.RowId and
                            p.String1 = lang.value
                    cross join
                        Last lt on
                            lt.TxId = t.RowId
                    cross join
                        Ratings r indexed by Ratings_Type_Uid_Last_Value on
                            r.Type = 2 and r.Uid = ct.Uid and r.Last = 1
                    cross join
                        Transactions u indexed by Transactions_Type_RegId1_RegId2_RegId3 on
                            u.Type in (100) and u.RegId1 = t.RegId1
                    cross join
                        Last lu on
                            lu.TxId = u.RowId
                    cross join
                        Chain cu on
                            cu.TxId = u.RowId
                    left join
                        Ratings ur indexed by Ratings_Type_Uid_Last_Value on
                            ur.Type = 0 and ur.Uid = cu.Uid and ur.Last = 1
                    left join
                        JuryBan jb on
                            jb.AccountId = cu.Uid and
                            jb.Ending > ?
                    left join
                        Jury j on
                            j.AccountId = cu.Uid
                    left join
                        JuryVerdict jv on
                            jv.FlagRowId = j.FlagRowId
                    where
                        -- Do not show posts from users with low reputation
                        ifnull(ur.Value,0) > ?
                        -- Do not show posts from banned users
                        and jb.AccountId is null
                        -- Do not show posts from users with active jury
                        and jv.FlagRowId is null
                    order by
                        r.Value desc
                    limit ?
                )sql")
                .Bind(
                    lang,
                    nHeight,
                    nHeight - depth,
                    contentTypes,
                    nHeight,
                    badReputationLimit,
                    countOut
                );
            },
            [&] (Stmt& stmt) {
                stmt.Select([&](Cursor& cursor) {
                    while (cursor.Step())
                        if (auto[ok, value] = cursor.TryGetColumnInt(0); ok)
                            ids.push_back(value);
                });
            }
        );

        if (ids.empty())
            return result;

        auto contents = GetContentsData({}, ids, address);
        result.push_backV(contents);

        return result;
    }

    map<int64_t, UniValue> WebRpcRepository::GetContentsData(const vector<int64_t>& ids, const string& address)
    {
        map<int64_t, UniValue> rslt;
        auto vctr = GetContentsData({}, ids, address);

        for (const auto& v : vctr)
            rslt.emplace(v["id"].get_int64(), v);

        return rslt;
    }

    map<string, UniValue> WebRpcRepository::GetContentsData(const vector<string>& hashes, const string& address)
    {
        map<string, UniValue> rslt;
        auto vctr = GetContentsData(hashes, {}, address);

        for (const auto& v : vctr)
            rslt.emplace(v["hash"].get_str(), v);

        return rslt;
    }

    vector<UniValue> WebRpcRepository::GetContentsData(const vector<string>& hashes, const vector<int64_t>& ids, const string& address)
    {
        vector<UniValue> result{};

        if (hashes.empty() && ids.empty())
            return result;

        if (!hashes.empty() && !ids.empty())
            throw std::runtime_error("Only one use: hashes or ids");

        string _withTxs = "";
        if (!hashes.empty())
        {
            _withTxs = R"sql(
                txs as (
                    select
                        RowId as id
                    from
                        Registry
                    where
                        String in ( )sql" + join(vector<string>(hashes.size(), "?"), ",") + R"sql( )
                ),
            )sql";
        }

        if (!ids.empty())
        {
            _withTxs = R"sql(
                txs as (
                    select
                        c.TxId as id
                    from
                        Chain c
                    where c.Uid in ( )sql" + join(vector<string>(ids.size(), "?"), ",") + R"sql( )
                ),
            )sql";
        }
        
        // --------------------------------------

        unordered_map<int64_t, UniValue> tmpResult{};
        vector<string> authors;
        vector<int64_t> allUIDs;

        SqlTransaction(
            __func__,
            [&]() -> Stmt& {
                return Sql(R"sql(
                    with
                    )sql" + _withTxs + R"sql(
                    addr as (
                        select
                            RowId as id
                        from
                            Registry
                        where
                            String = ?
                    )
                    select
                        (select String from Registry where RowId = t.RowId) as Hash,
                        (select String from Registry where RowId = t.RegId2) as RootTxHash,
                        c.Uid as Id,
                        case when t.RowId != t.RegId2 then 'true' else null end edit,
                        (select String from Registry where RowId = t.RegId3) as RelayTxHash,
                        (select String from Registry where RowId = t.RegId1) as AddressHash,
                        t.Time,
                        p.String1 as Lang,
                        t.Type,
                        p.String2 as Caption,
                        p.String3 as Message,
                        p.String7 as Url,
                        p.String4 as Tags,
                        p.String5 as Images,
                        p.String6 as Settings,
                        (
                            select
                                count()
                            from Transactions scr indexed by Transactions_Type_RegId2_RegId1
                            cross join Chain cscr
                                on cscr.TxId = scr.RowId
                            where
                                scr.Type in (300) and
                                scr.RegId2 = t.RegId2
                        ) as ScoresCount,
                        ifnull((
                            select
                                sum(scr.Int1)
                            from Transactions scr indexed by Transactions_Type_RegId2_RegId1
                            cross join Chain cscr
                                on cscr.TxId = scr.RowId
                            where
                                scr.Type in (300) and
                                scr.RegId2 = t.RegId2
                        ), 0) as ScoresSum,
                        (
                            select
                                count()
                            from Transactions rep indexed by Transactions_Type_RegId3_RegId1
                            join Last lrep
                                on lrep.TxId = rep.RowId
                            where
                                rep.Type in (200, 201, 202, 209, 210) and
                                rep.RegId3 = t.RegId2
                        ) as Reposted,
                        (
                            select
                                count()
                            from Transactions c indexed by Transactions_Type_RegId3_RegId1
                            cross join Last lc
                                on lc.TxId = c.RowId
                            cross join Chain cc
                                on cc.TxId = c.RowId
                            where
                                c.Type in (204, 205, 206) and
                                c.RegId3 = t.RegId2
                        ) as CommentsCount,
                        ifnull((
                            select
                                scr.Int1
                            from
                                Transactions scr indexed by Transactions_Type_RegId1_RegId2_RegId3
                            cross join Chain cscr
                                on cscr.TxId = scr.RowId
                            where
                                scr.Type in (300) and
                                scr.RegId1 = addr.id and
                                scr.RegId2 = t.RegId2
                        ), 0) as MyScore,
                        (
                            select
                                json_group_array(
                                    json_object(
                                        'h', cv.Height,
                                        'hs', (select rv.String from Registry rv where rv.RowId = tv.RowId)
                                    )
                                )
                            from
                                Transactions tv indexed by Transactions_Type_RegId2_RegId1
                            cross join Chain cv
                                on cv.TxId = tv.RowId
                            where
                                tv.Type = t.Type and
                                tv.RegId2 = t.RegId2 and
                                tv.RowId != t.RowId
                        ) as Versions,
                        (
                            select
                                json_group_object(ff.reason, ff.cnt)
                            from (
                                select
                                    f.Int1 as reason,
                                    count() as cnt
                                from
                                    Transactions f indexed by Transactions_Type_RegId2_RegId1
                                where
                                    f.Type in (410) and
                                    f.RegId2 = t.RowId
                                group by f.Int1
                            ) ff
                        ) as Flags
                    from
                        txs,
                        addr
                    cross join
                        Transactions t
                            on t.RowId = txs.id and t.Type in (200, 201, 202, 209, 210)
                    cross join
                        Chain c
                            on c.TxId = t.RowId
                    cross join
                        Last l
                            on l.TxId = t.RowId
                    cross join
                        Payload p
                            on p.TxId = t.RowId
                )sql")
                .Bind(
                    hashes,
                    ids,
                    address
                );
            },
            [&] (Stmt& stmt) {
                stmt.Select([&](Cursor& cursor) {
                    while (cursor.Step())
                    {
                        int ii = 0;
                        UniValue record(UniValue::VOBJ);

                        cursor.Collect<string>(ii++, record, "hash");
                        cursor.Collect<string>(ii++, record, "txid");
                        
                        int64_t id;
                        if (!cursor.Collect(ii++, id))
                            continue;
                        record.pushKV("id", id);
                        allUIDs.push_back(id);

                        cursor.Collect<string>(ii++, record, "edit");
                        cursor.Collect<string>(ii++, record, "repost");
                        cursor.Collect(ii++, [&](const string& value) {
                            authors.emplace_back(value);
                            record.pushKV("address", value);
                        });
                        cursor.Collect<int64_t>(ii++, record, "time");
                        cursor.Collect<string>(ii++, record, "l");

                        cursor.Collect(ii++, [&](int value) {
                            record.pushKV("type", TransactionHelper::TxStringType((TxType) value));
                            if ((TxType)value == CONTENT_DELETE)
                                record.pushKV("deleted", "true");
                        });
                        cursor.Collect<string>(ii++, record, "c");
                        cursor.Collect<string>(ii++, record, "m");
                        cursor.Collect<string>(ii++, record, "u");

                        cursor.Collect(ii++, [&](const string& value) {
                            UniValue t(UniValue::VARR);
                            t.read(value);
                            record.pushKV("t", t);
                        });

                        cursor.Collect(ii++, [&](const string& value) {
                            UniValue i(UniValue::VARR);
                            i.read(value);
                            record.pushKV("i", i);
                        });

                        cursor.Collect(ii++, [&](const string& value) {
                            UniValue s(UniValue::VOBJ);
                            s.read(value);
                            record.pushKV("s", s);
                        });

                        cursor.Collect<int>(ii++, record, "scoreCnt");
                        cursor.Collect<int>(ii++, record, "scoreSum");
                        cursor.Collect<int>(ii++, record, "reposted");
                        cursor.Collect<int>(ii++, record, "comments");

                        if (!address.empty())
                            cursor.Collect<int>(ii++, record, "myVal");
                        else
                            ii++;

                        cursor.Collect(ii++, [&](const string& value) {
                            UniValue versions(UniValue::VARR);
                            versions.read(value);
                            record.pushKV("versions", versions);
                        });

                        cursor.Collect(ii++, [&](const string& value) {
                            UniValue flags(UniValue::VOBJ);
                            flags.read(value);
                            record.pushKV("flags", flags);
                        });

                        tmpResult[id] = record;                
                    }
                });
            }
        );

        // ---------------------------------------------
        // Get last comments for all posts
        auto lastComments = GetLastComments(allUIDs, address);
        for (auto& record : tmpResult)
            record.second.pushKV("lastComment", lastComments[record.first]);

        // ---------------------------------------------
        // Get profiles for posts
        auto profiles = GetAccountProfiles(authors);
        for (auto& record : tmpResult)
            record.second.pushKV("userprofile", profiles[record.second["address"].get_str()]);

        // ---------------------------------------------
        // Place in result data with source sorting
        for (auto& id : ids)
            result.push_back(tmpResult[id]);

        return result;
    }

    vector<UniValue> WebRpcRepository::GetCollectionsData(const vector<int64_t>& ids)
    {
        vector<UniValue> result{};

        if (ids.empty())
            return result;

        unordered_map<int64_t, UniValue> tmpResult{};
        SqlTransaction(
            __func__,
            [&]() -> Stmt& {
                return Sql(R"sql(
                    select
                        (select r.String from Registry r where r.RowId = t.RegId2) as RootTxHash,
                        c.Uid,
                        case when t.RowId != t.RegId2 then 'true' else null end edit,
                        (select r.String from Registry r where r.RowId = t.RegId3) as ContentIds,
                        (select r.String from Registry r where r.RowId = t.RegId1) as AddressHash,
                        t.Time,
                        p.String1 as Lang,
                        t.Type,
                        t.Int1 as ContentTypes,
                        p.String2 as Caption,
                        p.String3 as Image

                    from
                        Chain c
                    cross join
                        Transactions t
                            on t.RowId = c.TxId and t.Type in (220)
                    cross join
                        Last l
                            on l.TxId = t.RowId
                    cross join
                        Transactions u indexed by Transactions_Type_RegId1_RegId2_RegId3
                            on u.Type in (100) and u.RegId1 = t.RegId1
                    cross join
                        Last lu
                            on lu.TxId = u.RowId
                    left join
                        Payload p
                            on p.TxId = t.RowId
                    where
                        c.Uid in ( )sql" + join(vector<string>(ids.size(), "?"), ",") + R"sql( )
                )sql")
                .Bind(ids);
            },
            [&] (Stmt& stmt) {
                stmt.Select([&](Cursor& cursor) {
                    while (cursor.Step())
                    {
                        UniValue record(UniValue::VOBJ);

                        auto[okHash, txHash] = cursor.TryGetColumnString(0);
                        auto[okId, txId] = cursor.TryGetColumnInt64(1);
                        record.pushKV("txid", txHash);
                        record.pushKV("id", txId);

                        if (auto[ok, value] = cursor.TryGetColumnString(2); ok) record.pushKV("edit", value);
                        if (auto[ok, value] = cursor.TryGetColumnString(3); ok) record.pushKV("contentIds", value);
                        if (auto[ok, value] = cursor.TryGetColumnString(4); ok)
                        {
                            record.pushKV("address", value);
                        }
                        if (auto[ok, value] = cursor.TryGetColumnString(5); ok) record.pushKV("time", value);
                        if (auto[ok, value] = cursor.TryGetColumnString(6); ok) record.pushKV("l", value); // lang
                        if (auto[ok, value] = cursor.TryGetColumnInt(7); ok)
                        {
                            record.pushKV("type", TransactionHelper::TxStringType((TxType) value));
                            if ((TxType)value == CONTENT_DELETE)
                                record.pushKV("deleted", "true");
                        }
                        if (auto[ok, value] = cursor.TryGetColumnString(8); ok) record.pushKV("contentTypes", value); // caption
                        if (auto[ok, value] = cursor.TryGetColumnString(9); ok) record.pushKV("c", value); // caption
                        if (auto[ok, value] = cursor.TryGetColumnString(10); ok) record.pushKV("i", value); // image
                        tmpResult[txId] = record;
                    }
                });
            }
        );

        // ---------------------------------------------
        // Place in result data with source sorting
        for (auto& id : ids)
            result.push_back(tmpResult[id]);

        return result;
    }

    UniValue WebRpcRepository::GetTopFeed(int countOut, const int64_t& topContentId, int topHeight,
        const string& lang, const vector<string>& tagsIncluded, const vector<int>& contentTypes,
        const vector<string>& txidsExcluded, const vector<string>& addrsExcluded, const vector<string>& tagsExcluded,
        const string& address, int depth, int badReputationLimit)
    {
        UniValue result(UniValue::VARR);
        vector<int64_t> ids;

        if (contentTypes.empty())
            return result;

        // ---------------------------------------------------

        string skipPaginationSql = "";
        if (topContentId > 0)
        {
            skipPaginationSql = R"sql( and t.RowId < (select max(cc.TxId) from Chain cc where cc.Uid = ?) )sql";
        }

        // ---------------------------------------------------

        string tagsIncludedSql = "";
        if (!tagsIncluded.empty())
        {
            tagsIncludedSql = R"sql(
                and t.RowId in (
                    select
                        tm.ContentId
                    from
                        web.Tags tag indexed by Tags_Lang_Value_Id
                    join
                        web.TagsMap tm indexed by TagsMap_TagId_ContentId on
                            tm.TagId = tag.Id
                    where
                        tag.Value in ( )sql" + join(vector<string>(tagsIncluded.size(), "?"), ",") + R"sql( ) and
                        ( ? or tag.Lang = ? )
                )
            )sql";
        }

        // ---------------------------------------------------

        string tagsExcludedSql = "";
        if (!tagsExcluded.empty())
        {
            tagsExcludedSql = R"sql(
                and t.RowId not in (
                    select
                        tm.ContentId
                    from
                        web.Tags tag indexed by Tags_Lang_Value_Id
                    join
                        web.TagsMap tm indexed by TagsMap_TagId_ContentId on
                            tm.TagId = tag.Id
                    where
                        tag.Value in ( )sql" + join(vector<string>(tagsExcluded.size(), "?"), ",") + R"sql( ) and
                        ( ? or tag.Lang = ? )
                )
            )sql";
        }

        // ---------------------------------------------------

        string sql = R"sql(
            select
                ct.Uid
            from
                Chain ct
            cross join
                Transactions t on
                    t.RowId = ct.TxId and
                    t.Type in ( )sql" + join(vector<string>(contentTypes.size(), "?"), ",") + R"sql( )
            cross join
                Last lt on
                    lt.TxId = t.RowId
            cross join
                Payload p on
                    p.TxId = t.RowId and
                    ( ? or p.String1 = ? )
            cross join
                Ratings cr indexed by Ratings_Type_Uid_Last_Value on
                    cr.Type = 2 and
                    cr.Uid = ct.Uid and
                    cr.Last = 1 and
                    cr.Value > 0
            cross join
                Transactions u indexed by Transactions_Type_RegId1_RegId2_RegId3 on
                    u.Type in (100) and u.RegId1 = t.RegId1
            cross join
                Last lu on
                    lu.TxId = u.RowId
            cross join
                Chain cu on
                    cu.TxId = u.RowId
            left join
                Ratings ur indexed by Ratings_Type_Uid_Last_Value on
                    ur.Type = 0 and
                    ur.Uid = cu.Uid and
                    ur.Last = 1
            left join
                JuryBan jb on
                    jb.AccountId = cu.Uid and
                    jb.Ending > ?
            left join
                Jury j on
                    j.AccountId = cu.Uid
            left join
                JuryVerdict jv on
                    jv.FlagRowId = j.FlagRowId
            where

                    ct.Height > ?
                and ct.Height <= ?

                -- Do not show posts from users with low reputation
                and ifnull(ur.Value,0) > ?

                -- Do not show posts from banned users
                and jb.AccountId is null

                -- Do not show posts from users with active jury
                and jv.FlagRowId is null

                -- Skip ids for pagination
                )sql" + skipPaginationSql + R"sql(

                -- Exclude posts
                and t.RegId2 not in (
                    select
                        r.RowId
                    from
                        Registry r
                    where
                        r.String in ( )sql" + join(vector<string>(txidsExcluded.size(), "?"), ",") + R"sql( )
                )

                -- Exclude authors
                and t.RegId1 not in (
                    select
                        r.RowId
                    from
                        Registry r
                    where
                        r.String in ( )sql" + join(vector<string>(addrsExcluded.size(), "?"), ",") + R"sql( )
                )

                -- Skip blocked authors
                and t.RegId1 not in (
                    select
                        bl.IdTarget
                    from
                        BlockingLists bl
                    where
                        bl.IdSource = ( select r.RowId from Registry r where r.String = ? )
                )

                )sql" + tagsIncludedSql + R"sql(

                )sql" + tagsExcludedSql + R"sql(

            order by
                cr.Value desc

            limit ?
        )sql";

        // ---------------------------------------------------

        SqlTransaction(
            __func__,
            [&]() -> Stmt& {
                auto& stmt = Sql(sql);

                stmt.Bind(
                    contentTypes,
                    lang.empty(),
                    lang,
                    topHeight,
                    topHeight - depth,
                    topHeight,
                    badReputationLimit
                );

                if (topContentId > 0)
                {
                    stmt.Bind(
                        topContentId
                    );
                }
                
                stmt.Bind(
                    txidsExcluded,
                    addrsExcluded,
                    address
                );

                if (!tagsIncluded.empty())
                {
                    stmt.Bind(
                        tagsIncluded,
                        lang.empty(),
                        lang
                    );
                }

                if (!tagsExcluded.empty())
                {
                    stmt.Bind(
                        tagsExcluded,
                        lang.empty(),
                        lang
                    );
                }

                stmt.Bind(
                    countOut
                );

                return stmt;
            },
            [&] (Stmt& stmt) {
                stmt.Select([&](Cursor& cursor) {
                    while (cursor.Step())
                    {
                        auto[ok0, contentId] = cursor.TryGetColumnInt64(0);
                        ids.push_back(contentId);
                    }
                });
            }
        );

        // Get content data
        if (!ids.empty())
        {
            auto contents = GetContentsData({}, ids, address);
            result.push_backV(contents);
        }

        // Complete!
        return result;
    }

    UniValue WebRpcRepository::GetMostCommentedFeed(int countOut, const int64_t& topContentId, int topHeight,
        const string& lang, const vector<string>& tagsIncluded, const vector<int>& contentTypes,
        const vector<string>& txidsExcluded, const vector<string>& addrsExcluded, const vector<string>& tagsExcluded,
        const string& address, int depth, int badReputationLimit)
    {
        UniValue result(UniValue::VARR);
        vector<int64_t> ids;

        if (contentTypes.empty())
            return result;

        // ---------------------------------------------------

        string skipPaginationSql = "";
        if (topContentId > 0)
        {
            skipPaginationSql = R"sql( and t.RowId < (select max(cc.TxId) from Chain cc where cc.Uid = ?) )sql";
        }

        // ---------------------------------------------------

        string tagsIncludedSql = "";
        if (!tagsIncluded.empty())
        {
            tagsIncludedSql = R"sql(
                and t.RowId in (
                    select
                        tm.ContentId
                    from
                        web.Tags tag indexed by Tags_Lang_Value_Id
                    join
                        web.TagsMap tm indexed by TagsMap_TagId_ContentId on
                            tm.TagId = tag.Id
                    where
                        tag.Value in ( )sql" + join(vector<string>(tagsIncluded.size(), "?"), ",") + R"sql( ) and
                        ( ? or tag.Lang = ? )
                )
            )sql";
        }

        // ---------------------------------------------------

        string tagsExcludedSql = "";
        if (!tagsExcluded.empty())
        {
            tagsExcludedSql = R"sql(
                and t.RowId not in (
                    select
                        tm.ContentId
                    from
                        web.Tags tag indexed by Tags_Lang_Value_Id
                    join
                        web.TagsMap tm indexed by TagsMap_TagId_ContentId on
                            tm.TagId = tag.Id
                    where
                        tag.Value in ( )sql" + join(vector<string>(tagsExcluded.size(), "?"), ",") + R"sql( ) and
                        ( ? or tag.Lang = ? )
                )
            )sql";
        }

        // ---------------------------------------------------

        string sql = R"sql(
            select
                ct.Uid
            from
                Chain ct
            cross join
                Transactions t on
                    t.RowId = ct.TxId and
                    t.Type in ( )sql" + join(vector<string>(contentTypes.size(), "?"), ",") + R"sql( )
            cross join
                Last lt on
                    lt.TxId = t.RowId
            cross join
                Payload p on
                    p.TxId = t.RowId and
                    ( ? or p.String1 = ? )
            cross join
                Ratings cr indexed by Ratings_Type_Uid_Last_Value on
                    cr.Type = 2 and
                    cr.Uid = ct.Uid and
                    cr.Last = 1 and
                    cr.Value > 0
            cross join
                Transactions u indexed by Transactions_Type_RegId1_RegId2_RegId3 on
                    u.Type in (100) and u.RegId1 = t.RegId1
            cross join
                Last lu on
                    lu.TxId = u.RowId
            cross join
                Chain cu on
                    cu.TxId = u.RowId
            left join
                Ratings ur indexed by Ratings_Type_Uid_Last_Value on
                    ur.Type = 0 and
                    ur.Uid = cu.Uid and
                    ur.Last = 1
            left join
                JuryBan jb on
                    jb.AccountId = cu.Uid and
                    jb.Ending > ?
            left join
                Jury j on
                    j.AccountId = cu.Uid
            left join
                JuryVerdict jv on
                    jv.FlagRowId = j.FlagRowId
            where

                    ct.Height > ?
                and ct.Height <= ?

                -- Do not show posts from users with low reputation
                and ifnull(ur.Value,0) > ?

                -- Do not show posts from banned users
                and jb.AccountId is null

                -- Do not show posts from users with active jury
                and jv.FlagRowId is null

                -- Skip ids for pagination
                )sql" + skipPaginationSql + R"sql(

                -- Exclude posts
                and t.RegId2 not in (
                    select
                        r.RowId
                    from
                        Registry r
                    where
                        r.String in ( )sql" + join(vector<string>(txidsExcluded.size(), "?"), ",") + R"sql( )
                )

                -- Exclude authors
                and t.RegId1 not in (
                    select
                        r.RowId
                    from
                        Registry r
                    where
                        r.String in ( )sql" + join(vector<string>(addrsExcluded.size(), "?"), ",") + R"sql( )
                )

                )sql" + tagsIncludedSql + R"sql(

                )sql" + tagsExcludedSql + R"sql(

            order by (
                select
                    count()
                from
                    Transactions cmnt indexed by Transactions_Type_RegId3_RegId1
                cross join
                    Last ls on
                        ls.TxId = cmnt.RowId
                left join
                    BlockingLists bl on
                        bl.IdSource = t.RegId1 and
                        bl.IdTarget = cmnt.RegId1
                where
                    cmnt.Type in (204, 205, 206) and
                    cmnt.RegId3 = t.RegId2
            ) desc

            limit ?
        )sql";

        // ---------------------------------------------------

        SqlTransaction(
            __func__,
            [&]() -> Stmt& {
                auto& stmt = Sql(sql);

                stmt.Bind(
                    contentTypes,
                    lang.empty(),
                    lang,
                    topHeight,
                    topHeight - depth,
                    topHeight,
                    badReputationLimit
                );

                if (topContentId > 0)
                {
                    stmt.Bind(
                        topContentId
                    );
                }
                
                stmt.Bind(
                    txidsExcluded,
                    addrsExcluded
                );

                if (!tagsIncluded.empty())
                {
                    stmt.Bind(
                        tagsIncluded,
                        lang.empty(),
                        lang
                    );
                }

                if (!tagsExcluded.empty())
                {
                    stmt.Bind(
                        tagsExcluded,
                        lang.empty(),
                        lang
                    );
                }

                stmt.Bind(
                    countOut
                );

                return stmt;
            },
            [&] (Stmt& stmt) {
                stmt.Select([&](Cursor& cursor) {
                    while (cursor.Step())
                    {
                        auto[ok0, contentId] = cursor.TryGetColumnInt64(0);
                        ids.push_back(contentId);
                    }
                });
            }
        );

        // Get content data
        if (!ids.empty())
        {
            auto contents = GetContentsData({}, ids, address);
            result.push_backV(contents);
        }

        // Complete!
        return result;
    }

    UniValue WebRpcRepository::GetProfileFeed(const string& addressFeed, int countOut, int pageNumber, const int64_t& topContentId, int topHeight,
        const string& lang, const vector<string>& tagsIncluded, const vector<int>& contentTypes, const vector<string>& txidsExcluded, 
        const vector<string>& addrsExcluded, const vector<string>& tagsExcluded, const string& address, const string& orderby, const string& ascdesc)
    {
        UniValue result(UniValue::VARR);

        if (addressFeed.empty())
            return result;

        string sorting = " ct.Uid ";
        if (orderby == "comment")
        {
            sorting = R"sql(
                (
                    select
                        count()
                    from
                        Transactions s indexed by Transactions_Type_RegId3_RegId1
                    cross join
                        Last l on
                            l.TxId = s.RowId
                    where
                        s.Type in (204, 205, 206) and
                        s.RegId3 = t.RegId2
                )
            )sql";
        }
        if (orderby == "score")
        {
            sorting = R"sql(
                (
                    select
                        count()
                    from
                        Transactions scr indexed by Transactions_Type_RegId2_RegId1
                    cross join
                        Chain cscr on
                            cscr.TxId = scr.RowId
                    where
                        scr.Type in (300) and
                        scr.RegId2 = t.RegId2
                )
            )sql";
        }
        sorting += " " + ascdesc;

        vector<int64_t> ids;
        SqlTransaction(
            __func__,
            [&]() -> Stmt& {
                return Sql(R"sql(
                    with
                        height as ( select ? as value ),
                        addr as ( select RowId as id, String as hash from Registry where String = ?),
                        lang as ( select ? as value ),
                        topContentId as ( select ? as value )
                    select
                        distinct ct.Uid
                    from
                        height,
                        addr,
                        lang,
                        topContentId
                    cross join
                        Transactions t indexed by Transactions_Type_RegId1_RegId2_RegId3 on
                            t.Type in ( )sql" + join(vector<string>(contentTypes.size(), "?"), ",") + R"sql( ) and
                            t.RegId1 = addr.id
                    cross join
                        Last lt on
                            lt.TxId = t.RowId
                    cross join
                        Chain ct on
                            ct.TxId = t.RowId and
                            ct.Height <= height.value and
                            ( ? or ct.Uid < topContentId.value )
                    cross join
                        Payload p on
                            p.TxId = t.RowId and
                            ( ? or p.String1 = lang.value )
                    left join
                        web.TagsMap tm on
                            tm.ContentId = ct.Uid
                    left join
                        web.Tags tg on
                            tg.Id = tm.TagId
                    where
                        ( ? or tg.Lang = lang.value ) and
                        ( ? or tg.Value in ( )sql" + join(vector<string>(tagsIncluded.size(), "?"), ",") + R"sql( ) ) and
                        ( ? or tg.Value not in ( )sql" + join(vector<string>(tagsExcluded.size(), "?"), ",") + R"sql( ) ) and
                        (
                            ? or
                            t.RegId2 not in (
                                select
                                    r.RowId
                                from
                                    Registry r
                                where
                                    r.String in ( )sql" + join(vector<string>(txidsExcluded.size(), "?"), ",") + R"sql( )
                            )
                        )
                    order by )sql" + sorting + R"sql(
                    limit ?
                    offset ?
                )sql")
                .Bind(
                    topHeight,
                    addressFeed,
                    lang,
                    topContentId,
                    contentTypes,
                    topContentId <= 0,
                    lang.empty(),
                    lang.empty(),
                    tagsIncluded.empty(),
                    tagsIncluded,
                    tagsExcluded.empty(),
                    tagsExcluded,
                    txidsExcluded.empty(),
                    txidsExcluded,
                    countOut,
                    pageNumber * countOut
                );
            },
            [&] (Stmt& stmt) {
                stmt.Select([&](Cursor& cursor) {
                    while (cursor.Step())
                    {
                        if (auto[ok, value] = cursor.TryGetColumnInt64(0); ok)
                            ids.push_back(value);
                    }
                });
            }
        );

        // Get content data
        if (!ids.empty())
        {
            auto contents = GetContentsData({}, ids, address);
            result.push_backV(contents);
        }

        return result;
    }

    UniValue WebRpcRepository::GetSubscribesFeed(const string& addressFeed, int countOut, const int64_t& topContentId, int topHeight,
        const string& lang, const vector<string>& tagsIncluded, const vector<int>& contentTypes,
        const vector<string>& txidsExcluded, const vector<string>& addrsExcluded, const vector<string>& tagsExcluded,
        const string& address, const vector<string>& addresses_extended)
    {
        UniValue result(UniValue::VARR);

        // Select RowId for top subscribes
        vector<int64_t> subsIds;
        SqlTransaction(
            __func__,
            [&]() -> Stmt& {
                return
                    Sql(R"sql(
                        select
                            q.RegId2
                        from (
                            select
                                distinct
                                s.RegId2
                            from
                                Transactions s indexed by Transactions_Type_RegId1_RegId2_RegId3
                            cross join
                                Last ls on
                                    ls.TxId = s.RowId
                            where
                                s.Type in (302, 303) and
                                s.RegId1 = (select RowId from Registry where String = ?)
                            order by
                                s.RowId desc
                            limit 100
                        )q

                        union

                        select
                            r.RowId
                        from
                            Registry r
                        where
                            r.String in ( )sql" + join(vector<string>(addresses_extended.size(), "?"), ",") + R"sql( )
                    )sql")
                    .Bind(
                        addressFeed,
                        addresses_extended
                    );
            },
            [&] (Stmt& stmt) {
                stmt.Select([&](Cursor& cursor) {
                    while (cursor.Step()) {
                        cursor.Collect(0, [&](int64_t val) {
                            subsIds.push_back(val);
                        });                            
                    }
                });
            }
        );

        if (subsIds.empty())
            return result;

        // ---------------------------------------------------

        string skipPaginationSql = "";
        if (topContentId > 0)
        {
            skipPaginationSql = R"sql( and t.RowId < (select max(cc.TxId) from Chain cc where cc.Uid = ?) )sql";
        }

        // ---------------------------------------------------

        string tagsIncludedSql = "";
        if (!tagsIncluded.empty())
        {
            tagsIncludedSql = R"sql(
                and t.RowId in (
                    select
                        tm.ContentId
                    from
                        web.Tags tag indexed by Tags_Lang_Value_Id
                    join
                        web.TagsMap tm indexed by TagsMap_TagId_ContentId on
                            tm.TagId = tag.Id
                    where
                        tag.Value in ( )sql" + join(vector<string>(tagsIncluded.size(), "?"), ",") + R"sql( ) and
                        ( ? or tag.Lang = ? )
                )
            )sql";
        }

        // ---------------------------------------------------

        string tagsExcludedSql = "";
        if (!tagsExcluded.empty())
        {
            tagsExcludedSql = R"sql(
                and t.RowId not in (
                    select
                        tm.ContentId
                    from
                        web.Tags tag indexed by Tags_Lang_Value_Id
                    join
                        web.TagsMap tm indexed by TagsMap_TagId_ContentId on
                            tm.TagId = tag.Id
                    where
                        tag.Value in ( )sql" + join(vector<string>(tagsExcluded.size(), "?"), ",") + R"sql( ) and
                        ( ? or tag.Lang = ? )
                )
            )sql";
        }

        // ---------------------------------------------------

        string sql = R"sql(
            select
                ct.Uid
            from
                Transactions t indexed by Transactions_RowId_desc_Type
            cross join
                Payload p on
                    p.TxId = t.RowId and
                    ( ? or p.String1 = ? )
            cross join
                Last lt on
                    lt.TxId = t.RowId
            cross join
                Chain ct indexed by Chain_TxId_Height on
                    ct.TxId = t.RowId and
                    ct.Height <= ?
            cross join
                Transactions u indexed by Transactions_Type_RegId1_RegId2_RegId3 on
                    u.Type in (100) and u.RegId1 = t.RegId1
            cross join
                Chain cu on
                    cu.TxId = u.RowId
            cross join
                Last lu on
                    lu.TxId = u.RowId
            left join
                JuryBan jb on
                    jb.AccountId = cu.Uid and
                    jb.Ending > ?
            left join
                Jury j on
                    j.AccountId = cu.Uid
            left join
                JuryVerdict jv on
                    jv.FlagRowId = j.FlagRowId
            where
                t.Type in ( )sql" + join(vector<string>(contentTypes.size(), "?"), ",") + R"sql( )
                and t.RegId3 is null

                -- Do not show posts from banned users
                and jb.AccountId is null

                -- Do not show posts from users with active jury
                and jv.FlagRowId is null

                -- Skip ids for pagination
                )sql" + skipPaginationSql + R"sql(

                -- Include authors only in subscribes
                and t.RegId1 in ( )sql" + join(vector<string>(subsIds.size(), "?"), ",") + R"sql( )

                -- Exclude posts
                and t.RegId2 not in (
                    select
                        r.RowId
                    from
                        Registry r
                    where
                        r.String in ( )sql" + join(vector<string>(txidsExcluded.size(), "?"), ",") + R"sql( )
                )

                -- Exclude authors
                and t.RegId1 not in (
                    select
                        r.RowId
                    from
                        Registry r
                    where
                        r.String in ( )sql" + join(vector<string>(addrsExcluded.size(), "?"), ",") + R"sql( )
                )

                -- Skip blocked authors
                and t.RegId1 not in (
                    select
                        bl.IdTarget
                    from
                        BlockingLists bl
                    where
                        bl.IdSource = ( select r.RowId from Registry r where r.String = ? )
                )

                )sql" + tagsIncludedSql + R"sql(

                )sql" + tagsExcludedSql + R"sql(

            limit ?
        )sql";

        // ---------------------------------------------------

        vector<int64_t> ids;
        SqlTransaction(
            __func__,
            [&]() -> Stmt& {
                auto& stmt = Sql(sql);

                stmt.Bind(
                    lang.empty(),
                    lang,
                    topHeight,
                    topHeight,
                    contentTypes
                );

                if (topContentId > 0)
                {
                    stmt.Bind(
                        topContentId
                    );
                }
                
                stmt.Bind(
                    subsIds,
                    txidsExcluded,
                    addrsExcluded,
                    address
                );

                if (!tagsIncluded.empty())
                {
                    stmt.Bind(
                        tagsIncluded,
                        lang.empty(),
                        lang
                    );
                }

                if (!tagsExcluded.empty())
                {
                    stmt.Bind(
                        tagsExcluded,
                        lang.empty(),
                        lang
                    );
                }

                stmt.Bind(
                    countOut
                );

                return stmt;
            },
            [&] (Stmt& stmt) {
                stmt.Select([&](Cursor& cursor) {
                    while (cursor.Step())
                    {
                        if (auto[ok, value] = cursor.TryGetColumnInt64(0); ok)
                            ids.push_back(value);
                    }
                });
            }
        );

        // Get content data
        if (!ids.empty())
        {
            auto contents = GetContentsData({}, ids, address);
            result.push_backV(contents);
        }

        // Complete!
        return result;
    }

    UniValue WebRpcRepository::GetHistoricalFeed(int countOut, const int64_t& topContentId, int topHeight,
        const string& lang, const vector<string>& tagsIncluded, const vector<int>& contentTypes,
        const vector<string>& txidsExcluded, const vector<string>& addrsExcluded, const vector<string>& tagsExcluded,
        const string& address, int badReputationLimit)
    {
        UniValue result(UniValue::VARR);
        vector<int64_t> ids;

        // ---------------------------------------------------

        string skipPaginationSql = "";
        if (topContentId > 0)
        {
            skipPaginationSql = R"sql( and t.RowId < (select min(cc.TxId) from Chain cc where cc.Uid = ?) )sql";
        }

        // ---------------------------------------------------

        string tagsIncludedSql = "";
        if (!tagsIncluded.empty())
        {
            tagsIncludedSql = R"sql(
                and t.RowId in (
                    select
                        tm.ContentId
                    from
                        web.Tags tag indexed by Tags_Lang_Value_Id
                    join
                        web.TagsMap tm indexed by TagsMap_TagId_ContentId on
                            tm.TagId = tag.Id
                    where
                        tag.Value in ( )sql" + join(vector<string>(tagsIncluded.size(), "?"), ",") + R"sql( ) and
                        ( ? or tag.Lang = ? )
                )
            )sql";
        }

        // ---------------------------------------------------

        string tagsExcludedSql = "";
        if (!tagsExcluded.empty())
        {
            tagsExcludedSql = R"sql(
                and t.RowId not in (
                    select
                        tm.ContentId
                    from
                        web.Tags tag indexed by Tags_Lang_Value_Id
                    join
                        web.TagsMap tm indexed by TagsMap_TagId_ContentId on
                            tm.TagId = tag.Id
                    where
                        tag.Value in ( )sql" + join(vector<string>(tagsExcluded.size(), "?"), ",") + R"sql( ) and
                        ( ? or tag.Lang = ? )
                )
            )sql";
        }

        // ---------------------------------------------------

        string sql = R"sql(
            select
                ct.Uid
            from
                Transactions t indexed by Transactions_RowId_desc_Type
            cross join
                Payload p on
                    p.TxId = t.RowId and
                    ( ? or p.String1 = ? )
            cross join
                Last lt on
                    lt.TxId = t.RowId
            cross join
                Chain ct indexed by Chain_TxId_Height on
                    ct.TxId = t.RowId and
                    ct.Height > ? and
                    ct.Height <= ?
            cross join
                Transactions u indexed by Transactions_Type_RegId1_RegId2_RegId3 on
                    u.Type in (100) and u.RegId1 = t.RegId1
            cross join
                Last lu on
                    lu.TxId = u.RowId
            cross join
                Chain cu on
                    cu.TxId = u.RowId
            left join
                Ratings ur indexed by Ratings_Type_Uid_Last_Value on
                    ur.Type = 0 and
                    ur.Uid = cu.Uid and
                    ur.Last = 1
            left join
                JuryBan jb on
                    jb.AccountId = cu.Uid and
                    jb.Ending > ?
            left join
                Jury j on
                    j.AccountId = cu.Uid
            left join
                JuryVerdict jv on
                    jv.FlagRowId = j.FlagRowId
            where
                t.Type in ( )sql" + join(vector<string>(contentTypes.size(), "?"), ",") + R"sql( )
                and t.RegId3 is null

                -- Do not show posts from users with low reputation
                and ifnull(ur.Value,0) > ?

                -- Do not show posts from banned users
                and jb.AccountId is null

                -- Do not show posts from users with active jury
                and jv.FlagRowId is null

                -- Skip ids for pagination
                )sql" + skipPaginationSql + R"sql(

                -- Exclude posts
                and t.RegId2 not in (
                    select
                        r.RowId
                    from
                        Registry r
                    where
                        r.String in ( )sql" + join(vector<string>(txidsExcluded.size(), "?"), ",") + R"sql( )
                )

                -- Exclude authors
                and t.RegId1 not in (
                    select
                        r.RowId
                    from
                        Registry r
                    where
                        r.String in ( )sql" + join(vector<string>(addrsExcluded.size(), "?"), ",") + R"sql( )
                )

                -- Skip blocked authors
                and t.RegId1 not in (
                    select
                        bl.IdTarget
                    from
                        BlockingLists bl
                    where
                        bl.IdSource = ( select r.RowId from Registry r where r.String = ? )
                )

                )sql" + tagsIncludedSql + R"sql(

                )sql" + tagsExcludedSql + R"sql(

            limit ?
        )sql";

        // ---------------------------------------------------

        SqlTransaction(
            __func__,
            [&]() -> Stmt& {
                auto& stmt = Sql(sql);

                stmt.Bind(
                    lang.empty(),
                    lang,
                    0,
                    topHeight,
                    topHeight,
                    contentTypes,
                    badReputationLimit
                );

                if (topContentId > 0)
                {
                    stmt.Bind(
                        topContentId
                    );
                }
                
                stmt.Bind(
                    txidsExcluded,
                    addrsExcluded,
                    address
                );

                if (!tagsIncluded.empty())
                {
                    stmt.Bind(
                        tagsIncluded,
                        lang.empty(),
                        lang
                    );
                }

                if (!tagsExcluded.empty())
                {
                    stmt.Bind(
                        tagsExcluded,
                        lang.empty(),
                        lang
                    );
                }

                stmt.Bind(
                    countOut
                );

                return stmt;
            },
            [&] (Stmt& stmt) {
                stmt.Select([&](Cursor& cursor) {
                    while (cursor.Step())
                    {
                        if (auto[ok, contentId] = cursor.TryGetColumnInt64(0); ok)
                            ids.push_back(contentId);
                    }
                });
            }
        );

        // Get content data
        if (!ids.empty())
        {
            auto contents = GetContentsData({}, ids, address);
            result.push_backV(contents);
        }

        return result;
    }

    UniValue WebRpcRepository::GetHierarchicalFeed(int countOut, const int64_t& topContentId, int topHeight,
        const string& lang, const vector<string>& tagsIncluded, const vector<int>& contentTypes,
        const vector<string>& txidsExcluded, const vector<string>& addrsExcluded, const vector<string>& tagsExcluded,
        const string& address, int badReputationLimit)
    {
        UniValue result(UniValue::VARR);

        string tagsIncludedSql = "";
        if (!tagsIncluded.empty())
        {
            tagsIncludedSql = R"sql(
                and t.RowId in (
                    select
                        tm.ContentId
                    from
                        web.Tags tag indexed by Tags_Lang_Value_Id
                    join
                        web.TagsMap tm indexed by TagsMap_TagId_ContentId on
                            tm.TagId = tag.Id
                    where
                        tag.Value in ( )sql" + join(vector<string>(tagsIncluded.size(), "?"), ",") + R"sql( ) and
                        ( ? or tag.Lang = ? )
                )
            )sql";
        }

        string tagsExcludedSql = "";
        if (!tagsExcluded.empty())
        {
            tagsExcludedSql = R"sql(
                and t.RowId not in (
                    select
                        tm.ContentId
                    from
                        web.Tags tag indexed by Tags_Lang_Value_Id
                    join
                        web.TagsMap tm indexed by TagsMap_TagId_ContentId on
                            tm.TagId = tag.Id
                    where
                        tag.Value in ( )sql" + join(vector<string>(tagsExcluded.size(), "?"), ",") + R"sql( ) and
                        ( ? or tag.Lang = ? )
                )
            )sql";
        }

        string sql = R"sql(
            select
                ct.Uid,
                ifnull(pr.Value, 0) as ContentRating,
                ifnull(ur.Value, 0) as AccountRating,
                ctorig.Height,
                ifnull((select Data from AccountStatistic a where a.AccountRegId = t.RegId1 and a.Type = 8), 0) as SumRatingsLast5Contents
            from
                Chain ct indexed by Chain_Height_Uid
            cross join
                Transactions t on
                    t.RowId = ct.TxId and
                    t.Type in ( )sql" + join(vector<string>(contentTypes.size(), "?"), ",") + R"sql( ) and
                    t.RegId3 is null
            cross join
                Payload p on
                    p.TxId = t.RowId and
                    ( ? or p.String1 = ? )
            cross join
                Last lt on
                    lt.TxId = t.RowId
            cross join
                Transactions torig on
                    torig.RowId = t.RegId2
            cross join
                Chain ctorig on
                    ctorig.TxId = torig.RowId
            cross join
                Transactions u indexed by Transactions_Type_RegId1_RegId2_RegId3 on
                    u.Type in (100) and u.RegId1 = t.RegId1
            cross join
                Last lu on
                    lu.TxId = u.RowId
            cross join
                Chain cu on
                    cu.TxId = u.RowId
            left join
                Ratings pr indexed by Ratings_Type_Uid_Last_Height on
                    pr.Type = 2 and
                    pr.Uid = ct.Uid and
                    pr.Last = 1
            left join
                Ratings ur indexed by Ratings_Type_Uid_Last_Value on
                    ur.Type = 0 and
                    ur.Uid = cu.Uid and
                    ur.Last = 1
            left join
                JuryBan jb on
                    jb.AccountId = cu.Uid and
                    jb.Ending > ?
            left join
                Jury j on
                    j.AccountId = cu.Uid
            left join
                JuryVerdict jv on
                    jv.FlagRowId = j.FlagRowId
            where
                ct.Height > ?
                and ct.Height <= ?

                -- Do not show posts from users with low reputation
                and ifnull(ur.Value,0) > ?

                -- Do not show posts from banned users
                and jb.AccountId is null

                -- Do not show posts from users with active jury
                and jv.FlagRowId is null

                -- Exclude posts
                and t.RegId2 not in (
                    select
                        r.RowId
                    from
                        Registry r
                    where
                        r.String in ( )sql" + join(vector<string>(txidsExcluded.size(), "?"), ",") + R"sql( )
                )

                -- Exclude authors
                and t.RegId1 not in (
                    select
                        r.RowId
                    from
                        Registry r
                    where
                        r.String in ( )sql" + join(vector<string>(addrsExcluded.size(), "?"), ",") + R"sql( )
                )

                -- Skip blocked authors
                and t.RegId1 not in (
                    select
                        bl.IdTarget
                    from
                        BlockingLists bl
                    where
                        bl.IdSource = ( select r.RowId from Registry r where r.String = ? )
                )

                )sql" + tagsIncludedSql + R"sql(

                )sql" + tagsExcludedSql + R"sql(
        )sql";

        // ---------------------------------------------
        vector<HierarchicalRecord> postsRanks;
        double dekay = (contentTypes.size() == 1 && contentTypes[0] == CONTENT_VIDEO) ? dekayVideo : dekayContent;

        SqlTransaction(
            __func__,
            [&]() -> Stmt& {
                auto& stmt = Sql(sql);

                stmt.Bind(
                    contentTypes,
                    lang.empty(),
                    lang,
                    topHeight,
                    topHeight - cntBlocksForResult,
                    topHeight,
                    badReputationLimit,
                    txidsExcluded,
                    addrsExcluded,
                    address
                );

                if (!tagsIncluded.empty())
                {
                    stmt.Bind(
                        tagsIncluded,
                        lang.empty(),
                        lang
                    );
                }

                if (!tagsExcluded.empty())
                {
                    stmt.Bind(
                        tagsExcluded,
                        lang.empty(),
                        lang
                    );
                }

                return stmt;
            },
            [&] (Stmt& stmt) {
                stmt.Select([&](Cursor& cursor) {
                    while (cursor.Step())
                    {
                        HierarchicalRecord record{};

                        int64_t contentId;
                        int contentRating, accountRating, contentOrigHeight, contentScores;

                        cursor.CollectAll(contentId, contentRating, accountRating, contentOrigHeight, contentScores);

                        record.Id = contentId;
                        record.LAST5 = 1.0 * contentScores;
                        record.UREP = accountRating;
                        record.PREP = contentRating;
                        record.DREP = pow(dekayRep, (topHeight - contentOrigHeight));
                        record.DPOST = pow(dekay, (topHeight - contentOrigHeight));

                        postsRanks.push_back(record);
                    }
                });
            }
        );

        // ---------------------------------------------

        // Calculate content ratings
        int nElements = postsRanks.size();
        for (auto& iPostRank : postsRanks)
        {
            double _LAST5R = 0;
            double _UREPR = 0;
            double _PREPR = 0;

            if (nElements > 1)
            {
                for (auto jPostRank : postsRanks)
                {
                    if (iPostRank.LAST5 > jPostRank.LAST5)
                        _LAST5R += 1;
                    if (iPostRank.UREP > jPostRank.UREP)
                        _UREPR += 1;
                    if (iPostRank.PREP > jPostRank.PREP)
                        _PREPR += 1;
                }

                iPostRank.LAST5R = 1.0 * (_LAST5R * 100) / (nElements - 1);
                iPostRank.UREPR = min(iPostRank.UREP, 1.0 * (_UREPR * 100) / (nElements - 1)) * (iPostRank.UREP < 0 ? 2.0 : 1.0);
                iPostRank.PREPR = min(iPostRank.PREP, 1.0 * (_PREPR * 100) / (nElements - 1)) * (iPostRank.PREP < 0 ? 2.0 : 1.0);
            }
            else
            {
                iPostRank.LAST5R = 100;
                iPostRank.UREPR = 100;
                iPostRank.PREPR = 100;
            }

            iPostRank.POSTRF = 0.4 * (0.75 * iPostRank.LAST5R + 0.25 * iPostRank.UREPR) * iPostRank.DREP + 0.6 * iPostRank.PREPR * iPostRank.DPOST;
        }

        // Sort results
        sort(postsRanks.begin(), postsRanks.end(), greater<HierarchicalRecord>());

        // Build result list
        bool found = false;
        int64_t minPostRank = topContentId;
        vector<int64_t> resultIds;
        for(auto iter = postsRanks.begin(); iter < postsRanks.end() && (int)resultIds.size() < countOut; iter++)
        {
            if (iter->Id < minPostRank || minPostRank == 0)
                minPostRank = iter->Id;

            // Find start position
            if (!found && topContentId > 0)
            {
                if (iter->Id == topContentId)
                    found = true;

                continue;
            }

            // Save id for get data
            resultIds.push_back(iter->Id);
        }

        // Get content data
        if (!resultIds.empty())
        {
            auto contents = GetContentsData({}, resultIds, address);
            result.push_backV(contents);
        }

        // ---------------------------------------------
        // If not completed - request historical data
        int lack = countOut - (int)resultIds.size();
        if (lack > 0)
        {
            UniValue histContents = GetHistoricalFeed(lack, minPostRank, topHeight, lang, tagsIncluded, contentTypes,
                txidsExcluded, addrsExcluded, tagsExcluded, address, badReputationLimit);

            result.push_backV(histContents.getValues());
        }

        // Complete!
        return result;
    }

    UniValue WebRpcRepository::GetBoostFeed(int topHeight, int countOut,
        const string& lang, const vector<string>& tags, const vector<int>& contentTypes,
        const vector<string>& txidsExcluded, const vector<string>& addrsExcluded, const vector<string>& tagsExcluded,
        int badReputationLimit)
    {
        UniValue result(UniValue::VARR);

        string sql = R"sql(
            with
                heightMin as ( select ? as value ),
                heightMax as ( select ? as value ),
                lang as ( select ? as value ),
                minReputation as ( select ? as value ),
                boosts as (
                    select
                        tb.rowid boostId,
                        bur.String boostAddress,
                        ctc.Uid contentId,
                        tb.RegId2 contentRowId,
                        (select String from Registry where RowId = tb.RegId2) contentHash,
                        tc.Type as contentType,
                        (
                            (
                                select
                                    sum(io.Value)
                                from
                                    TxInputs i indexed by TxInputs_SpentTxId_Number_TxId
                                cross join TxOutputs io indexed by TxOutputs_TxId_Number_AddressId on
                                    io.TxId = i.TxId and
                                    io.Number = i.Number
                                where
                                    i.SpentTxId = tb.RowId
                            )
                            -
                            (
                                select
                                    sum(o.Value)
                                from
                                    TxOutputs o indexed by TxOutputs_TxId_Number_AddressId
                                where
                                    o.TxId = tb.RowId
                            )
                        ) as boost,
                        json_group_array(tag.Value) as _tags
                    from
                        heightMin,
                        heightMax,
                        lang,
                        minReputation,
                        Transactions tb indexed by Transactions_Type_RegId2_RegId1
                    cross join
                        Registry bur on
                            bur.RowId = tb.RegId1
                    cross join
                        Chain ctb on
                            ctb.TxId = tb.RowId and
                            ctb.Height > heightMin.value and
                            ctb.Height <= heightMax.value
                    cross join
                        Transactions tc indexed by Transactions_Type_RegId2_RegId1 on
                            tc.RegId2 = tb.RegId2 and
                            tc.Type in ( )sql" + join(vector<string>(contentTypes.size(), "?"), ",") + R"sql( )
                    cross join
                        Last ltc on
                            ltc.TxId = tc.RowId
                    cross join
                        Chain ctc on
                            ctc.TxId = tc.RowId
                    left join
                        web.TagsMap tm on
                            tm.ContentId = tc.RowId
                    left join
                        web.Tags tag on
                            tag.Id = tm.TagId and
                            ( ? or tag.Lang = lang.value )
                    cross join
                        Payload p on
                            p.TxId = tc.RowId and
                            ( ? or p.String1 = lang.value )
                    cross join
                        Transactions u indexed by Transactions_Type_RegId1_RegId2_RegId3 on
                            u.Type in (100) and
                            u.RegId1 = tc.RegId1
                    cross join
                        Chain cu on
                            cu.TxId = u.RowId
                    cross join
                        Last lu on
                            lu.TxId = u.RowId
                    left join
                        Ratings ur indexed by Ratings_Type_Uid_Last_Value on
                            ur.Type = 0 and
                            ur.Uid = cu.Uid and
                            ur.Last = 1
                    left join
                        JuryBan jb on
                            jb.AccountId = cu.Uid and
                            jb.Ending > heightMax.value
                    left join
                        Jury j on
                            j.AccountId = cu.Uid
                    left join
                        JuryVerdict jv on
                            jv.FlagRowId = j.FlagRowId
                    where
                        tb.Type in ( 208 )

                        -- Do not show posts from users with low reputation
                        and ifnull(ur.Value, 0) > minReputation.value

                        -- Do not show posts from banned users
                        and jb.AccountId is null

                        -- Do not show posts from users with active jury
                        and jv.FlagRowId is null

                        -- Other excludes
                        and tc.RegId2 not in (
                            select RowId
                            from Registry
                            where String in ( )sql" + join(vector<string>(txidsExcluded.size(), "?"), ",") + R"sql( )
                        )

                        and tc.RegId1 not in (
                            select RowId
                            from Registry
                            where String in ( )sql" + join(vector<string>(addrsExcluded.size(), "?"), ",") + R"sql( )
                        )
                    group by
                        tb.RowId
                    having
                        ( ? or max(case when tag.value in ( )sql" + join(vector<string>(tags.size(), "?"), ",") + R"sql( ) then 1 else 0 end) = 1 ) and
                        ( ? or max(case when tag.value in ( )sql" + join(vector<string>(tagsExcluded.size(), "?"), ",") + R"sql( ) then 1 else 0 end) = 0 )
                )
            select
                b.contentId,
                b.contentHash,
                b.contentType,
                sum(b.boost),
                json_group_array(json_object(b.boostAddress, b.boost)),
                (
                    select
                        json_group_object(ff.reason, ff.cnt)
                    from (
                        select
                            f.Int1 as reason,
                            count() as cnt
                        from
                            Transactions f indexed by Transactions_Type_RegId2_RegId1
                        where
                            f.Type in (410) and
                            f.RegId2 = b.contentRowId
                        group by f.Int1
                    ) ff
                ) as Flags
            from
                boosts b
            group by
                b.contentId
            order by
                sum(b.boost) desc
        )sql";

        SqlTransaction(
            __func__,
            [&]() -> Stmt& {
                return
                    Sql(sql)
                    .Bind(
                        topHeight - countOut,
                        topHeight,
                        lang,
                        -500,
                        contentTypes,
                        lang.empty(),
                        lang.empty(),
                        txidsExcluded,
                        addrsExcluded,
                        tags.empty(),
                        tags,
                        tagsExcluded.empty(),
                        tagsExcluded
                    );
            },
            [&] (Stmt& stmt) {
                stmt.Select([&](Cursor& cursor) {
                    while (cursor.Step())
                    {
                        int64_t contentId, sumBoost, contentType;
                        std::string contentHash, whoBoostedStr, flagsStr;
                        cursor.CollectAll(contentId, contentHash, contentType,
                            sumBoost, whoBoostedStr, flagsStr);

                        UniValue whoBoosted(UniValue::VARR);
                        whoBoosted.read(whoBoostedStr);

                        UniValue boost(UniValue::VOBJ);
                        boost.pushKV("id", contentId);
                        boost.pushKV("txid", contentHash);
                        boost.pushKV("txtype", contentType);
                        boost.pushKV("boost", sumBoost);
                        boost.pushKV("boosted", whoBoosted);

                        UniValue flags(UniValue::VOBJ);
                        flags.read(flagsStr);
                        boost.pushKV("flags", flags);

                        result.push_back(boost);
                    }
                });
            }
        );

        return result;
    }

    // TODO (aok, api): implement
    UniValue WebRpcRepository::GetProfileCollections(const string& addressFeed, int countOut, int pageNumber, const int64_t& topContentId, int topHeight,
                                   const string& lang, const vector<string>& tags, const vector<int>& contentTypes,
                                   const vector<string>& txidsExcluded, const vector<string>& addrsExcluded, const vector<string>& tagsExcluded,
                                   const string& address, const string& keyword, const string& orderby, const string& ascdesc)
    {
        UniValue result(UniValue::VARR);

        if (addressFeed.empty())
            return result;

        // ---------------------------------------------
        string _keyword;
        if(!keyword.empty())
        {
            _keyword = "\"" + keyword + "\"" + " OR " + keyword + "*";
        }

        string contentTypesWhere = " ( 220 ) ";

        string contentIdWhere;
        if (topContentId > 0)
            contentIdWhere = " and t.Id < ? ";

        string accountExistence = " join Transactions ua indexed by Transactions_Type_Last_String1_Height_Id "
                                  " on ua.String1 = t.String1 and ua.Type = 100 and ua.Last = 1 and ua.Height is not null ";

        string langFilter;
        if (!lang.empty())
            langFilter += " join Payload p indexed by Payload_String1_TxHash on p.TxHash = t.Hash and p.String1 = ? ";

        string sorting = "t.Id ";
        sorting += " " + ascdesc;

        string sql = R"sql(
            select t.Id
            from Transactions t indexed by Transactions_Type_Last_String1_Height_Id
            )sql" + accountExistence + R"sql(
            )sql" + langFilter + R"sql(
            where t.Type in )sql" + contentTypesWhere + R"sql(
                and t.Height > 0
                and t.Height <= ?
                and t.Last = 1
                and t.String1 = ?
                )sql" + contentIdWhere   + R"sql(
        )sql";

        sql += R"sql( order by
        )sql" + sorting   + R"sql(
         limit ?
         offset ?
        )sql";

        // ---------------------------------------------

        vector<int64_t> ids;
        SqlTransaction(
            __func__,
            [&]() -> Stmt& {
                auto& stmt = Sql(sql);
                
                if (!lang.empty()) stmt.Bind(lang);
                stmt.Bind(topHeight, addressFeed);
                if (topContentId > 0)
                    stmt.Bind(topContentId);
                stmt.Bind(countOut, pageNumber * countOut);

                return stmt;
            },
            [&] (Stmt& stmt) {
                stmt.Select([&](Cursor& cursor) {
                    while (cursor.Step()) {
                        if (auto[ok, value] = cursor.TryGetColumnInt64(0); ok)
                            ids.push_back(value);
                    }
                });
            }
        );

        // Get content data
        if (!ids.empty())
        {
            auto contents = GetCollectionsData(ids);
            result.push_backV(contents);
        }

        // Complete!
        return result;
    }

    UniValue WebRpcRepository::GetsubsciptionsGroupedByAuthors(const string& address, const string& addressPagination, int nHeight,
        int countOutOfUsers, int countOutOfcontents, int badReputationLimit)
    {
        UniValue result(UniValue::VARR);

        SqlTransaction(
            __func__,
            [&]() -> Stmt& {
                return Sql(R"sql(
                    with
                        addr as (
                            select
                                RowId as id,
                                String as hash
                            from
                                Registry
                            where
                                String = ?
                        ),
                        pageAddr as (
                            select
                                RowId as id,
                                String as hash
                            from
                                Registry
                            where
                                String = ?
                        ),
                        height as (
                            select ? as value
                        )
                    select
                    (select r.String from Registry r where r.RowId = s.RegId1),
                    (
                            select
                                json_group_array(
                                    (select r.String from Registry r where r.RowId = _c.RowId)
                                )
                            from
                                Transactions _c indexed by Transactions_Type_RegId1_RegId2_RegId3
                            cross join
                                Last _lc on
                                    _lc.TxId = _c.RowId
                            cross join
                                Chain _cc indexed by Chain_TxId_Height on
                                    _cc.TxId = _lc.TxId and
                                    _cc.Height <= height.value
                            where
                                _c.Type in (200, 201, 202, 209, 210) and
                                _c.RegId1 = s.RegId1
                            order by
                                _c.RowId desc
                            limit ?
                        )
                    from
                        addr,
                        pageAddr,
                        height
                    cross join
                        Transactions s indexed by Transactions_Type_RegId2_RegId1 on
                            s.Type in (302, 303) and
                            s.RegId2 = addr.id and
                            (? or s.RegId1 > pageAddr.id)
                    cross join
                        Last ls on
                            ls.TxId = s.RowId
                    cross join
                        Chain cs indexed by Chain_TxId_Height on
                            cs.TxId = ls.TxId and
                            cs.Height <= height.value
                    where
                        exists (
                            select 1
                            from
                                Transactions c indexed by Transactions_Type_RegId1_RegId2_RegId3
                            cross join
                                Last lc on
                                    lc.TxId = c.RowId
                            cross join
                                Chain cc indexed by Chain_TxId_Height on
                                    cc.TxId = lc.TxId and
                                    cc.Height <= height.value
                            where
                                c.Type in (200, 201, 202, 209, 210) and
                                c.RegId1 = s.RegId1
                        )
                        -- Do not show posts from users with low reputation
                        -- ifnull(ur.Value,0) > ?
                    order by
                        s.RegId1
                    limit ?
                )sql")
                .Bind(
                    address,
                    addressPagination.empty(),
                    addressPagination,
                    nHeight,
                    countOutOfcontents,
                    countOutOfUsers
                );
            },
            [&] (Stmt& stmt) {
                stmt.Select([&](Cursor& cursor) {
                    while (cursor.Step())
                    {
                        UniValue contents(UniValue::VOBJ);

                        cursor.Collect<string>(0, contents, "address");
                        cursor.Collect<string>(1, contents, "txids");

                        result.push_back(contents);
                    }
                });
            }
        );

        return result;
    }

    // ------------------------------------------------------

    vector<int64_t> WebRpcRepository::GetRandomContentIds(const string& lang, int count, int height)
    {
        vector<int64_t> result;

        SqlTransaction(
            __func__,
            [&]() -> Stmt& {
                return Sql(R"sql(
                    with
                        lang as (
                            select ? as value
                        ),
                        height as (
                            select ? as value
                        )
                    select
                        c.Uid
                    from
                        lang,
                        height
                    cross join
                        Chain c on
                            c.Height > height.value
                    cross join
                        Transactions t on
                            t.RowId = c.TxId and
                            t.Type in (200, 201, 202, 209, 210)
                    cross join
                        Last lt on
                            lt.TxId = t.RowId
                    cross join
                        Payload p on
                            t.RowId = p.TxId and
                            p.String1 = lang.value

                    cross join
                        Transactions u indexed by Transactions_Type_RegId1_RegId2_RegId3 on
                            u.Type = 100 and
                            u.RegId1 = t.RegId1
                    cross join
                        Last lu on
                            lu.TxId = u.RowId
                    cross join
                        Chain cu on
                            cu.TxId = lu.TxId
                    cross join
                        Ratings r indexed by Ratings_Type_Uid_Last_Value on
                            r.Type = 0 and r.Last = 1 and r.Uid = cu.Uid and r.Value > 0
                    order by
                        random()
                    limit ?
                )sql")
                .Bind(lang, height, count);
            },
            [&] (Stmt& stmt) {
                stmt.Select([&](Cursor& cursor) {
                    while (cursor.Step())
                    {
                        if (auto[ok, value] = cursor.TryGetColumnInt64(0); ok)
                            result.push_back(value);
                    }
                });
            }
        );

        return result;
    }

    UniValue WebRpcRepository::GetContentActions(const string& postTxHash)
    {
        UniValue result(UniValue::VOBJ);
        UniValue resultScores(UniValue::VARR);
        UniValue resultBoosts(UniValue::VARR);
        UniValue resultDonations(UniValue::VARR);

        SqlTransaction(
            __func__,
            [&]() -> Stmt& {
                return Sql(R"sql(
                    --scores
                    with
                        tx as (
                            select
                                RowId as id,
                                String as hash
                            from
                                Registry
                            where
                                String = ?
                        )
                    select
                        tx.hash as ContentTxHash,
                        (select r.String from Registry r where r.RowId = s.RegId1) as AddressHash,
                        p.String2 as AccountName,
                        p.String3 as AccountAvatar,
                        r.Value as AccountReputation,
                        s.Int1 as ScoreValue,
                        0 as sumBoost,
                        0 as sumDonation
                    from
                        tx
                    cross join
                        Transactions s indexed by Transactions_Type_RegId2_RegId1 on
                            s.Type in (300) and
                            s.RegId2 = tx.id
                    cross join
                        Chain cs on
                            cs.TxId = s.RowId
                    cross join
                        Transactions u indexed by Transactions_Type_RegId1_RegId2_RegId3 on
                            u.Type in (100) and
                            u.RegId1 = s.RegId1
                    cross join
                        Chain cu on
                            cu.TxId = u.RowId
                    cross join
                        Payload p on
                            p.TxId = u.RowId
                    left join
                        Ratings r indexed by Ratings_Type_Uid_Last_Value on
                            r.Uid = cu.Uid and
                            r.Type = 0 and
                            r.Last = 1

                    --boosts
                    union

                    select
                        b.hash as ContentTxHash,
                        (select r.String from Registry r where r.RowId = b.RegId1) as AddressHash,
                        p.String2 as AccountName,
                        p.String3 as AccountAvatar,
                        r.Value as AccountReputation,
                        0 as ScoreValue,
                        b.sumBoost as sumBoost,
                        0 as sumDonation
                    from
                        (
                            select
                                tx.hash,
                                b.RegId1,
                                b.RegId2,
                                sum(
                                    (
                                        select
                                            sum(io.Value)
                                        from
                                            TxInputs i indexed by TxInputs_SpentTxId_Number_TxId
                                        cross join TxOutputs io indexed by TxOutputs_TxId_Number_AddressId on
                                            io.TxId = i.TxId and
                                            io.Number = i.Number
                                        where
                                            i.SpentTxId = b.RowId
                                    )
                                    -
                                    (
                                        select
                                            sum(o.Value)
                                        from
                                            TxOutputs o indexed by TxOutputs_TxId_Number_AddressId
                                        where
                                            o.TxId = b.RowId
                                    )
                                ) as sumBoost
                            from
                                tx
                            cross join
                                Transactions b indexed by Transactions_Type_RegId2_RegId1 on
                                    b.Type in (208) and
                                    b.RegId2 = tx.id
                            cross join
                                Chain cb on
                                    cb.TxId = b.RowId
                            group by
                                b.RegId1,
                                b.RegId2
                        )b
                    cross join
                        Transactions u indexed by Transactions_Type_RegId1_RegId2_RegId3 on
                            u.Type in (100) and
                            u.RegId1 = b.RegId1
                    cross join
                        Last lu on
                            lu.TxId = u.RowId
                    cross join
                        Chain cu on
                            cu.TxId = u.RowId
                    cross join
                        Payload p on
                            p.TxId = u.RowId
                    left join
                        Ratings r indexed by Ratings_Type_Uid_Last_Value on
                            r.Uid = cu.Uid and
                            r.Type = 0 and
                            r.Last = 1

                    --donations
                    union

                    select
                        td.hash as ContentTxHash,
                        (select r.String from Registry r where r.RowId = td.RegId1) as AddressHash,
                        p.String2 as AccountName,
                        p.String3 as AccountAvatar,
                        r.Value as AccountReputation,
                        0 as ScoreValue,
                        0 as sumBoost,
                        td.sumDonation as sumDonation
                    from
                        (
                            select
                                tx.hash,
                                c.RegId1,
                                c.RegId3,
                                sum(o.Value) as sumDonation
                            from
                                tx
                            cross join
                                Transactions c indexed by Transactions_Type_RegId3_RegId1 on
                                    c.Type in (204, 205, 206) and
                                    c.RegId3 = tx.id
                            cross join
                                Last lc on
                                    lc.TxId = c.RowId
                            cross join
                                Transactions p indexed by Transactions_Type_RegId2_RegId1 on
                                    p.Type in (200,201,202,209,210) and
                                    p.RegId2 = c.RegId3
                            cross join
                                Last lp on
                                    lp.TxId = p.RowId
                            cross join
                                TxOutputs o indexed by TxOutputs_AddressId_TxIdDesc_Number on
                                    o.AddressId = p.RegId1 and
                                    o.AddressId != c.RegId1 and
                                    o.TxId = c.RowId and
                                    o.Value > 0
                            group by
                                c.RegId1,
                                c.RegId3
                        )td
                    cross join
                        Transactions u indexed by Transactions_Type_RegId1_RegId2_RegId3 on
                            u.Type in (100) and
                            u.RegId1 = td.RegId1
                    cross join
                        Last lu on
                            lu.TxId = u.RowId
                    cross join
                        Chain cu on
                            cu.TxId = u.RowId
                    cross join
                        Payload p on
                            p.TxId = u.RowId
                    left join
                        Ratings r indexed by Ratings_Type_Uid_Last_Value on
                            r.Uid = cu.Uid and
                            r.Type = 0 and
                            r.Last = 1
                )sql")
                .Bind(postTxHash);
            },
            [&] (Stmt& stmt) {
                stmt.Select([&](Cursor& cursor) {
                    while (cursor.Step())
                    {
                        UniValue record(UniValue::VOBJ);
                        if (auto[ok, value] = cursor.TryGetColumnString(0); ok) record.pushKV("posttxid", value);
                        if (auto[ok, value] = cursor.TryGetColumnString(1); ok) record.pushKV("address", value);
                        if (auto[ok, value] = cursor.TryGetColumnString(2); ok) record.pushKV("name", value);
                        if (auto[ok, value] = cursor.TryGetColumnString(3); ok) record.pushKV("avatar", value);
                        if (auto[ok, value] = cursor.TryGetColumnInt(4); ok) record.pushKV("reputation", value);
                        if (auto[ok, value] = cursor.TryGetColumnInt(5); ok && value > 0) {
                            record.pushKV("value", value);
                            resultScores.push_back(record);
                        }
                        if (auto[ok, value] = cursor.TryGetColumnInt(6); ok && value > 0) {
                            record.pushKV("value", value);
                            resultBoosts.push_back(record);
                        }
                        if (auto[ok, value] = cursor.TryGetColumnInt(7); ok && value > 0) {
                            record.pushKV("value", value);
                            resultDonations.push_back(record);
                        }
                    }
                });
            }
        );

        result.pushKV("scores",resultScores);
        result.pushKV("boosts",resultBoosts);
        result.pushKV("donations",resultDonations);

        return result;
    };

}
