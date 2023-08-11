// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include "pocketdb/repositories/web/WebRpcRepository.h"
#include "pocketdb/helpers/ShortFormRepositoryHelper.h"
#include <functional>

namespace PocketDb
{
    UniValue WebRpcRepository::GetAddressId(const string& address)
    {
        UniValue result(UniValue::VOBJ);

        SqlTransaction(__func__, [&]()
        {
            Sql(R"sql(
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
            .Bind(address)
            .Select([&](Cursor& cursor) {
                if (cursor.Step())
                {
                    if (auto[ok, value] = cursor.TryGetColumnString(0); ok) result.pushKV("address", value);
                    if (auto[ok, value] = cursor.TryGetColumnInt64(1); ok) result.pushKV("id", value);
                }
            });
        });

        return result;
    }

    UniValue WebRpcRepository::GetAddressId(int64_t id)
    {
        UniValue result(UniValue::VOBJ);

        SqlTransaction(__func__, [&]()
        {
            Sql(R"sql(
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
            .Bind(id)
            .Select([&](Cursor& cursor) {
                if (cursor.Step())
                {
                    if (auto[ok, value] = cursor.TryGetColumnString(0); ok) result.pushKV("address", value);
                    if (auto[ok, value] = cursor.TryGetColumnInt64(1); ok) result.pushKV("id", value);
                }
            });
        });

        return result;
    }

    UniValue WebRpcRepository::GetUserAddress(const string& name)
    {
        UniValue result(UniValue::VARR);
        
        if (name.empty()) return result;

        auto _name = EscapeValue(name);

        SqlTransaction(__func__, [&]()
        {
            Sql(R"sql(
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
            .Bind(_name)
            .Select([&](Cursor& cursor) {
                while (cursor.Step())
                {
                    UniValue record(UniValue::VOBJ);

                    cursor.Collect<string>(0, record, "name");
                    cursor.Collect<string>(1, record, "address");

                    result.push_back(record);
                }
            });
        });

        return result;
    }

    UniValue WebRpcRepository::GetAddressesRegistrationDates(const vector<string>& addresses)
    {
        auto result = UniValue(UniValue::VARR);

        if (addresses.empty())
            return result;

        SqlTransaction(__func__, [&]()
        {
            Sql(R"sql(
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
                    (select r.String from Registry r where r.RowId = u.HashId)
                from
                    addr
                cross join
                    Transactions u indexed by Transactions_Type_RegId1_RegId2_RegId3
                        on u.Type in (100) and u.RegId1 = addr.id
                cross join
                    First f
                        on f.TxId = u.RowId
            )sql")
            .Bind(addresses)
            .Select([&](Cursor& cursor) {
                while (cursor.Step())
                {
                    UniValue record(UniValue::VOBJ);

                    cursor.Collect<string>(0, record, "address");
                    cursor.Collect<int64_t>(1, record, "time");
                    cursor.Collect<string>(2, record, "txid");

                    result.push_back(record);
                }
            });
        });

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

        SqlTransaction(__func__, [&]()
        {
            Sql(sql)
            .Bind(count)
            .Select([&](Cursor& cursor) {
                while (cursor.Step())
                {
                    UniValue addr(UniValue::VOBJ);

                    cursor.Collect<string>(0, addr, "addresses");
                    cursor.Collect<int64_t>(1, addr, "balance");

                    result.push_back(addr);
                }
            });
        });

        return result;
    }

    UniValue WebRpcRepository::GetAccountState(const string& address, int heightWindow)
    {
        UniValue result(UniValue::VOBJ);

        SqlTransaction(__func__, [&]()
        {
            Sql(R"sql(
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
                        where p.Type in (200) and p.RegId1 = u.RegId1 and (c.Height >= height.val or c.Height isnull)
                    ) as PostSpent,
                    (
                        select count()
                        from Transactions p indexed by Transactions_Type_RegId1_RegId2_RegId3
                        left join Chain c indexed by Chain_TxId_Height on c.TxId = p.RowId
                        where p.Type in (201) and p.RegId1 = u.RegId1 and (c.Height >= height.val or c.Height isnull)
                    ) as VideoSpent,
                    (
                        select count()
                        from Transactions p indexed by Transactions_Type_RegId1_RegId2_RegId3
                        left join Chain c indexed by Chain_TxId_Height on c.TxId = p.RowId
                        where p.Type in (202) and p.RegId1 = u.RegId1 and (c.Height >= height.val or c.Height isnull)
                    ) as ArticleSpent,
                    (
                        select count()
                        from Transactions p indexed by Transactions_Type_RegId1_RegId2_RegId3
                        left join Chain c indexed by Chain_TxId_Height on c.TxId = p.RowId
                        where p.Type in (209) and p.RegId1 = u.RegId1 and (c.Height >= height.val or c.Height isnull)
                    ) as StreamSpent,
                    (
                        select count()
                        from Transactions p indexed by Transactions_Type_RegId1_RegId2_RegId3
                        left join Chain c indexed by Chain_TxId_Height on c.TxId = p.RowId
                        where p.Type in (210) and p.RegId1 = u.RegId1 and (c.Height >= height.val or c.Height isnull)
                    ) as AudioSpent,
                    (
                        select count()
                        from Transactions p indexed by Transactions_Type_RegId1_RegId2_RegId3
                        left join Chain c indexed by Chain_TxId_Height on c.TxId = p.RowId
                        where p.Type in (204) and p.RegId1 = u.RegId1 and (c.Height >= height.val or c.Height isnull)
                    ) as CommentSpent,
                    (
                        select count()
                        from Transactions p indexed by Transactions_Type_RegId1_RegId2_RegId3
                        left join Chain c indexed by Chain_TxId_Height on c.TxId = p.RowId
                        where p.Type in (300) and p.RegId1 = u.RegId1 and (c.Height >= height.val or c.Height isnull)
                    ) as ScoreSpent,
                    (
                        select count()
                        from Transactions p indexed by Transactions_Type_RegId1_RegId2_RegId3
                        left join Chain c indexed by Chain_TxId_Height on c.TxId = p.RowId
                        where p.Type in (301) and p.RegId1 = u.RegId1 and (c.Height >= height.val or c.Height isnull)
                    ) as ScoreCommentSpent,
                    (
                        select count()
                        from Transactions p indexed by Transactions_Type_RegId1_RegId2_RegId3
                        left join Chain c indexed by Chain_TxId_Height on c.TxId = p.RowId
                        where p.Type in (307) and p.RegId1 = u.RegId1 and (c.Height >= height.val or c.Height isnull)
                    ) as ComplainSpent,
                    (
                        select count()
                        from Transactions p indexed by Transactions_Type_RegId1_RegId2_RegId3
                        left join Chain c indexed by Chain_TxId_Height on c.TxId = p.RowId
                        where p.Type in (410) and p.RegId1 = u.RegId1 and (c.Height >= height.val or c.Height isnull)
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
            .Bind(heightWindow, address)
            .Select([&](Cursor& cursor) {
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
        });

        return result;
    }

    UniValue WebRpcRepository::GetAccountSetting(const string& address)
    {
        string result;

        SqlTransaction(__func__, [&]()
        {
            Sql(R"sql(
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
            .Bind(address)
            .Select([&](Cursor& cursor) {
                while (cursor.Step())
                {
                    if (auto[ok, value] = cursor.TryGetColumnString(0); ok)
                        result = value;
                }
            });
        });

        return result;
    }

    UniValue WebRpcRepository::GetUserStatistic(const vector<string>& addresses, const int nHeight, const int depthR, const int depthC, const int cntC)
    {
        UniValue result(UniValue::VARR);
        if (addresses.empty())
            return  result;

        SqlTransaction(__func__, [&]()
        {
            Sql(R"sql(
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
                        join First f on
                            f.TxId = ref.RowId
                        join Chain c on
                            c.Height <= ? and c.Height > ?
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
                            join Last lp on
                                lp.TxId = p.RowId
                            join Transactions c indexed by Transactions_Type_RegId3_RegId1 on
                                c.Type in (204) and c.RegId3 = p.RegId2 and c.RegId1 != addr.id
                            join First fc on
                                fc.TxId = c.RowId
                            join Chain cc indexed by Chain_TxId_Height on
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
            .Bind(addresses, nHeight, nHeight - depthR, nHeight, nHeight - depthC, cntC)
            .Select([&](Cursor& cursor) {
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
        });

        return result;
    }

    // TODO (aok, api): firstFlagsDepth disabled for optimization
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
        
        string with;
        if (!addresses.empty())
        {
            with = R"sql(
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
            )sql";
        }

        if (!ids.empty())
        {
            with = R"sql(
                with
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

        string fullProfileSql = "";
        if (!shortForm)
        {
            fullProfileSql = R"sql(

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
                    from BlockingLists bl indexed by BlockingLists_IdSource_IdTarget
                    cross join Chain cbl indexed by Chain_Uid_Height
                        on cbl.Uid = bl.IdTarget
                    cross join Transactions ub
                        on ub.RowId = cbl.TxId and ub.Type = 100
                    cross join Last lub
                        on lub.TxId = ub.RowId
                    cross join Registry rub
                        on rub.RowId = ub.RegId1
                    where
                        bl.IdSource = cu.Uid
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

        string sql = R"sql(
            )sql" + with + R"sql(
            select

                (select r.String from Registry r where r.RowId = u.HashId) as AccountHash
                ,(select r.String from Registry r where r.RowId = u.RegId1) as Addres
                ,cu.Uid
                ,u.Type
                ,ifnull(p.String2,'') as Name
                ,ifnull(p.String3,'') as Avatar
                ,ifnull(p.String7,'') as Donations
                ,ifnull((select r.String from Registry r where r.RowId = u.RegId2),'') as Referrer

                ,ifnull((
                    select
                        count()
                    from
                        Transactions po indexed by Transactions_Type_RegId1_RegId2_RegId3
                    cross join
                        Last lpo
                            on lpo.TxId = po.RowId
                    where
                        po.Type in (200,201,202,209,210) and po.RegId1 = addr.id
                ), 0) as PostsCount

                ,ifnull((
                    select
                        count()
                    from
                        Transactions po indexed by Transactions_Type_RegId1_RegId2_RegId3
                    cross join
                        Last lpo
                            on lpo.TxId = po.RowId
                    where
                        po.Type in (207) and
                        po.RegId1 = addr.id
                ), 0) as DelCount

                ,ifnull((
                    select
                        r.Value
                    from
                        Ratings r indexed by Ratings_Type_Uid_Last_Value
                    where
                        r.Type = 0 and
                        r.Uid = cu.Uid and
                        r.Last = 1
                ), 0) as Reputation

                ,(
                    select
                        count()
                    from
                        Transactions subs indexed by Transactions_Type_RegId1_RegId2_RegId3
                    cross join
                        Last lsubs
                            on lsubs.TxId = subs.RowId
                    cross join
                        Transactions uas indexed by Transactions_Type_RegId1_RegId2_RegId3
                            on uas.Type in (100) and uas.RegId1 = subs.RegId2
                    cross join
                        Last luas
                            on luas.TxId = uas.RowId
                    where
                        subs.Type in (302, 303) and
                        subs.RegId1 = addr.id
                ) as SubscribesCount

                ,(
                    select
                        count()
                    from
                        Transactions subs indexed by Transactions_Type_RegId2_RegId1
                    cross join
                        Last lsubs
                            on lsubs.TxId = subs.RowId
                    cross join
                        Transactions uas indexed by Transactions_Type_RegId1_RegId2_RegId3
                            on uas.Type in (100) and uas.RegId1 = subs.RegId1
                    cross join
                        Last luas
                            on luas.TxId = uas.RowId
                    where
                        subs.Type in (302, 303) and
                        subs.RegId2 = addr.id
                ) as SubscribersCount

                ,(
                    select
                        count()
                    from
                        BlockingLists bl
                    where
                        bl.IdSource = cu.Uid
                ) as BlockingsCount

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

                ,(
                    select
                        reg.Time
                    from Transactions reg indexed by Transactions_Type_RegId1_RegId2_RegId3
                    cross join First freg
                        on freg.TxId = reg.RowId
                    where
                        reg.Type in (100) and
                        reg.RegId1 = addr.id
                ) as RegistrationDate

                ,(
                    select
                        json_group_object(gr.Type, gr.Cnt)
                    from
                        (
                            select
                                (f.Int1)Type,
                                (count())Cnt
                            from
                                Transactions f indexed by Transactions_Type_RegId3_RegId1
                            cross join
                                Chain c
                                    on c.TxId = f.RowId
                            where
                                f.Type in (410) and
                                f.RegId3 = addr.id
                            group by
                                f.Int1
                        )gr
                ) as FlagsJson

                ,(
                    select 0
                --    select
                --        json_group_object(gr.Type, gr.Cnt)
                --    from
                --        (
                --            select
                --                (f.Int1)Type,
                --                (count())Cnt
                --            from
                --                Transactions f indexed by Transactions_Type_RegId3_RegId1
                --            cross join (
                --                select
                --                    min(cfp.Height) as minHeight
                --                from
                --                    Transactions fp indexed by Transactions_Type_RegId1_RegId2_RegId3
                --                cross join
                --                    First ffp
                --                        on ffp.TxId = fp.RowId
                --                cross join
                --                    Chain cfp indexed by Chain_TxId_Height
                --                        on cfp.TxId = fp.RowId
                --                where
                --                    fp.Type in (200, 201, 202, 209, 210) and
                --                    fp.RegId1 = addr.id
                --            )fp
                --            cross join
                --                Chain cf indexed by Chain_TxId_Height
                --                    on cf.TxId = f.RowId and cf.Height >= fp.minHeight and cf.Height <= (fp.minHeight + ?)
                --            where
                --                f.Type in (410) and
                --                f.RegId3 = addr.id
                --            group by
                --                f.Int1
                --        )gr
                ) as FirstFlagsCount

                )sql" + fullProfileSql + R"sql(

            from addr
            join Transactions u indexed by Transactions_Type_RegId1_RegId2_RegId3
                on u.Type in (100, 170) and u.RegId1 = addr.id
            join Last lu
                on lu.TxId = u.RowId
            join Chain cu
                on cu.TxId = u.RowId
            left join Payload p
                on p.TxId = u.RowId
        )sql";

        SqlTransaction(__func__, [&]()
        {
            Sql(sql)
            .Bind(addresses, ids) // firstFlagsDepth * 1440
            .Select([&](Cursor& cursor) {
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
                                UniValue subscribes(UniValue::VARR);
                                subscribes.read(value);
                                record.pushKV("blocking", subscribes);
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
        });

        return result;
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

        SqlTransaction(__func__, [&]()
        {
            Sql(R"sql(
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
                    ifnull((select (select r.String from Registry r where r.RowId = tt.RegId1) from Transactions tt where tt.HashId = t.RegId4), '') as AddressCommentParent,
                    ifnull((select (select r.String from Registry r where r.RowId = tt.RegId1) from Transactions tt where tt.HashId = t.RegId5), '') as AddressCommentAnswer,

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

                    (case when t.HashId != t.RegId2 then 1 else 0 end) as CommentEdit,

                    (
                        select o.Value
                        from TxOutputs o indexed by TxOutputs_AddressId_TxId_Number
                        where o.TxId = t.RowId and o.AddressId = p.RegId1 and o.AddressId != t.RegId1
                    ) as Donate

                from
                    Chain c

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

                cross join
                    Transactions ua indexed by Transactions_Type_RegId1_RegId2_RegId3
                        on ua.Type = 100 and ua.RegId1 = t.RegId1
                cross join
                    Last lua
                        on lua.TxId = ua.RowId

                cross join Transactions p indexed by Transactions_Type_RegId2_RegId1
                on p.Type in (200, 201, 202, 209, 210) and p.RegId2 = t.RegId3
                cross join
                    Last lp
                        on lp.TxId = p.RowId
                cross join
                    Chain cp
                        on cp.TxId = p.RowId
                cross join
                    Payload pp
                        on pp.TxId = p.RowId and pp.String1 = ?

                left join
                    BlockingLists bl
                        on bl.IdSource = cp.Uid and bl.IdTarget = c.Uid

                where
                    c.Height > (? - 600) and
                    bl.ROWID is null

                order by c.Height desc
                limit ?
            )sql")
            .Bind(lang, height, count)
            .Select([&](Cursor& cursor) {
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
                    if (auto[ok, value] = cursor.TryGetColumnString(i++); ok) record.pushKV("scoreUp", value);
                    if (auto[ok, value] = cursor.TryGetColumnString(i++); ok) record.pushKV("scoreDown", value);
                    if (auto[ok, value] = cursor.TryGetColumnString(i++); ok) record.pushKV("reputation", value);
                    if (auto[ok, value] = cursor.TryGetColumnInt(i++); ok) record.pushKV("edit", value == 1);
                    if (auto[ok, value] = cursor.TryGetColumnString(i++); ok)
                    {
                        record.pushKV("donation", "true");
                        record.pushKV("amount", value);
                    }

                    result.push_back(record);
                }
            });
        });

        return result;
    }

    map<int64_t, UniValue> WebRpcRepository::GetLastComments(const vector<int64_t>& ids, const string& address)
    {
        auto func = __func__;
        map<int64_t, UniValue> result;

        string sql = R"sql(
            select
                cmnt.contentId,
                cc.Uid                                                                      as commentId,
                c.Type,
                (select r.String from Registry r where r.RowId = c.RegId2)                  as RootTxHash,
                (select r.String from Registry r where r.RowId = c.RegId3)                  as PostTxHash,
                (select r.String from Registry r where r.RowId = c.RegId1)                  as AddressHash,
                (select corig.Time from Transactions corig where corig.HashId = c.RegId2)   as Time,
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

                ifnull((select scr.Int1
                        from Transactions scr indexed by Transactions_Type_RegId1_RegId2_RegId3
                        join Chain cscr on cscr.TxId = scr.RowId
                        where scr.Type = 301
                            and scr.RegId1 = (select RowId from Registry where String = ?)
                            and scr.RegId2 = c.RegId2),0)                                   as MyScore,

                (
                    select o.Value
                    from TxOutputs o
                    where o.TxId = c.RowId
                        and o.AddressId = cmnt.ContentAddressId
                        and o.AddressId != c.RegId1
                )                                                                           as Donate

            from (
                select

                    c.Uid    as contentId,
                    t.RegId1 as ContentAddressId,

                    -- Last comment for content record
                    (
                        select c1.RowId

                        from Transactions c1 indexed by Transactions_Type_RegId3_RegId1
                        join Chain cc1 on cc1.TxId = c1.RowId
                        join Last lc1 on lc1.TxId = c1.RowId

                        join Transactions uac indexed by Transactions_Type_RegId1_RegId2_RegId3
                          on uac.Type = 100 and uac.RegId1 = c1.RegId1
                        join Chain cuac on cuac.TxId = uac.RowId
                        join Last luac on luac.TxId = uac.RowId

                        left join TxOutputs o
                            on o.TxId = c1.RowId and o.AddressId = t.RegId1 and o.AddressId != c1.RegId1 and o.Value > ?

                        where c1.Type in (204, 205)
                          and c1.RegId3 = t.RegId2
                          and c1.RegId4 is null

                          -- exclude commenters blocked by the author of the post
                          and not exists (
                            select 1
                            from BlockingLists bl
                            where bl.IdSource = t.RegId1 and bl.IdTarget = c1.RegId1
                          )

                        order by o.Value desc, c1.RowId desc
                        limit 1
                    )commentRowId

                from Chain c indexed by Chain_Uid_Height
                join Transactions t on t.RowId = c.TxId
                join Last l on l.TxId = t.RowId
                join Transactions ua indexed by Transactions_Type_RegId1_RegId2_RegId3
                  on ua.RegId1 = t.RegId1 and ua.Type = 100
                join Chain cua on cua.TxId = ua.RowId
                join Last lua on lua.TxId = ua.RowId

                where c.Uid in ( )sql" + join(vector<string>(ids.size(), "?"), ",") + R"sql( )

            ) cmnt

            join Transactions c on c.RowId = cmnt.commentRowId
            join Chain cc on cc.TxId = c.RowId
        )sql";

        SqlTransaction(func, [&]()
        {
            Sql(sql)
            .Bind(address, (int64_t)(0.5 * COIN), ids)
            .Select([&](Cursor& cursor) {
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
                    if (auto[ok, value] = cursor.TryGetColumnInt(16); ok) record.pushKV("myScore", value);
                    if (auto[ok, value] = cursor.TryGetColumnString(17); ok)
                    {
                        record.pushKV("donation", "true");
                        record.pushKV("amount", value);
                    }
                                    
                    result.emplace(contentId, record);
                }
            });
        });

        return result;
    }

    UniValue WebRpcRepository::GetCommentsByPost(const string& postHash, const string& parentHash, const string& addressHash)
    {
        auto func = __func__;
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
                (select r.String from Registry r where r.RowId = c.HashId),
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
                sc.Int1 as MyScore,
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
                    left join
                        BlockingLists bls
                            on bls.IdSource = ct.Uid and bls.IdTarget = cs.Uid
                    where
                        s.Type in (204, 205) and
                        s.RegId4 = c.RegId2 and
                        bls.ROWID is null
                ) AS ChildrenCount,
                o.Value as Donate
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
                Transactions r indexed by Transactions_HashId
                    on r.HashId = c.RegId2
            cross join
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
            left join Transactions sc indexed by Transactions_Type_RegId2_RegId1
                on sc.Type in (301) and sc.RegId2 = c.RegId2 and sc.RegId1 = addr.id and exists (select 1 from Chain csc where csc.TxId = sc.RowId)
            left join TxOutputs o indexed by TxOutputs_AddressId_TxId_Number
                on o.TxId = r.RowId and o.AddressId = t.RegId1 and o.AddressId != c.RegId1
            left join
                BlockingLists bl
                    on bl.IdSource = ct.Uid and bl.IdTarget = cc.Uid
            where
                bl.ROWID is null
                )sql" + parentWhere + R"sql(
        )sql";

        SqlTransaction(func, [&]()
        {
            auto& stmt = Sql(sql);
            stmt.Bind(postHash, addressHash);
            if (!parentHash.empty())
                stmt.Bind(parentHash);

            stmt.Select([&](Cursor& cursor) {
                while (cursor.Step())
                {
                    UniValue record(UniValue::VOBJ);

                    //auto[ok0, txHash] = cursor.TryGetColumnString(cursor, 1);
                    auto[ok1, rootTxHash] = cursor.TryGetColumnString(2);
                    record.pushKV("id", rootTxHash);

                    if (auto[ok, value] = cursor.TryGetColumnString(3); ok)
                        record.pushKV("postid", value);

                    if (auto[ok, value] = cursor.TryGetColumnString(4); ok) record.pushKV("address", value);
                    if (auto[ok, value] = cursor.TryGetColumnString(5); ok) record.pushKV("time", value);
                    if (auto[ok, value] = cursor.TryGetColumnString(6); ok) record.pushKV("timeUpd", value);
                    if (auto[ok, value] = cursor.TryGetColumnString(7); ok) record.pushKV("block", value);
                    if (auto[ok, value] = cursor.TryGetColumnString(8); ok) record.pushKV("msg", value);
                    if (auto[ok, value] = cursor.TryGetColumnString(9); ok) record.pushKV("parentid", value);
                    if (auto[ok, value] = cursor.TryGetColumnString(10); ok) record.pushKV("answerid", value);
                    if (auto[ok, value] = cursor.TryGetColumnString(11); ok) record.pushKV("scoreUp", value);
                    if (auto[ok, value] = cursor.TryGetColumnString(12); ok) record.pushKV("scoreDown", value);
                    if (auto[ok, value] = cursor.TryGetColumnString(13); ok) record.pushKV("reputation", value);
                    if (auto[ok, value] = cursor.TryGetColumnString(14); ok) record.pushKV("myScore", value);
                    if (auto[ok, value] = cursor.TryGetColumnString(15); ok) record.pushKV("children", value);

                    if (auto[ok, value] = cursor.TryGetColumnString(16); ok)
                    {
                        record.pushKV("amount", value);
                        record.pushKV("donation", "true");
                    }

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
        });

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
                            on r.RowId = t.RegId1
                    where
                        c.Uid in ( )sql" + join(vector<string>(cmntIds.size(), "?"), ",") + R"sql( )
                )
            )sql";
        }

        SqlTransaction(__func__, [&]()
        {
            Sql(R"sql(
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
                    (select r.String from Registry r where r.RowId = c.HashId),
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
                    sc.Int1 as MyScore,
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
                        left join
                            BlockingLists bls
                                on bls.IdSource = ct.Uid and bls.IdTarget = cs.Uid
                        where
                            s.Type in (204, 205) and
                            s.RegId4 = c.RegId2 and
                            bls.ROWID is null
                    ) AS ChildrenCount,
                    o.Value as Donate,
                    (select 1 from BlockingLists bl where bl.IdSource = ct.Uid and bl.IdTarget = cc.Uid)Blocked,
                    cc.Uid
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
                    Transactions r indexed by Transactions_HashId
                        on r.HashId = c.RegId2
                cross join
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
                left join TxOutputs o indexed by TxOutputs_AddressId_TxId_Number
                    on o.TxId = r.RowId and o.AddressId = t.RegId1 and o.AddressId != c.RegId1
            )sql")
            .Bind(addressHash, cmntHashes, cmntIds)
            .Select([&](Cursor& cursor) {
                while (cursor.Step())
                {
                    UniValue record(UniValue::VOBJ);

                    if (auto[ok, value] = cursor.TryGetColumnInt(0); ok) record.pushKV("type", value);
                    if (auto[ok, value] = cursor.TryGetColumnString(2); ok) record.pushKV("id", value);
                    if (auto[ok, value] = cursor.TryGetColumnString(3); ok) record.pushKV("postid", value);
                    if (auto[ok, value] = cursor.TryGetColumnString(4); ok) record.pushKV("address", value);
                    if (auto[ok, value] = cursor.TryGetColumnString(5); ok) record.pushKV("time", value);
                    if (auto[ok, value] = cursor.TryGetColumnString(6); ok) record.pushKV("timeUpd", value);
                    if (auto[ok, value] = cursor.TryGetColumnString(7); ok) record.pushKV("block", value);
                    if (auto[ok, value] = cursor.TryGetColumnString(8); ok) record.pushKV("msg", value);
                    if (auto[ok, value] = cursor.TryGetColumnString(9); ok) record.pushKV("parentid", value);
                    if (auto[ok, value] = cursor.TryGetColumnString(10); ok) record.pushKV("answerid", value);
                    if (auto[ok, value] = cursor.TryGetColumnString(11); ok) record.pushKV("scoreUp", value);
                    if (auto[ok, value] = cursor.TryGetColumnString(12); ok) record.pushKV("scoreDown", value);
                    if (auto[ok, value] = cursor.TryGetColumnString(13); ok) record.pushKV("reputation", value);
                    if (auto[ok, value] = cursor.TryGetColumnString(14); ok) record.pushKV("myScore", value);
                    if (auto[ok, value] = cursor.TryGetColumnString(15); ok) record.pushKV("children", value);

                    if (auto[ok, value] = cursor.TryGetColumnString(16); ok)
                    {
                        record.pushKV("amount", value);
                        record.pushKV("donation", "true");
                    }

                    if (auto[ok, value] = cursor.TryGetColumnInt(17); ok && value > 0)
                        record.pushKV("blck", 1);

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

                    auto[ok_hash, hash] = cursor.TryGetColumnString(1);
                    auto[ok_id, id] = cursor.TryGetColumnInt64(18);
                    result.emplace_back(hash, id, record);
                }
            });
        });

        return result;
    }

    UniValue WebRpcRepository::GetPagesScores(const vector<string>& postHashes, const vector<string>& commentHashes, const string& addressHash)
    {
        UniValue result(UniValue::VARR);

        if (!postHashes.empty())
        {
            SqlTransaction(__func__, [&]()
            {
                Sql(R"sql(
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
                        sc.Int1 as MyScoreValue

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
                .Bind(addressHash, postHashes)
                .Select([&](Cursor& cursor) {
                    while (cursor.Step())
                    {
                        UniValue record(UniValue::VOBJ);

                        if (auto[ok, value] = cursor.TryGetColumnString(0); ok) record.pushKV("posttxid", value);
                        if (auto[ok, value] = cursor.TryGetColumnString(1); ok) record.pushKV("value", value);

                        result.push_back(record);
                    }
                });
            });
        }

        if (!commentHashes.empty())
        {
            SqlTransaction(__func__, [&]()
            {
                Sql(R"sql(
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
                            where sc.Type in (301) and sc.RegId2 = c.HashId and sc.Int1 = 1
                        ) as ScoreUp,
                        (
                            select count()
                            from Transactions sc indexed by Transactions_Type_RegId2_RegId1
                            cross join Chain csc on csc.TxId = sc.RowId
                            where sc.Type in (301) and sc.RegId2 = c.HashId and sc.Int1 = -1
                        ) as ScoreDown,
                        (
                            select r.Value
                            from Ratings r indexed by Ratings_Type_Uid_Last_Value
                            where r.Uid = cc.Uid and r.Type = 3 and r.Last = 1
                        ) as Reputation,
                        msc.Int1 AS MyScore
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
                .Bind(addressHash, commentHashes)
                .Select([&](Cursor& cursor) {
                    while (cursor.Step())
                    {
                        UniValue record(UniValue::VOBJ);

                        if (auto[ok, value] = cursor.TryGetColumnString(0); ok) record.pushKV("cmntid", value);
                        if (auto[ok, value] = cursor.TryGetColumnString(1); ok) record.pushKV("scoreUp", value);
                        if (auto[ok, value] = cursor.TryGetColumnString(2); ok) record.pushKV("scoreDown", value);
                        if (auto[ok, value] = cursor.TryGetColumnString(3); ok) record.pushKV("reputation", value);
                        if (auto[ok, value] = cursor.TryGetColumnString(4); ok) record.pushKV("myScore", value);

                        result.push_back(record);
                    }
                });
            });
        }

        return result;
    }

    UniValue WebRpcRepository::GetPostScores(const string& postTxHash)
    {
        UniValue result(UniValue::VARR);

        SqlTransaction(__func__, [&]()
        {
            Sql(R"sql(
                with
                tx as (
                    select
                        r.RowId as id,
                        r.String as hash
                    from
                        Registry r
                    where
                        r.String = ?
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
                        on u.Type in (100) and u.RegId1 = s.RegId1
                cross join
                    Last lu
                        on lu.TxId = u.RowId
                cross join
                    Chain cu
                        on cu.TxId = u.RowId
                left join
                    Ratings r indexed by Ratings_Type_Uid_Last_Value
                        on r.Type = 0 and r.Last = 1 and r.Uid = cu.Uid
                cross join
                    Payload p
                        on p.TxId = u.RowId
            )sql")
            .Bind(postTxHash)
            .Select([&](Cursor& cursor) {
                while (cursor.Step())
                {
                    UniValue record(UniValue::VOBJ);

                    if (auto[ok, value] = cursor.TryGetColumnString(0); ok) record.pushKV("posttxid", value);
                    if (auto[ok, value] = cursor.TryGetColumnString(1); ok) record.pushKV("address", value);
                    if (auto[ok, value] = cursor.TryGetColumnString(2); ok) record.pushKV("name", value);
                    if (auto[ok, value] = cursor.TryGetColumnString(3); ok) record.pushKV("avatar", value);
                    if (auto[ok, value] = cursor.TryGetColumnString(4); ok) record.pushKV("reputation", value);
                    if (auto[ok, value] = cursor.TryGetColumnString(5); ok) record.pushKV("value", value);

                    result.push_back(record);
                }
            });
        });

        return result;
    }

    UniValue WebRpcRepository::GetAddressScores(const vector<string>& postHashes, const string& address)
    {
        UniValue result(UniValue::VARR);

        string postWhere;
        if (!postHashes.empty())
            postWhere += " and s.String2 in ( " + join(vector<string>(postHashes.size(), "?"), ",") + " ) ";

        SqlTransaction(__func__, [&]()
        {
            Sql(R"sql(
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
                    (select r.String from Registry r where r.RowId = s.RegId2) as posttxid,
                    addr.hash as address,
                    up.String2 as name,
                    up.String3 as avatar,
                    ur.Value as reputation,
                    s.Int1 as value
                from
                    addr
                cross join
                    Transactions s indexed by Transactions_Type_RegId1_RegId2_RegId3
                        on s.Type in (300) and s.RegId1 = addr.id )sql" + postWhere + R"sql(
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
            .Bind(address, postHashes)
            .Select([&](Cursor& cursor) {
                while (cursor.Step())
                {
                    UniValue record(UniValue::VOBJ);

                    if (auto[ok, value] = cursor.TryGetColumnString(0); ok) record.pushKV("posttxid", value);
                    if (auto[ok, value] = cursor.TryGetColumnString(1); ok) record.pushKV("address", value);
                    if (auto[ok, value] = cursor.TryGetColumnString(2); ok) record.pushKV("name", value);
                    if (auto[ok, value] = cursor.TryGetColumnString(3); ok) record.pushKV("avatar", value);
                    if (auto[ok, value] = cursor.TryGetColumnInt64(4); ok) record.pushKV("reputation", value);
                    if (auto[ok, value] = cursor.TryGetColumnInt64(5); ok) record.pushKV("value", value);

                    result.push_back(record);
                }
            });
        });

        return result;
    }

    UniValue WebRpcRepository::GetAccountRaters(const string& address)
    {
        UniValue result(UniValue::VARR);

        SqlTransaction(__func__, [&]()
        {
            Sql(R"sql(
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
                                r.RegId2 = c.HashId and
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
            .Bind(address)
            .Select([&](Cursor& cursor) {
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
        });

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
        
        SqlTransaction(__func__, [&]()
        {
            auto& stmt = Sql(sql);
            stmt.Bind(address);
            if (limit > 0)
            {
                stmt.Bind(limit, offset);
            }

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
        });

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
        
        SqlTransaction(__func__, [&]()
        {
            auto& stmt = Sql(sql);
            stmt.Bind(address);
            if (limit > 0)
                stmt.Bind(limit, offset);

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
        });

        return result;
    }

    UniValue WebRpcRepository::GetBlockings(const string& address)
    {
        UniValue result(UniValue::VARR);

        SqlTransaction(__func__, [&]()
        {
            Sql(R"sql(
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
                    bl.IdTarget
                from
                    addr
                cross join
                    Transactions us indexed by Transactions_Type_RegId1_RegId2_RegId3
                        on us.Type in (100) and us.RegId1 = addr.id
                cross join
                    Chain ct
                        on ct.TxId = us.RowId
                cross join
                    Last lut
                        on lut.TxId = us.RowId
                cross join
                    BlockingLists bl indexed by BlockingLists_IdSource_IdTarget
                        on bl.IdSource = ct.Uid
            )sql")
            .Bind(address)
            .Select([&](Cursor& cursor) {
                while (cursor.Step())
                    if (auto[ok, value] = cursor.TryGetColumnInt64(0); ok)
                        result.push_back(value);
            });


        });

        return result;
    }
    
    UniValue WebRpcRepository::GetBlockers(const string& address)
    {
        UniValue result(UniValue::VARR);

        SqlTransaction(__func__, [&]()
        {
            Sql(R"sql(
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
                    bl.IdSource
                from
                    addr
                cross join
                    Transactions ut indexed by Transactions_Type_RegId1_RegId2_RegId3
                        on ut.Type = 100 and ut.RegId1 = addr.id
                cross join
                    Chain ct
                        on ct.TxId = ut.RowId
                cross join
                    Last lut
                        on lut.TxId = ut.RowId
                cross join
                    BlockingLists bl indexed by BlockingLists_IdTarget_IdSource
                        on bl.IdTarget = ct.Uid
            )sql")
            .Bind(address)
            .Select([&](Cursor& cursor) {
                while (cursor.Step())
                    if (auto[ok, value] = cursor.TryGetColumnInt64(0); ok)
                        result.push_back(value);
            });
        });

        return result;
    }

    // TODO (aok, api): implement
    vector<string> WebRpcRepository::GetTopAccounts(int topHeight, int countOut, const string& lang,
        const vector<string>& tags, const vector<int>& contentTypes,
        const vector<string>& addrsExcluded, const vector<string>& tagsExcluded, int depth,
        int badReputationLimit)
    {
        auto func = __func__;
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

        SqlTransaction(func, [&]()
        {
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
            stmt.Select([&](Cursor& cursor) {
                // ---------------------------------------------
                while (cursor.Step())
                {
                    if (auto[ok, value] = cursor.TryGetColumnString(0); ok) result.push_back(value);
                }
            });
        });

        // Complete!
        return result;
    }

    UniValue WebRpcRepository::GetTags(const string& lang, int pageSize, int pageStart)
    {
        UniValue result(UniValue::VARR);

        SqlTransaction(__func__, [&]()
        {
            Sql(R"sql(
                select
                    t.Lang,
                    t.Value,
                    t.Count
                from
                    web.Tags t
                where
                    t.Lang = ?
                group by
                    t.Id
                order by
                    t.Count desc
                limit ?
                offset ?
            )sql")
            .Bind(lang, pageSize, pageStart)
            .Select([&](Cursor& cursor) {
                while (cursor.Step())
                {
                    auto[ok0, vLang] = cursor.TryGetColumnString(0);
                    auto[ok1, vValue] = cursor.TryGetColumnString(1);
                    auto[ok2, vCount] = cursor.TryGetColumnInt(2);

                    UniValue record(UniValue::VOBJ);
                    record.pushKV("tag", vValue);
                    record.pushKV("count", vCount);

                    result.push_back(record);
                }
            });
        });

        return result;
    }

    vector<int64_t> WebRpcRepository::GetContentIds(const vector<string>& txHashes)
    {
        vector<int64_t> result;

        if (txHashes.empty())
            return result;

        SqlTransaction(__func__, [&]()
        {
            Sql(R"sql(
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
                    Transactions t indexed by Transactions_HashId
                        on t.HashId = tx.id
                cross join
                    Chain c
                        on c.TxId = t.RowId
            )sql")
            .Bind(txHashes)
            .Select([&](Cursor& cursor) {
                while (cursor.Step())
                {
                    if (auto[ok, value] = cursor.TryGetColumnInt64(0); ok)
                        result.push_back(value);
                }
            });
        });

        return result;
    }

    map<string,string> WebRpcRepository::GetContentsAddresses(const vector<string>& txHashes)
    {
        map<string, string> result;

        if (txHashes.empty())
            return result;

        SqlTransaction(__func__, [&]()
        {
            Sql(R"sql(
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
                    Transactions t indexed by Transactions_HashId
                        on t.HashId = tx.id
                cross join
                    Chain c
                        on c.TxId = t.RowId
            )sql")
            .Bind(txHashes)
            .Select([&](Cursor& cursor) {
                while (cursor.Step())
                {
                    auto[ok0, contenthash] = cursor.TryGetColumnString(0);
                    auto[ok1, contentaddress] = cursor.TryGetColumnString(1);
                    if(ok0 && ok1)
                        result.emplace(contenthash,contentaddress);
                }
            });
        });

        return result;
    }

    UniValue WebRpcRepository::GetUnspents(const vector<string>& addresses, int height, int confirmations, vector<pair<string, uint32_t>>& mempoolInputs)
    {
        UniValue result(UniValue::VARR);

        SqlTransaction(__func__, [&]()
        {
            Sql(R"sql(
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
                    (select String from Registry where RowId=t.HashId),
                    o.Number,
                    (select String from Registry where RowId=o.AddressId),
                    o.Value,
                    (select String from Registry where RowId=o.ScriptPubKeyId),
                    t.Type,
                    c.Height
                from addr
                join TxOutputs o indexed by TxOutputs_AddressId_TxId_Number on
                    o.AddressId = addr.id and not exists (select 1 from TxInputs i where i.TxId = o.TxId and i.Number = o.Number)
                join Chain c on
                    c.TxId = o.TxId and c.Height <= ?
                join Transactions t on
                    t.RowId = o.TxId
                order by c.Height asc
            )sql")
            .Bind(addresses, height - confirmations)
            .Select([&](Cursor& cursor) {
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
        });

        return result;
    }

    UniValue WebRpcRepository::GetAccountEarning(const string& address, int height, int depth)
    {
        UniValue result(UniValue::VARR);

        SqlTransaction(__func__, [&]()
        {
            Sql(R"sql(
                with
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
                    addr.hash,
                    ifnull(t.Type, 0) as type,
                    sum(ifnull(o.Value, 0)) as amount
                from
                    addr
                cross join
                    TxOutputs o indexed by TxOutputs_AddressId_TxId_Number on
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
                group by t.Type
            )sql")
            .Bind(address,height,height - depth)
            .Select([&](Cursor& cursor) {
                int64_t amountLottery = 0;
                int64_t amountDonation = 0;
                int64_t amountTransfer = 0;

                while (cursor.Step())
                {
                    int64_t amount = 0;
                    if (auto[ok, type] = cursor.TryGetColumnInt(1); ok)
                    {
                        if (auto[ok, value] = cursor.TryGetColumnInt64(2); ok) amount = value;

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
        });

        return result;
    }

    tuple<int, UniValue> WebRpcRepository::GetContentLanguages(int height)
    {
        int resultCount = 0;
        UniValue resultData(UniValue::VOBJ);

        SqlTransaction(__func__, [&]()
        {
            Sql(R"sql(
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
            .Bind(height)
            .Select([&](Cursor& cursor) {
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
        });

        return {resultCount, resultData};
    }

    tuple<int, UniValue> WebRpcRepository::GetLastAddressContent(const string& address, int height, int count)
    {
        int resultCount = 0;
        SqlTransaction(__func__, [&]()
        {
            Sql(R"sql(
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
            .Bind(address, height)
            .Select([&](Cursor& cursor) {
                if (cursor.Step())
                    if (auto[ok, value] = cursor.TryGetColumnInt(0); ok)
                        resultCount = value;
            });


        });

        // Try get last N records
        UniValue resultData(UniValue::VARR);
        if (resultCount > 0)
        {
            SqlTransaction(__func__, [&]()
            {
                Sql(R"sql(
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
                .Bind(address, height, count)
                .Select([&](Cursor& cursor) {
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
            });
        }

        return {resultCount, resultData};
    }
    
    UniValue WebRpcRepository::GetContentsForAddress(const string& address)
    {
        auto func = __func__;
        UniValue result(UniValue::VARR);

        if (address.empty())
            return result;

        SqlTransaction(func, [&]()
        {
            Sql(R"sql(
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
            .Bind(address)
            .Select([&](Cursor& cursor) {
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
        });

        return result;
    }

    vector<UniValue> WebRpcRepository::GetMissedRelayedContent(const string& address, int height)
    {
        vector<UniValue> result;

        SqlTransaction(__func__, [&]()
        {
            Sql(R"sql(
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
                    Transactions p indexed by Transactions_HashId
                        on p.HashId = r.RegId3 and p.Type in (200, 201) and p.RegId1 = addr.id
            )sql")
            .Bind(address, height)
            .Select([&](Cursor& cursor) {
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
        });

        return result;
    }

    vector<UniValue> WebRpcRepository::GetMissedContentsScores(const string& address, int height, int limit)
    {
        vector<UniValue> result;

        SqlTransaction(__func__, [&]()
        {
            Sql(R"sql(
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
                    (select r.String from Registry r where r.RowId = s.HashId),
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
            .Bind(address, height, limit)
            .Select([&](Cursor& cursor) {
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
        });

        return result;
    }

    vector<UniValue> WebRpcRepository::GetMissedCommentsScores(const string& address, int height, int limit)
    {
        vector<UniValue> result;

        SqlTransaction(__func__, [&]()
        {
            Sql(R"sql(
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
                    (select r.String from Registry r where r.RowId = s.HashId),
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
            .Bind(address, height, limit)
            .Select([&](Cursor& cursor) {
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
        });

        return result;
    }

    map<string, UniValue> WebRpcRepository::GetMissedTransactions(const string& address, int height, int count)
    {
        map<string, UniValue> result;

        SqlTransaction(__func__, [&]()
        {
            Sql(R"sql(
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
                    (select r.String from Registry r where r.RowId = t.HashId),
                    t.Time,
                    o.Value,
                    c.Height,
                    t.Type
                from
                    addr,
                    height
                cross join
                    TxOutputs o indexed by TxOutputs_AddressId_TxId_Number
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
            .Bind(address, height, count)
            .Select([&](Cursor& cursor) {
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
        });

        return result;
    }

    vector<UniValue> WebRpcRepository::GetMissedCommentAnswers(const string& address, int height, int count)
    {
        vector<UniValue> result;

        SqlTransaction(__func__, [&]()
        {
            Sql(R"sql(
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
            .Bind(address, height, count)
            .Select([&](Cursor& cursor) {
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
        });

        return result;
    }

    vector<UniValue> WebRpcRepository::GetMissedPostComments(const string& address, const vector<string>& excludePosts, int height, int count)
    {
        vector<UniValue> result;

        SqlTransaction(__func__, [&]()
        {
            Sql(R"sql(
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
                        from TxOutputs o indexed by TxOutputs_AddressId_TxId_Number
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
            .Bind(address, height, excludePosts, count)
            .Select([&](Cursor& cursor) {
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
                    if (auto[ok, value] = cursor.TryGetColumnString(7); ok)
                    {
                        record.pushKV("donation", "true");
                        record.pushKV("amount", value);
                    }

                    result.push_back(record);
                }
            });
        });

        return result;
    }

    vector<UniValue> WebRpcRepository::GetMissedSubscribers(const string& address, int height, int count)
    {
        vector<UniValue> result;

        SqlTransaction(__func__, [&]()
        {
            Sql(R"sql(
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
                    (select r.String from Registry r where r.RowId = subs.HashId),
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
            .Bind(address, height, count)
            .Select([&](Cursor& cursor) {
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
        });

        return result;
    }

    vector<UniValue> WebRpcRepository::GetMissedBoosts(const string& address, int height, int count)
    {
        vector<UniValue> result;

        SqlTransaction(__func__, [&]()
        {
            Sql(R"sql(
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
                    (select r.String from Registry r where r.RowId = tBoost.HashId),
                    tBoost.Time,
                    cb.Height,
                    (select r.String from Registry r where r.RowId = tBoost.RegId2) as contenttxid,
                    tBoost.Int1 as boostAmount,
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
            .Bind(address, height, count)
            .Select([&](Cursor& cursor) {
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
        });

        return result;
    }

    UniValue WebRpcRepository::SearchLinks(const vector<string>& links, const vector<int>& contentTypes, const int nHeight, const int countOut)
    {
        UniValue result(UniValue::VARR);

        if (links.empty())
            return result;

        vector<int64_t> ids;
        SqlTransaction(__func__, [&]()
        {
            Sql(R"sql(
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
            .Bind(nHeight, links, contentTypes, countOut)
            .Select([&](Cursor& cursor) {
                while (cursor.Step())
                {
                    if (auto[ok, value] = cursor.TryGetColumnInt64(0); ok)
                        ids.push_back(value);
                }
            });
        });

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

        SqlTransaction(__func__, [&]()
        {
            Sql(R"sql(
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
            .Bind(addresses, contentTypes)
            .Select([&](Cursor& cursor) {
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
        });

        return result;
    }

    // ------------------------- Feeds ---------------------

    UniValue WebRpcRepository::GetHotPosts(int countOut, const int depth, const int nHeight, const string& lang,
        const vector<int>& contentTypes, const string& address, int badReputationLimit)
    {
        UniValue result(UniValue::VARR);

        vector<int64_t> ids;
        SqlTransaction(__func__, [&]()
        {
            Sql(R"sql(
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
                where
                    -- Do not show posts from users with low reputation
                    ifnull(ur.Value,0) > ?
                order by
                    r.Value desc
                limit ?
            )sql")
            .Bind(
                lang,
                nHeight,
                nHeight - depth,
                contentTypes,
                badReputationLimit,
                countOut
            )
            .Select([&](Cursor& cursor) {
                while (cursor.Step())
                    if (auto[ok, value] = cursor.TryGetColumnInt(0); ok)
                        ids.push_back(value);
            });
        });

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
        auto func = __func__;
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
                        t.HashId as id
                    from
                        Chain c
                    cross join
                        Transactions t
                            on t.RowId = c.TxId
                    where c.Uid in ( )sql" + join(vector<string>(ids.size(), "?"), ",") + R"sql( )
                ),
            )sql";
        }
        
        // Get posts
        unordered_map<int64_t, UniValue> tmpResult{};
        vector<string> authors;
        SqlTransaction(func, [&]()
        {
            Sql(R"sql(
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
                    (select String from Registry where RowId = t.HashId) as Hash,
                    (select String from Registry where RowId = t.RegId2) as RootTxHash,
                    c.Uid as Id,
                    case when t.HashId != t.RegId2 then 'true' else null end edit,
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
                        left join BlockingLists bl
                            on bl.IdSource = c.Uid and bl.IdTarget = cc.Uid
                        where
                            c.Type in (204, 205) and
                            c.RegId3 = t.RegId2 and
                            bl.ROWID is null
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
                                    'hs', (select rv.String from Registry rv where rv.RowId = tv.HashId)
                                )
                            )
                        from
                            Transactions tv indexed by Transactions_Type_RegId2_RegId1
                        cross join Chain cv
                            on cv.TxId = tv.RowId
                        where
                            tv.Type = t.Type and
                            tv.RegId2 = t.RegId2 and
                            tv.HashId != t.HashId
                    ) as Versions
                from
                    txs,
                    addr
                cross join
                    Transactions t indexed by Transactions_HashId
                        on t.HashId = txs.id and t.Type in (200, 201, 202, 209, 210)
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
            .Bind(hashes, ids, address)
            .Select([&](Cursor& cursor) {
                while (cursor.Step())
                {
                    int ii = 0;
                    UniValue record(UniValue::VOBJ);

                    cursor.Collect<string>(ii++, record, "hash");
                    cursor.Collect<string>(ii++, record, "txid");
                    int64_t id;
                    if (!cursor.Collect(ii++, id)) {
                        continue; // TODO (aok): error
                    }
                    record.pushKV("id", id);
                    cursor.Collect<string>(ii++, record, "edit");
                    cursor.Collect<string>(ii++, record, "repost");
                    cursor.Collect(ii++, [&](const string& value) {
                        authors.emplace_back(value);
                        record.pushKV("address", value);
                    });
                    cursor.Collect<int64_t>(ii++, record, "time"); // TODO (aok): is it str?
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
                        UniValue ii(UniValue::VARR);
                        ii.read(value);
                        record.pushKV("i", ii);
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
                    cursor.Collect<int>(ii++, record, "myVal");
                    cursor.Collect(ii++, [&](const string& value) {
                        UniValue ii(UniValue::VARR);
                        ii.read(value);
                        record.pushKV("versions", ii);
                    });

                    tmpResult[id] = record;                
                }
            });
        });

        // ---------------------------------------------
        // Get last comments for all posts
        auto lastComments = GetLastComments(ids, address);
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
        auto func = __func__;
        vector<UniValue> result{};

        if (ids.empty())
            return result;

        unordered_map<int64_t, UniValue> tmpResult{};
        SqlTransaction(func, [&]()
        {
            Sql(R"sql(
                select
                    (select r.String from Registry r where r.RowId = t.RegId2) as RootTxHash,
                    c.Uid,
                    case when t.HashId != t.RegId2 then 'true' else null end edit,
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
            .Bind(ids)
            .Select([&](Cursor& cursor) {
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
        });

        // ---------------------------------------------
        // Place in result data with source sorting
        for (auto& id : ids)
            result.push_back(tmpResult[id]);

        return result;
    }

    // TODO (aok, api) : optimization
    UniValue WebRpcRepository::GetTopFeed(int countOut, const int64_t& topContentId, int topHeight,
        const string& lang, const vector<string>& tags, const vector<int>& contentTypes,
        const vector<string>& txidsExcluded, const vector<string>& addrsExcluded, const vector<string>& tagsExcluded,
        const string& address, int depth, int badReputationLimit)
    {
        auto func = __func__;
        UniValue result(UniValue::VARR);

        if (contentTypes.empty())
            return result;

        // --------------------------------------------

        string contentTypesWhere = " ( " + join(vector<string>(contentTypes.size(), "?"), ",") + " ) ";

        string contentIdWhere;
        if (topContentId > 0)
            contentIdWhere = " and ct.Id < ? ";

        string langFilter;
        if (!lang.empty())
            langFilter += " join Payload p on p.TxId = t.RowId and p.String1 = ? ";

        string sql = R"sql(
            select ct.Uid

            from Transactions t --indexed by Transactions_Type_Last_Height_Id
            join Chain ct indexed by Chain_Uid_Height on ct.TxId = t.RowId
            join Last lt on lt.TxId = t.RowId

            join Ratings cr indexed by Ratings_Type_Uid_Last_Value
                on cr.Type = 2 and cr.Uid=ct.Uid and cr.Last = 1 and cr.Value > 0

            )sql" + langFilter + R"sql(

            cross join Transactions u indexed by Transactions_Type_RegId1_Time
                on u.Type in (100) and u.RegId1 = t.RegId1
            join Chain cu on cu.TxId = u.RowId
            join Last lu on lu.TxId = u.RowId

            left join Ratings ur indexed by Ratings_Type_Uid_Last_Value
                on ur.Type=0 and ur.Uid=cu.Uid and ur.Last=1

            where t.Type in )sql" + contentTypesWhere + R"sql(
                --and t.String3 is null
                and ct.Height > ?
                and ct.Height <= ?

                -- Do not show posts from users with low reputation
                and ifnull(ur.Value,0) > ?

                )sql" + contentIdWhere + R"sql(
        )sql";

        if (!tags.empty())
        {
            sql += R"sql(
                and ct.Uid in (
                    select tm.ContentId
                    from web.Tags tag
                    join web.TagsMap tm indexed by TagsMap_TagId_ContentId
                        on tag.Id = tm.TagId
                    where tag.Value in ( )sql" + join(vector<string>(tags.size(), "?"), ",") + R"sql( )
                        )sql" + (!lang.empty() ? " and tag.Lang = ? " : "") + R"sql(
                )
            )sql";
        }

        if (!txidsExcluded.empty()) sql += " and t.RegId2 not in ( select RowId from Registry where String in ( " + join(vector<string>(txidsExcluded.size(), "?"), ",") + " ) ) ";
        if (!addrsExcluded.empty()) sql += " and t.RegId1 not in ( select RowId from Registry where String in ( " + join(vector<string>(addrsExcluded.size(), "?"), ",") + " ) ) ";
        if (!tagsExcluded.empty())
        {
            sql += R"sql( and ct.Uid not in (
                select tmEx.ContentId
                from web.Tags tagEx
                join web.TagsMap tmEx indexed by TagsMap_TagId_ContentId
                    on tagEx.Id=tmEx.TagId
                where tagEx.Value in ( )sql" + join(vector<string>(tagsExcluded.size(), "?"), ",") + R"sql( )
                    )sql" + (!lang.empty() ? " and tagEx.Lang = ? " : "") + R"sql(
             ) )sql";
        }

        sql += " order by cr.Value desc ";
        sql += " limit ? ";

        // ---------------------------------------------

        vector<int64_t> ids;

        SqlTransaction(func, [&]()
        {
            auto& stmt = Sql(sql);

            if (!lang.empty()) stmt.Bind(lang);

            stmt.Bind(contentTypes, topHeight - depth, topHeight, badReputationLimit);

            if (topContentId > 0)
                stmt.Bind(topContentId);

            if (!tags.empty())
            {
                stmt.Bind(tags);

                if (!lang.empty())
                    stmt.Bind(lang);
            }

            stmt.Bind(txidsExcluded, addrsExcluded);

            if (!tagsExcluded.empty())
            {
                stmt.Bind(tagsExcluded);

                if (!lang.empty())
                    stmt.Bind(lang);
            }

            stmt.Bind(countOut);

            // ---------------------------------------------

            stmt.Select([&](Cursor& cursor) {
                while (cursor.Step())
                {
                    auto[ok0, contentId] = cursor.TryGetColumnInt64(0);
                    ids.push_back(contentId);
                }
            });
        });

        // Get content data
        if (!ids.empty())
        {
            auto contents = GetContentsData({}, ids, address);
            result.push_backV(contents);
        }

        // Complete!
        return result;
    }

    // TODO (aok, api) : optimization
    UniValue WebRpcRepository::GetMostCommentedFeed(int countOut, const int64_t& topContentId, int topHeight,
        const string& lang, const vector<string>& tags, const vector<int>& contentTypes,
        const vector<string>& txidsExcluded, const vector<string>& addrsExcluded, const vector<string>& tagsExcluded,
        const string& address, int depth, int badReputationLimit)
    {
        auto func = __func__;
        UniValue result(UniValue::VARR);

        if (contentTypes.empty())
            return result;

        // --------------------------------------------

        string contentTypesWhere = " ( " + join(vector<string>(contentTypes.size(), "?"), ",") + " ) ";

        string contentIdWhere;
        // if (topContentId > 0)
        //     contentIdWhere = " and t.Id < ? ";

        string langFilter;
        if (!lang.empty())
            langFilter += " join Payload p on p.TxId = t.RowId and p.String1 = ? ";

        string sql = R"sql(
            select ct.Uid

            from Transactions t indexed by Transactions_Type_RegId2_RegId1
            join Chain ct on ct.TxId = t.RowId
            join Last lt on lt.TxId = t.RowId

            join Ratings cr indexed by Ratings_Type_Uid_Last_Value
                on cr.Type = 2 and cr.Last = 1 and cr.Uid = ct.Uid and cr.Value > 0

            )sql" + langFilter + R"sql(

            join Transactions u indexed by Transactions_Type_RegId1_Time
                on u.Type in (100) and u.RegId1 = t.RegId1
            join Chain cu on cu.TxId = u.RowId
            join Last lu on lu.TxId = u.RowId

            left join Ratings ur indexed by Ratings_Type_Uid_Last_Value
                on ur.Type=0 and ur.Uid=cu.Uid and ur.Last=1

            where t.Type in )sql" + contentTypesWhere + R"sql(
                --and t.String3 is null
                and ct.Height > ?
                and ct.Height <= ?

                -- Do not show posts from users with low reputation
                and ifnull(ur.Value,0) > ?

                )sql" + contentIdWhere + R"sql(
        )sql";

        if (!tags.empty())
        {
            sql += R"sql(
                and ct.Uid in (
                    select tm.ContentId
                    from web.Tags tag
                    join web.TagsMap tm indexed by TagsMap_TagId_ContentId
                        on tag.Id = tm.TagId
                    where tag.Value in ( )sql" + join(vector<string>(tags.size(), "?"), ",") + R"sql( )
                        )sql" + (!lang.empty() ? " and tag.Lang = ? " : "") + R"sql(
                )
            )sql";
        }

        if (!txidsExcluded.empty()) sql += " and t.RegId2 not in ( select RowId from Registry where String in ( " + join(vector<string>(txidsExcluded.size(), "?"), ",") + " ) ) ";
        if (!addrsExcluded.empty()) sql += " and t.RegId1 not in ( select RowId from Registry where String in ( " + join(vector<string>(addrsExcluded.size(), "?"), ",") + " ) ) ";
        if (!tagsExcluded.empty())
        {
            sql += R"sql( and ct.Uid not in (
                select tmEx.ContentId
                from web.Tags tagEx
                join web.TagsMap tmEx indexed by TagsMap_TagId_ContentId
                    on tagEx.Id=tmEx.TagId
                where tagEx.Value in ( )sql" + join(vector<string>(tagsExcluded.size(), "?"), ",") + R"sql( )
                    )sql" + (!lang.empty() ? " and tagEx.Lang = ? " : "") + R"sql(
             ) )sql";
        }

        // sql += " order by cr.Value desc ";
        sql +=  R"sql(
        order by (
                select count()
                from Transactions s indexed by Transactions_Type_RegId3_RegId1
                join Chain cs on cs.TxId = s.RowId
                join Last ls on ls.TxId = s.RowId
                where s.Type in (204, 205) and s.RegId3 = t.RegId2
                    -- exclude commenters blocked by the author of the post
                    and not exists (
                                    select 1
                                    from BlockingLists bl
                                    where bl.IdSource = t.RegId1 and bl.IdTarget = s.RegId1
                                    )
                ) desc
        limit ?
        )sql";

        // ---------------------------------------------

        vector<int64_t> ids;

        SqlTransaction(func, [&]()
        {
            auto& stmt = Sql(sql);

            if (!lang.empty()) stmt.Bind(lang);

            stmt.Bind(contentTypes, topHeight - depth, topHeight, badReputationLimit);

            // if (topContentId > 0)
            //     stmt.TryBindStatementInt64(i++, topContentId);

            if (!tags.empty())
            {
                stmt.Bind(tags);

                if (!lang.empty())
                    stmt.Bind(lang);
            }

            stmt.Bind(txidsExcluded, addrsExcluded);

            if (!tagsExcluded.empty())
            {
                stmt.Bind(tagsExcluded);

                if (!lang.empty())
                    stmt.Bind(lang);
            }

            stmt.Bind(countOut);

            // ---------------------------------------------

            stmt.Select([&](Cursor& cursor) {
                while (cursor.Step())
                {
                    auto[ok0, contentId] = cursor.TryGetColumnInt64(0);
                    ids.push_back(contentId);
                }
            });
        });

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
                    left join
                        BlockingLists bl indexed by BlockingLists_IdSource_IdTarget on
                            bl.IdSource = t.RegId1 and
                            bl.IdTarget = s.RegId1
                    where
                        s.Type in (204, 205) and
                        s.RegId3 = t.RegId2 and
                        -- exclude commenters blocked by the author of the post
                        bl.IdSource is null
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
        SqlTransaction(__func__, [&]()
        {
            Sql(R"sql(
                with
                    height as ( select ? as value ),
                    addr as ( select RowId as id, String as hash from Registry where String = ?),
                    lang as ( select ? as value ),
                    topContentId as ( select ? as value )
                select distinct
                    ct.Uid
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
            )
            .Select([&](Cursor& cursor) {
                while (cursor.Step())
                {
                    if (auto[ok, value] = cursor.TryGetColumnInt64(0); ok)
                        ids.push_back(value);
                }
            });
        });

        // Get content data
        if (!ids.empty())
        {
            auto contents = GetContentsData({}, ids, address);
            result.push_backV(contents);
        }

        return result;
    }

    // TODO (aok, api) : optimization
    UniValue WebRpcRepository::GetSubscribesFeed(const string& addressFeed, int countOut, const int64_t& topContentId, int topHeight,
        const string& lang, const vector<string>& tags, const vector<int>& contentTypes,
        const vector<string>& txidsExcluded, const vector<string>& addrsExcluded, const vector<string>& tagsExcluded,
        const string& address, const vector<string>& addresses_extended)
    {
        auto func = __func__;
        UniValue result(UniValue::VARR);

        if (addressFeed.empty())
            return result;

        // ---------------------------------------------------

        string contentTypesWhere = " ( " + join(vector<string>(contentTypes.size(), "?"), ",") + " ) ";

        string contentIdWhere;
        if (topContentId > 0)
            contentIdWhere = " and t.Id < ? ";

        string langFilter;
        if (!lang.empty())
            langFilter += " join Payload p on p.TxId = t.RowId and p.String1 = ? ";

        string sql = R"sql(
            select ct.Uid

            from Transactions t indexed by Transactions_Type_RegId1_Time
            join Chain ct on ct.TxId = t.RowId
            join Last lt on lt.TxId = t.RowId

            join Transactions u indexed by Transactions_Type_RegId1_Time
                on u.Type in (100) and u.RegId1 = t.RegId1
            join Chain cu on cu.TxId = u.RowId
            join Last lu on lu.TxId = u.RowId

            )sql" + langFilter + R"sql(

            where t.Type in )sql" + contentTypesWhere + R"sql(
              and t.RegId1 in (
                  select subs.RegId2
                  from Transactions subs indexed by Transactions_Type_RegId1_RegId2_RegId3
                  join Chain csubs on csubs.TxId = subs.RowId
                  join Last lsubs on lsubs.TxId = subs.RowId
                  where subs.Type in (302,303)
                    and subs.RegId1 = (select RowId from Registry indexed by Registry_String where String = ?)
                    )sql" + (!addresses_extended.empty() ? (" union select RowId from Registry indexed by Registry_String where String in (" + join(vector<string>(addresses_extended.size(), "?"), ",") + ")") : "") + R"sql(
              )
              and ct.Height <= ?
            
            )sql" + contentIdWhere + R"sql(

        )sql";

        if (!tags.empty())
        {
            sql += R"sql(
                and ct.Uid in (
                    select tm.ContentId
                    from web.Tags tag
                    join web.TagsMap tm indexed by TagsMap_TagId_ContentId
                        on tag.Id = tm.TagId
                    where tag.Value in ( )sql" + join(vector<string>(tags.size(), "?"), ",") + R"sql( )
                        )sql" + (!lang.empty() ? " and tag.Lang = ? " : "") + R"sql(
                )
            )sql";
        }

        if (!txidsExcluded.empty()) sql += " and t.RegId2 not in ( select RowId from Registry where String in ( " + join(vector<string>(txidsExcluded.size(), "?"), ",") + " ) ) ";
        if (!addrsExcluded.empty()) sql += " and t.RegId1 not in ( select RowId from Registry where String in ( " + join(vector<string>(addrsExcluded.size(), "?"), ",") + " ) ) ";
        if (!tagsExcluded.empty())
        {
            sql += R"sql( and ct.Uid not in (
                select tmEx.ContentId
                from web.Tags tagEx
                join web.TagsMap tmEx indexed by TagsMap_TagId_ContentId
                    on tagEx.Id=tmEx.TagId
                where tagEx.Value in ( )sql" + join(vector<string>(tagsExcluded.size(), "?"), ",") + R"sql( )
                    )sql" + (!lang.empty() ? " and tagEx.Lang = ? " : "") + R"sql(
             ) )sql";
        }

        sql += " order by ct.Uid desc ";
        sql += " limit ? ";

        // ---------------------------------------------------

        vector<int64_t> ids;
        SqlTransaction(func, [&]()
        {
            auto& stmt = Sql(sql);

            if (!lang.empty()) stmt.Bind(lang);

            stmt.Bind(contentTypes, addressFeed, addresses_extended, topHeight);

            if (!tags.empty())
            {
                stmt.Bind(tags);

                if (!lang.empty())
                    stmt.Bind(lang);
            }

            stmt.Bind(txidsExcluded, addrsExcluded);

            if (!tagsExcluded.empty())
            {
                stmt.Bind(tagsExcluded);

                if (!lang.empty())
                    stmt.Bind(lang);
            }

            stmt.Bind(countOut);

            // ---------------------------------------------

            stmt.Select([&](Cursor& cursor) {
                while (cursor.Step())
                {
                    if (auto[ok, value] = cursor.TryGetColumnInt64(0); ok)
                        ids.push_back(value);
                }
            });
        });

        // Get content data
        if (!ids.empty())
        {
            auto contents = GetContentsData({}, ids, address);
            result.push_backV(contents);
        }

        // Complete!
        return result;
    }

    // TODO (aok, api) : optimization
    UniValue WebRpcRepository::GetHistoricalFeed(int countOut, const int64_t& topContentId, int topHeight,
        const string& lang, const vector<string>& tags, const vector<int>& contentTypes,
        const vector<string>& txidsExcluded, const vector<string>& addrsExcluded, const vector<string>& tagsExcluded,
        const string& address, int badReputationLimit)
    {
        auto func = __func__;
        UniValue result(UniValue::VARR);

        if (contentTypes.empty())
            return result;

        // --------------------------------------------

        string contentTypesWhere = " ( " + join(vector<string>(contentTypes.size(), "?"), ",") + " ) ";

        string contentIdWhere;
        if (topContentId > 0)
            contentIdWhere = " and ct.Uid < ? ";

        string langFilter;
        if (!lang.empty())
            langFilter += " cross join Payload p on p.TxId = t.RowId and p.String1 = ? ";

        string sql = R"sql(
            select ct.Uid

            from Chain ct
            cross join Transactions t on ct.TxId = t.RowId
            cross join Last lt on lt.TxId = t.RowId

            )sql" + langFilter + R"sql(

            cross join Transactions u indexed by Transactions_Type_RegId1_RegId2_RegId3
                on u.Type in (100) and u.RegId1 = t.RegId1
            cross join Chain cu on cu.TxId = u.RowId
            cross join Last lu on lu.TxId = u.RowId

            left join Ratings ur indexed by Ratings_Type_Uid_Last_Value
                on ur.Type=0 and ur.Uid=cu.Uid and ur.Last=1

            where t.Type in )sql" + contentTypesWhere + R"sql(
                and t.RegId3 is null
                and ct.Height <= ?

                -- Do not show posts from users with low reputation
                and ifnull(ur.Value,0) > ?

                )sql" + contentIdWhere + R"sql(
        )sql";

        if (!tags.empty())
        {
            sql += R"sql(
                and ct.Uid in (
                    select tm.ContentId
                    from web.Tags tag
                    join web.TagsMap tm indexed by TagsMap_TagId_ContentId
                        on tag.Id = tm.TagId
                    where tag.Value in ( )sql" + join(vector<string>(tags.size(), "?"), ",") + R"sql( )
                        )sql" + (!lang.empty() ? " and tag.Lang = ? " : "") + R"sql(
                )
            )sql";
        }

        if (!txidsExcluded.empty()) sql += " and t.RegId2 not in ( select RowId from Registry where String in ( " + join(vector<string>(txidsExcluded.size(), "?"), ",") + " ) ) ";
        if (!addrsExcluded.empty()) sql += " and t.RegId1 not in ( select RowId from Registry where String in ( " + join(vector<string>(addrsExcluded.size(), "?"), ",") + " ) ) ";
        if (!tagsExcluded.empty())
        {
            sql += R"sql( and ct.Uid not in (
                select tmEx.ContentId
                from web.Tags tagEx
                join web.TagsMap tmEx indexed by TagsMap_TagId_ContentId
                    on tagEx.Id=tmEx.TagId
                where tagEx.Value in ( )sql" + join(vector<string>(tagsExcluded.size(), "?"), ",") + R"sql( )
                    )sql" + (!lang.empty() ? " and tagEx.Lang = ? " : "") + R"sql(
             ) )sql";
        }

        sql += " order by ct.Uid desc ";
        sql += " limit ? ";

        // ---------------------------------------------

        vector<int64_t> ids;

        SqlTransaction(func, [&]()
        {
            auto& stmt = Sql(sql);

            if (!lang.empty()) stmt.Bind(lang);

            stmt.Bind(contentTypes, topHeight, badReputationLimit);

            if (topContentId > 0)
                stmt.Bind(topContentId);

            if (!tags.empty())
            {
                stmt.Bind(tags);

                if (!lang.empty())
                    stmt.Bind(lang);
            }

            stmt.Bind(txidsExcluded, addrsExcluded);
            
            if (!tagsExcluded.empty())
            {
                stmt.Bind(tagsExcluded);

                if (!lang.empty())
                    stmt.Bind(lang);
            }
                    
            stmt.Bind(countOut);

            // ---------------------------------------------

            stmt.Select([&](Cursor& cursor) {
                while (cursor.Step())
                {
                    auto[ok0, contentId] = cursor.TryGetColumnInt64(0);
                    ids.push_back(contentId);
                }
            });
        });

        // Get content data
        if (!ids.empty())
        {
            auto contents = GetContentsData({}, ids, address);
            result.push_backV(contents);
        }

        return result;
    }

    // TODO (aok, api) : optimization
    UniValue WebRpcRepository::GetHierarchicalFeed(int countOut, const int64_t& topContentId, int topHeight,
        const string& lang, const vector<string>& tags, const vector<int>& contentTypes,
        const vector<string>& txidsExcluded, const vector<string>& addrsExcluded, const vector<string>& tagsExcluded,
        const string& address, int badReputationLimit)
    {
        auto func = __func__;
        UniValue result(UniValue::VARR);

        // ---------------------------------------------

        string contentTypesFilter = join(vector<string>(contentTypes.size(), "?"), ",");

        string langFilter;
        if (!lang.empty())
            langFilter += " join Payload p on p.TxId = t.RowId and p.String1 = ? ";

        string sql = R"sql(
            select
                (ct.Uid)ContentId,
                ifnull(pr.Value,0)ContentRating,
                ifnull(ur.Value,0)AccountRating,
                ctorig.Height,
                
                ifnull((
                    select sum(ifnull(ptr.Value,0))
                    from (
                        select cpt.Uid
                        from Transactions pt indexed by Transactions_Type_RegId1_RegId2_RegId3
                        join Chain cpt on cpt.TxId = pt.RowId
                        join Last lpt on lpt.TxId = pt.RowId
                        where pt.Type in ( )sql" + contentTypesFilter + R"sql( )
                            and pt.RegId1 = t.RegId1
                            and cpt.Height < ctorig.Height
                            and cpt.Height > (ctorig.Height - ?)
                        order by cpt.Height desc
                        limit ?
                    )q
                    left join Ratings ptr indexed by Ratings_Type_Uid_Last_Height
                        on ptr.Type = 2 and ptr.Uid = q.Uid and ptr.Last = 1
                ), 0)SumRating

            from Transactions t indexed by Transactions_Type_RegId3_RegId1
            join Chain ct on ct.TxId = t.RowId
            join Last lt on lt.TxId = t.RowId

            )sql" + langFilter + R"sql(

            join Transactions torig indexed by Transactions_HashId on torig.HashId = t.RegId2
            join Chain ctorig on ctorig.TxId = torig.RowId

            left join Ratings pr indexed by Ratings_Type_Uid_Last_Height
                on pr.Type = 2 and pr.Last = 1 and pr.Uid = ct.Uid

            join Transactions u indexed by Transactions_Type_RegId1_RegId2_RegId3
                on u.Type in (100) and u.RegId1 = t.RegId1
            join Chain cu on cu.TxId = u.RowId
            join Last lu on lu.TxId = u.RowId

            left join Ratings ur indexed by Ratings_Type_Uid_Last_Value
                on ur.Type=0 and ur.Uid=cu.Uid and ur.Last=1

            where t.Type in ( )sql" + contentTypesFilter + R"sql( )
                and t.RegId3 is null
                and ct.Height <= ?
                and ct.Height > ?

                -- Do not show posts from users with low reputation
                and ifnull(ur.Value,0) > ?
        )sql";

        if (!tags.empty())
        {
            sql += R"sql(
                and ct.Uid in (
                    select tm.ContentId
                    from web.Tags tag
                    join web.TagsMap tm indexed by TagsMap_TagId_ContentId
                        on tag.Id = tm.TagId
                    where tag.Value in ( )sql" + join(vector<string>(tags.size(), "?"), ",") + R"sql( )
                        )sql" + (!lang.empty() ? " and tag.Lang = ? " : "") + R"sql(
                )
            )sql";
        }

        if (!txidsExcluded.empty()) sql += " and t.RegId2 not in ( select RowId from Registry where String in ( " + join(vector<string>(txidsExcluded.size(), "?"), ",") + " ) ) ";
        if (!addrsExcluded.empty()) sql += " and t.RegId1 not in ( select RowId from Registry where String in ( " + join(vector<string>(addrsExcluded.size(), "?"), ",") + " ) ) ";
        if (!tagsExcluded.empty())
        {
            sql += R"sql( and ct.Uid not in (
                select tmEx.ContentId
                from web.Tags tagEx
                join web.TagsMap tmEx indexed by TagsMap_TagId_ContentId
                    on tagEx.Id=tmEx.TagId
                where tagEx.Value in ( )sql" + join(vector<string>(tagsExcluded.size(), "?"), ",") + R"sql( )
                    )sql" + (!lang.empty() ? " and tagEx.Lang = ? " : "") + R"sql(
             ) )sql";
        }

        // ---------------------------------------------
        vector<HierarchicalRecord> postsRanks;
        double dekay = (contentTypes.size() == 1 && contentTypes[0] == CONTENT_VIDEO) ? dekayVideo : dekayContent;

        SqlTransaction(func, [&]()
        {
            auto& stmt = Sql(sql);

            stmt.Bind(contentTypes, durationBlocksForPrevPosts, cntPrevPosts);
            
            if (!lang.empty()) stmt.Bind(lang);

            stmt.Bind(contentTypes, topHeight, topHeight - cntBlocksForResult, badReputationLimit);

            if (!tags.empty())
            {
                stmt.Bind(tags);

                if (!lang.empty())
                    stmt.Bind(lang);
            }

            stmt.Bind(txidsExcluded, addrsExcluded);
            
            if (!tagsExcluded.empty())
            {
                stmt.Bind(tagsExcluded);

                if (!lang.empty())
                    stmt.Bind(lang);
            }

            // ---------------------------------------------

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
        });

        // ---------------------------------------------
        // Calculate content ratings
        int nElements = postsRanks.size();
        for (auto& iPostRank : postsRanks)
        {
            double _LAST5R = 0;
            double _UREPR = 0;
            double _PREPR = 0;

            double boost = 0;
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

            iPostRank.POSTRF = 0.4 * (0.75 * (iPostRank.LAST5R + boost) + 0.25 * iPostRank.UREPR) * iPostRank.DREP + 0.6 * iPostRank.PREPR * iPostRank.DPOST;
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
            UniValue histContents = GetHistoricalFeed(lack, minPostRank, topHeight, lang, tags, contentTypes,
                txidsExcluded, addrsExcluded, tagsExcluded, address, badReputationLimit);

            result.push_backV(histContents.getValues());
        }

        // Complete!
        return result;
    }

    // TODO (aok, api) : optimization
    UniValue WebRpcRepository::GetBoostFeed(int topHeight,
        const string& lang, const vector<string>& tags, const vector<int>& contentTypes,
        const vector<string>& txidsExcluded, const vector<string>& addrsExcluded, const vector<string>& tagsExcluded,
        int badReputationLimit)
    {
        auto func = __func__;
        UniValue result(UniValue::VARR);

        // --------------------------------------------

        string contentTypesWhere = " ( " + join(vector<string>(contentTypes.size(), "?"), ",") + " ) ";

        string langFilter;
        if (!lang.empty())
            langFilter += " join Payload p on p.TxId = tc.RowId and p.String1 = ? ";

        string sql = R"sql(
            select
                ctc.Uid contentId,
                (select String from Registry where RowId = tb.RegId2) contentHash,
                sum(tb.Int1) as sumBoost

            from Transactions tb indexed by Transactions_Type_RegId2_RegId1
            join Chain ctb on ctb.TxId = tb.RowId
            join Last ltb on ltb.TxId = tb.RowId
            join Transactions tc indexed by Transactions_Type_RegId2_RegId1
                on tc.RegId2 = tb.RegId2 and tc.Type in )sql" + contentTypesWhere + R"sql(
            join Chain ctc on ctc.TxId = tc.RowId
            join Last ltc on ltc.TxId = tc.RowId
            )sql" + langFilter + R"sql(

            join Transactions u indexed by Transactions_Type_RegId1_RegId2_RegId3
                on u.Type in (100) and u.RegId1 = tc.RegId1
            join Chain cu on cu.TxId = u.RowId
            join Last lu on lu.TxId = u.RowId

            left join Ratings ur indexed by Ratings_Type_Uid_Last_Value
                on ur.Type=0 and ur.Uid=cu.Uid and ur.Last=1

            where tb.Type = 208
                and ctb.Height <= ?
                and ctb.Height > ?

                -- Do not show posts from users with low reputation
                and ifnull(ur.Value,0) > ?

                )sql";

        if (!tags.empty())
        {
            sql += R"sql(
                and ctc.Uid in (
                    select tm.ContentId
                    from web.Tags tag
                    join web.TagsMap tm indexed by TagsMap_TagId_ContentId
                        on tag.Id = tm.TagId
                    where tag.Value in ( )sql" + join(vector<string>(tags.size(), "?"), ",") + R"sql( )
                        )sql" + (!lang.empty() ? " and tag.Lang = ? " : "") + R"sql(
                )
            )sql";
        }

        if (!txidsExcluded.empty()) sql += " and t.RegId2 not in ( select RowId from Registry where String in ( " + join(vector<string>(txidsExcluded.size(), "?"), ",") + " ) ) ";
        if (!addrsExcluded.empty()) sql += " and t.RegId1 not in ( select RowId from Registry where String in ( " + join(vector<string>(addrsExcluded.size(), "?"), ",") + " ) ) ";
        if (!tagsExcluded.empty())
        {
            sql += R"sql( and ctc.Uid not in (
                select tmEx.ContentId
                from web.Tags tagEx
                join web.TagsMap tmEx indexed by TagsMap_TagId_ContentId
                    on tagEx.Id=tmEx.TagId
                where tagEx.Value in ( )sql" + join(vector<string>(tagsExcluded.size(), "?"), ",") + R"sql( )
                    )sql" + (!lang.empty() ? " and tagEx.Lang = ? " : "") + R"sql(
             ) )sql";
        }

        sql += " group by ctc.Uid, tb.RegId2";
        sql += " order by sum(tb.Int1) desc";

        // ---------------------------------------------

        vector<int64_t> ids;

        SqlTransaction(func, [&]()
        {
            auto& stmt = Sql(sql);

            stmt.Bind(contentTypes);

            if (!lang.empty()) stmt.Bind(lang);

            stmt.Bind(topHeight, topHeight - cntBlocksForResult, badReputationLimit);

            if (!tags.empty())
            {
                stmt.Bind(tags);

                if (!lang.empty())
                    stmt.Bind(lang);
            }

            stmt.Bind(txidsExcluded, addrsExcluded);

            if (!tagsExcluded.empty())
            {
                stmt.Bind(tagsExcluded);

                if (!lang.empty())
                    stmt.Bind(lang);
            }

            // ---------------------------------------------

            stmt.Select([&](Cursor& cursor) {
                while (cursor.Step())
                {
                    int64_t contentId, sumBoost;
                    std::string contentHash;
                    cursor.CollectAll(contentId, contentHash, sumBoost);

                    UniValue boost(UniValue::VOBJ);
                    boost.pushKV("id", contentId);
                    boost.pushKV("txid", contentHash);
                    boost.pushKV("boost", sumBoost);
                    result.push_back(boost);
                }
            });
        });

        // Complete!
        return result;
    }

    // TODO (aok, api): implement
    UniValue WebRpcRepository::GetProfileCollections(const string& addressFeed, int countOut, int pageNumber, const int64_t& topContentId, int topHeight,
                                   const string& lang, const vector<string>& tags, const vector<int>& contentTypes,
                                   const vector<string>& txidsExcluded, const vector<string>& addrsExcluded, const vector<string>& tagsExcluded,
                                   const string& address, const string& keyword, const string& orderby, const string& ascdesc)
    {
        auto func = __func__;
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
        SqlTransaction(func, [&]()
        {
            auto& stmt = Sql(sql);
            
            if (!lang.empty()) stmt.Bind(lang);
            stmt.Bind(topHeight, addressFeed);
            if (topContentId > 0)
                stmt.Bind(topContentId);
            stmt.Bind(countOut, pageNumber * countOut);

            stmt.Select([&](Cursor& cursor) {
                while (cursor.Step()) {
                    if (auto[ok, value] = cursor.TryGetColumnInt64(0); ok)
                        ids.push_back(value);
                }
            });
        });

        // Get content data
        if (!ids.empty())
        {
            auto contents = GetCollectionsData(ids);
            result.push_backV(contents);
        }

        // Complete!
        return result;
    }

    UniValue WebRpcRepository::GetsubsciptionsGroupedByAuthors(const string& address, const string& addressPagination, int nHeight, int countOutOfUsers, int countOutOfcontents, int badReputationLimit)
    {
        UniValue result(UniValue::VARR);

        SqlTransaction(__func__, [&]()
        {
            Sql(R"sql(
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
                                (select r.String from Registry r where r.RowId = _c.HashId)
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
            )
            .Select([&](Cursor& cursor) {
                while (cursor.Step())
                {
                    UniValue contents(UniValue::VOBJ);

                    cursor.Collect<string>(0, contents, "address");
                    cursor.Collect<string>(1, contents, "txids");

                    result.push_back(contents);
                }
            });
        });

        return result;
    }

    // ------------------------------------------------------

    vector<int64_t> WebRpcRepository::GetRandomContentIds(const string& lang, int count, int height)
    {
        vector<int64_t> result;

        SqlTransaction(__func__, [&]()
        {
            Sql(R"sql(
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
            .Bind(lang, height, count)
            .Select([&](Cursor& cursor) {
                while (cursor.Step())
                {
                    if (auto[ok, value] = cursor.TryGetColumnInt64(0); ok)
                        result.push_back(value);
                }
            });
        });

        return result;
    }

    UniValue WebRpcRepository::GetContentActions(const string& postTxHash)
    {
        UniValue result(UniValue::VOBJ);
        UniValue resultScores(UniValue::VARR);
        UniValue resultBoosts(UniValue::VARR);
        UniValue resultDonations(UniValue::VARR);

        SqlTransaction(__func__, [&]()
        {
            Sql(R"sql(
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
                            sum(b.Int1) as sumBoost
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
                            TxOutputs o indexed by TxOutputs_AddressId_TxId_Number on
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
            .Bind(postTxHash)
            .Select([&](Cursor& cursor) {
                while (cursor.Step())
                {
                    UniValue record(UniValue::VOBJ);
                    if (auto[ok, value] = cursor.TryGetColumnString(0); ok) record.pushKV("posttxid", value);
                    if (auto[ok, value] = cursor.TryGetColumnString(1); ok) record.pushKV("address", value);
                    if (auto[ok, value] = cursor.TryGetColumnString(2); ok) record.pushKV("name", value);
                    if (auto[ok, value] = cursor.TryGetColumnString(3); ok) record.pushKV("avatar", value);
                    if (auto[ok, value] = cursor.TryGetColumnString(4); ok) record.pushKV("reputation", value);
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
        });

        result.pushKV("scores",resultScores);
        result.pushKV("boosts",resultBoosts);
        result.pushKV("donations",resultDonations);

        return result;
    };

    // Choosing predicate for function above based on filters.
    std::function<bool(const ShortTxType&)> _choosePredicate(const std::set<ShortTxType>& filters) {
        if (filters.empty()) {
            // No filters mean that we should perform all selects
            return [&filters](...) { return true; };
        } else {
            // Perform only selects that are specified in filters.
            return [&filters](const ShortTxType& select) { return filters.find(select) != filters.end(); };
        }
    };

    // Method used to construct sql query and required bindings from provided selects based on filters
    template <class QueryParams>
    static inline auto _constructSelectsBasedOnFilters(
                const std::set<ShortTxType>& filters,
                const std::map<ShortTxType, ShortFormSqlEntry<Stmt&, QueryParams>>& selects,
                const std::string& header,
                const std::string& footer, const std::string& separator = "union")
    {
        auto predicate = _choosePredicate(filters);

        // Binds that should be performed to constructed query
        std::vector<std::function<void(Stmt&, QueryParams const&)>> binds;
        // Query elemets that will be used to construct full query
        std::vector<std::string> queryElems;
        for (const auto& select: selects) {
            if (predicate(select.first)) {
                queryElems.emplace_back(select.second.query);
                queryElems.emplace_back(separator);
                binds.emplace_back(select.second.binding);
            }
        }

        if (queryElems.empty()) {
            throw std::runtime_error("Failed to construct query for requested filters");
        }
        queryElems.pop_back(); // Dropping last "union"

        std::stringstream ss;
        ss << header;
        for (const auto& elem: queryElems) {
            ss << elem;
        }
        ss << footer;

        // Full query and required binds in correct order
        return std::pair { ss.str(), binds };
    }

    std::vector<ShortForm> WebRpcRepository::GetActivities(const std::string& address, int64_t heightMax, int64_t heightMin, int64_t blockNumMax, const std::set<ShortTxType>& filters)
    {
        // This is required because we want static bind functors for optimization so parameters can't be captured there
        struct QueryParams {
            // Handling all by reference
            const int64_t& heightMax;
            const int64_t& heightMin;
            const int64_t& blockNumMax;
        } queryParams{heightMax, heightMin, blockNumMax};

        static const auto binding = [](Stmt& stmt, QueryParams const& params) {
            stmt.Bind(
                params.heightMin,
                params.heightMax,
                params.heightMax,
                params.blockNumMax
            );
        };

        static const std::map<ShortTxType, ShortFormSqlEntry<Stmt&, QueryParams>> selects = {
        {
            ShortTxType::Answer, { R"sql(
            -- My answers to other's comments
            select
                (')sql" + ShortTxTypeConvertor::toString(ShortTxType::Answer) + R"sql(')TP,
                sa.Hash,
                a.Type,
                null,
                ca.Height as Height,
                ca.BlockNum as BlockNum,
                a.Time,
                sa.String2,
                sa.String3,
                null,
                null,
                null,
                pa.String1,
                sa.String4,
                sa.String5,
                null,
                null,
                null,
                null,
                null,
                sc.Hash,
                c.Type,
                sc.String1,
                cc.Height,
                cc.BlockNum,
                c.Time,
                sc.String2,
                null,
                null,
                (
                    select
                        json_group_array(
                            json_object(
                                'Value', o.Value,
                                'Number', o.Number,
                                'AddressHash', (select r.String from Registry r where r.RowId = o.AddressId),
                                'ScriptPubKey', (select r.String from Registry r where r.RowId = o.ScriptPubKeyId)
                            )
                        )

                    from
                        TxInputs i

                        join TxOutputs o on
                            o.TxId = i.TxId and
                            o.Number = i.Number

                    where
                        i.SpentTxId = c.RowId
                ),
                (
                    select
                        json_group_array(
                            json_object(
                                'Value', o.Value,
                                'AddressHash', (select r.String from Registry r where r.RowId = o.AddressId),
                                'ScriptPubKey', (select r.String from Registry r where r.RowId = o.ScriptPubKeyId)
                            )
                        )

                    from
                        TxOutputs o

                    where
                        o.TxId = c.RowId
                    order by
                        o.Number
                ),
                pc.String1,
                sc.String4,
                sc.String5,
                null, -- Badge
                pac.String2,
                pac.String3,
                ifnull(rca.Value,0),
                null

            from
                address,
                Transactions c indexed by Transactions_Type_RegId1_RegId2_RegId3 -- My comments

                join Chain cc on
                    cc.TxId = c.RowId

                join vTxStr sc on
                    sc.RowId = c.RowId

                left join Payload pc on
                    pc.TxId = c.RowId

                join Transactions a indexed by Transactions_Type_RegId5_RegId1 on -- Other answers
                    a.Type in (204, 205, 206) and
                    a.RegId5 = c.RegId2 and
                    a.RegId1 != c.RegId1 and
                    a.RegId1 = address.id

                join vTxStr sa on
                    sa.RowId = a.RowId

                join Chain ca on
                    ca.TxId = a.RowId and
                    ca.Height > ? and
                    (ca.Height < ? or (ca.Height = ? and ca.BlockNum < ?))

                left join Payload pa on
                    pa.TxId = a.RowId

                left join Transactions ac indexed by Transactions_Type_RegId1_RegId2_RegId3 on
                    ac.Type = 100 and
                    ac.RegId1 = c.RegId1 and
                    exists (select 1 from Last l where l.TxId = ac.RowId)

                left join Payload pac on
                    pac.TxId = ac.RowId

                left join Ratings rca indexed by Ratings_Type_Uid_Last_Height on
                    rca.Type = 0 and
                    rca.Uid = ca.Uid and
                    rca.Last = 1

            where
                c.Type in (204, 205) and
                exists (select 1 from Last l where l.TxId = c.RowId)

        )sql",
            binding 
        }},

        {
            ShortTxType::Comment, { R"sql(
            -- Comments for my content
            select
                (')sql" + ShortTxTypeConvertor::toString(ShortTxType::Comment) + R"sql(')TP,
                sc.Hash,
                c.Type,
                null,
                cc.Height as Height,
                cc.BlockNum as BlockNum,
                c.Time,
                sc.String2,
                sc.String3,
                null,
                (
                    select
                        json_group_array(
                            json_object(
                                'Value', o.Value,
                                'Number', o.Number,
                                'AddressHash', (select r.String from Registry r where r.RowId = o.AddressId),
                                'ScriptPubKey', (select r.String from Registry r where r.RowId = o.ScriptPubKeyId)
                            )
                        )

                    from
                        TxInputs i

                        join TxOutputs o on
                            o.TxId = i.TxId and
                            o.Number = i.Number

                    where
                        i.SpentTxId = c.RowId
                ),
                (
                    select
                        json_group_array(
                            json_object(
                                'Value', o.Value,
                                'AddressHash', (select r.String from Registry r where r.RowId = o.AddressId),
                                'ScriptPubKey', (select r.String from Registry r where r.RowId = o.ScriptPubKeyId)
                            )
                        )

                    from
                        TxOutputs o

                    where
                        o.TxId = c.RowId
                    order by
                        o.Number
                ),
                pc.String1,
                sc.String4,
                sc.String5,
                null,
                null,
                null,
                null,
                null,
                sp.Hash,
                p.Type,
                sp.String1,
                cp.Height,
                cp.BlockNum,
                p.Time,
                sp.String2,
                null,
                null,
                null,
                null,
                pp.String2,
                null,
                null,
                null,
                pap.String2,
                pap.String3,
                ifnull(rap.Value, 0),
                null

            from
                address,
                Transactions p indexed by Transactions_Type_RegId2_RegId1 -- TODO (optimization): use Transactions_Type_RegId2_RegId1

                join Chain cp on
                    cp.TxId = p.RowId

                join vTxStr sp on
                    sp.RowId = p.RowId

                join Transactions c indexed by Transactions_Type_RegId5_RegId1 on -- TODO (optimization): use Transactions_Type_RegId3_RegId4_RegId5_RegId1
                    c.Type in (204, 205, 206) and
                    c.RegId3 = p.RegId2 and
                    c.RegId1 != p.RegId1 and
                    c.RegId4 is null and
                    c.RegId5 is null and
                    c.RegId1 = address.id

                join vTxStr sc on
                    sc.RowId = c.RowId

                join Chain cc on
                    cc.TxId = c.RowId and
                    cc.Height > ? and
                    (cc.Height < ? or (cc.Height = ? and cc.BlockNum < ?))

                left join Payload pc on
                    pc.TxId = c.RowId

                left join Payload pp on
                    pp.TxId = p.RowId

                left join Transactions ap indexed by Transactions_Type_RegId1_RegId2_RegId3 on -- accounts of commentators
                    ap.Type = 100 and
                    ap.RegId1 = p.RegId1 and
                    exists (select 1 from Last l where l.TxId = ap.RowId)

                left join Chain cap on
                    cap.TxId = ap.RowId

                left join Payload pap on
                    pap.TxId = ap.RowId

                left join Ratings rap indexed by Ratings_Type_Uid_Last_Height on
                    rap.Type = 0 and
                    rap.Uid = cap.Uid and
                    rap.Last = 1

            where
                p.Type in (200,201,202,209,210) and
                exists (select 1 from Last l where l.TxId = p.RowId)

        )sql",
            binding
        }},

        {
            ShortTxType::Subscriber, { R"sql(
            -- Subscribers
            select
                (')sql" + ShortTxTypeConvertor::toString(ShortTxType::Subscriber) + R"sql(')TP,
                ssubs.Hash,
                subs.Type,
                null,
                csubs.Height as Height,
                csubs.BlockNum as BlockNum,
                subs.Time,
                null,
                null,
                null,
                null,
                null,
                null,
                null,
                null,
                null,
                null,
                null,
                null,
                null,
                su.Hash,
                u.Type,
                su.String1,
                cu.Height,
                cu.BlockNum,
                u.Time,
                null,
                null,
                null,
                null,
                null,
                null,
                null,
                null,
                null,
                pu.String2,
                pu.String3,
                ifnull(ru.Value,0),
                null

            from
                address,
                Transactions subs indexed by Transactions_Type_RegId1_RegId2_RegId3

                join Chain csubs on
                    csubs.TxId = subs.RowId and
                    csubs.Height > ? and
                    (csubs.Height < ? or (csubs.Height = ? and csubs.BlockNum < ?))

                join vTxStr ssubs on
                    ssubs.RowId = subs.RowId

                join Transactions u indexed by Transactions_Type_RegId1_RegId2_RegId3 on
                    u.Type in (100) and
                    u.RegId1 = subs.RegId2 and
                    exists (select 1 from Last l where l.TxId = u.RowId)

                join Chain cu on
                    cu.TxId = u.RowId

                join vTxStr su on
                    su.RowId = u.RowId

                left join Payload pu on
                    pu.TxId = u.RowId

                left join Ratings ru indexed by Ratings_Type_Uid_Last_Height on
                    ru.Type = 0 and
                    ru.Uid = cu.Uid and
                    ru.Last = 1

            where
                subs.Type in (302, 303, 304) and
                subs.RegId1 = address.id

        )sql",
            binding
        }},

        {
            ShortTxType::CommentScore, { R"sql(
            -- Comment scores
            select
                (')sql" + ShortTxTypeConvertor::toString(ShortTxType::CommentScore) + R"sql(')TP,
                ss.Hash,
                s.Type,
                null,
                cs.Height as Height,
                cs.BlockNum as BlockNum,
                s.Time,
                null,
                null,
                s.Int1,
                null,
                null,
                null,
                null,
                null,
                null,
                null,
                null,
                null,
                null,
                sc.Hash,
                c.Type,
                sc.String1,
                cc.Height,
                cc.BlockNum,
                c.Time,
                sc.String2,
                sc.String3,
                null,
                (
                    select
                        json_group_array(
                            json_object(
                                'Value', o.Value,
                                'Number', o.Number,
                                'AddressHash', (select r.String from Registry r where r.RowId = o.AddressId),
                                'ScriptPubKey', (select r.String from Registry r where r.RowId = o.ScriptPubKeyId)
                            )
                        )

                    from
                        TxInputs i

                        join TxOutputs o on
                            o.TxId = i.TxId and
                            o.Number = i.Number

                    where
                        i.SpentTxId = c.RowId
                ),
                (
                    select
                        json_group_array(
                            json_object(
                                'Value', o.Value,
                                'AddressHash', (select r.String from Registry r where r.RowId = o.AddressId),
                                'ScriptPubKey', (select r.String from Registry r where r.RowId = o.ScriptPubKeyId)
                            )
                        )

                    from
                        TxOutputs o

                    where
                        o.TxId = c.RowId
                    order by
                        o.Number
                ),
                pc.String1,
                sc.String4,
                sc.String5,
                null,
                pac.String2,
                pac.String3,
                ifnull(rac.Value,0),
                null

            from
                address,
                Transactions c indexed by Transactions_Type_RegId5_RegId1 -- TODO (optimization): only (Type)

                join Chain cc on
                    cc.TxId = c.RowId

                join vTxStr sc on
                    sc.RowId = c.RowId

                left join Payload pc on
                    pc.TxId = c.RowId

                join Transactions s indexed by Transactions_Type_RegId1_RegId2_RegId3 on
                    s.Type = 301 and
                    s.RegId2 = c.RegId2 and
                    s.RegId1 = address.id and
                    exists (select 1 from Last l where l.TxId = s.RowId)

                join Chain cs indexed by Chain_Height_Uid on
                    cs.Height > ? and
                    (cs.Height < ? or (cs.Height = ? and cs.BlockNum < ?))

                join vTxStr ss on
                    ss.RowId = s.RowId

                left join Transactions ac indexed by Transactions_Type_RegId1_RegId2_RegId3 on
                    ac.Type = 100 and
                    ac.RegId1 = c.RegId1 and
                    exists (select 1 from Last l where l.TxId = ac.RowId)

                left join Chain cac on
                    cac.TxId = ac.RowId

                left join Payload pac on
                    pac.TxId = ac.RowId

                left join Ratings rac indexed by Ratings_Type_Uid_Last_Height on
                    rac.Type = 0 and
                    rac.Uid = cac.Uid and
                    rac.Last = 1

            where
                c.Type in (204,205) and
                exists (select 1 from Last l where l.TxId = c.RowId)

        )sql",
            binding
        }},

        {
            ShortTxType::ContentScore, { R"sql(
            -- Content scores
            select
                (')sql" + ShortTxTypeConvertor::toString(ShortTxType::ContentScore) + R"sql(')TP,
                ss.Hash,
                s.Type,
                null,
                cs.Height as Height,
                cs.BlockNum as BlockNum,
                s.Time,
                null,
                null,
                s.Int1,
                null,
                null,
                null,
                null,
                null,
                null,
                null,
                null,
                null,
                null,
                sc.Hash,
                c.Type,
                sc.String1,
                cc.Height,
                cc.BlockNum,
                c.Time,
                sc.String2,
                null,
                null,
                null,
                null,
                pc.String2,
                null,
                null,
                null,
                pac.String2,
                pac.String3,
                ifnull(rac.Value,0),
                null

            from
                address,
                Transactions c indexed by Transactions_Type_RegId5_RegId1 -- TODO (optimization): only (Type)

                join Chain cc on
                    cc.TxId = c.RowId

                join vTxStr sc on
                    sc.RowId = c.RowId

                left join Payload pc on
                    pc.TxId = c.RowId

                join Transactions s indexed by Transactions_Type_RegId1_RegId2_RegId3 on
                    s.Type = 300 and
                    s.RegId2 = c.RegId2 and
                    s.RegId1 = address.id and
                    exists (select 1 from Last l where l.TxId = s.RowId)

                join Chain cs indexed by Chain_Height_Uid on
                    cs.Height > ? and
                    (cs.Height < ? or (cs.Height = ? and cs.BlockNum < ?))

                join vTxStr ss on
                    ss.RowId = s.RowId

                left join Transactions ac indexed by Transactions_Type_RegId1_RegId2_RegId3 on
                    ac.Type = 100 and
                    ac.RegId1 = c.RegId1 and
                    exists (select 1 from Last l where l.TxId = ac.RowId)

                left join Chain cac on
                    cac.TxId = ac.RowId

                left join Payload pac on
                    pac.TxId = ac.RowId

                left join Ratings rac indexed by Ratings_Type_Uid_Last_Height on
                    rac.Type = 0 and
                    rac.Uid = cac.Uid and
                    rac.Last = 1

            where
                c.Type in (200,201,202,209,210) and
                exists (select 1 from Last l where l.TxId = c.RowId)

        )sql",
            binding
        }},

        {
            ShortTxType::Boost, { R"sql(
            -- Boosts for my content
            select
                (')sql" + ShortTxTypeConvertor::toString(ShortTxType::Boost) + R"sql(')TP,
                sBoost.Hash,
                tboost.Type,
                null,
                cBoost.Height as Height,
                cBoost.BlockNum as BlockNum,
                tBoost.Time,
                null,
                null,
                null,
                (
                    select
                        json_group_array(
                            json_object(
                                'Value', o.Value,
                                'Number', o.Number,
                                'AddressHash', (select r.String from Registry r where r.RowId = o.AddressId),
                                'ScriptPubKey', (select r.String from Registry r where r.RowId = o.ScriptPubKeyId)
                            )
                        )

                    from
                        TxInputs i

                        join TxOutputs o on
                            o.TxId = i.TxId and
                            o.Number = i.Number

                    where
                        i.SpentTxId = tBoost.RowId
                ),
                (
                    select
                        json_group_array(
                            json_object(
                                'Value', o.Value,
                                'AddressHash', (select r.String from Registry r where r.RowId = o.AddressId),
                                'ScriptPubKey', (select r.String from Registry r where r.RowId = o.ScriptPubKeyId)
                            )
                        )

                    from
                        TxOutputs o

                    where
                        o.TxId = tBoost.RowId
                    order by
                        o.Number
                ),
                null,
                null,
                null,
                null,
                null,
                null,
                null,
                null,
                sContent.Hash,
                tContent.Type,
                sContent.String1,
                cContent.Height,
                cContent.BlockNum,
                tContent.Time,
                sContent.String2,
                null,
                null,
                null,
                null,
                pContent.String2,
                null,
                null,
                null,
                pac.String2,
                pac.String3,
                ifnull(rac.Value,0),
                null

            from
                Transactions tBoost indexed by Transactions_Type_RegId1_RegId2_RegId3

                join Chain cBoost indexed by Chain_Height_Uid on
                    cBoost.TxId = tBoost.RowId and
                    cBoost.Height > ? and
                    (cBoost.Height < ? or (cBoost.Height = ? and cBoost.BlockNum < ?))
    
                join vTxStr sBoost on
                    sBoost.RowId = tBoost.RowId
    
                join Transactions tContent indexed by Transactions_Type_RegId2_RegId1 on
                    tContent.Type in (200,201,202,209,210) and
                    tContent.RegId2 = tBoost.RegId2 and
                    exists (select 1 from Last l where l.TxId = tContent.RowId)
    
                join Chain cContent on
                    cContent.TxId = tContent.RowId
    
                join vtxStr sContent on
                    sContent.RowId = tContent.RowId
    
                left join Payload pContent on
                    pContent.TxId = tContent.RowId
                
                left join Transactions ac indexed by Transactions_Type_RegId1_RegId2_RegId3 on
                    ac.RegId1 = tContent.RegId1 and
                    ac.Type = 100 and
                    exists (select 1 from Last l where l.TxId = ac.RowId)
    
                left join Chain cac on
                    cac.TxId = ac.RowId
    
                left join Payload pac on
                    pac.TxId = ac.RowId
    
                left join Ratings rac indexed by Ratings_Type_Uid_Last_Height on
                    rac.Type = 0 and
                    rac.Uid = cac.Uid and
                    rac.Last = 1

            where
                tBoost.Type in (208) and 
                tBoost.RegId1 = address.id and
                exists (select 1 from Last l where l.TxId = tBoost.RowId)

        )sql",
            binding
        }},

        {
            ShortTxType::Blocking, { R"sql(
            -- My blockings and unblockings
            select
                ('blocking')TP,
                sb.Hash,
                b.Type,
                sac.String1,
                cb.Height as Height,
                cb.BlockNum as BlockNum,
                b.Time,
                null,
                null,
                null,
                null,
                null,
                null,
                null,
                null,
                null,
                pac.String2,
                pac.String3,
                ifnull(rac.Value,0),
                (
                    select
                        json_group_array(
                            json_object(
                                'address', smac.String1,
                                'account', json_object(
                                    'name', pmac.String2,
                                    'avatar', pmac.String3,
                                    'reputation', ifnull(rmac.Value,0)
                                )
                            )
                        )
                    from
                        Transactions mac indexed by Transactions_Type_RegId1_RegId2_RegId3

                        join vTxStr smac on
                            smac.RowId = mac.RowId

                        join Chain cmac on
                            cmac.TxId = mac.RowId

                        left join Payload pmac on
                            pmac.TxId = mac.RowId

                        left join Ratings rmac indexed by Ratings_Type_Uid_Last_Height on
                            rmac.Type = 0 and
                            rmac.Uid = cmac.Uid and
                            rmac.Last = 1

                    where
                        mac.RegId1 in (select value from json_each(b.RegId3)) and
                        mac.Type = 100 and
                        exists (select 1 from Last l where l.TxId = mac.RowId)
                ),
                null,
                null,
                null,
                null,
                null,
                null,
                null,
                null,
                null,
                null,
                null,
                null,
                null,
                null,
                null,
                null,
                null,
                null,
                null

            from Transactions b indexed by Transactions_Type_RegId1_RegId2_RegId3

            join Chain cb on
                cb.TxId = b.RowId and
                cb.Height > ? and
                (cb.Height < ? or (cb.Height = ? and cb.BlockNum < ?))

            join vTxStr sb on
                sb.RowId = b.RowId

            left join Transactions ac indexed by Transactions_Type_RegId1_RegId2_RegId3 on
                ac.RegId1 = b.RegId2 and
                ac.Type = 100 and
                exists (select 1 from Last l where l.TxId = ac.RowId)

            left join vTxStr sac on
                sac.RowId = ac.RowId

            left join Chain cac on
                cac.TxId = ac.RowId

            left join Payload pac on
                pac.TxId = ac.RowId

            left join Ratings rac indexed by Ratings_Type_Uid_Last_Height on
                rac.Type = 0 and
                rac.Uid = cac.Uid and
                rac.Last = 1

            where
                b.Type in (305,306) and
                b.RegId1 = ?

        )sql",
            binding
        }},
        };

        static const auto header = R"sql(

            -- Common 'with' for all unions
            with
                address as (
                    select
                        r.String as hash,
                        r.RowId as id

                    from
                        Registry r
                    
                    where
                        r.String = ?
                )
        )sql";

        static const auto footer = R"sql(

            -- Global order and limit for pagination
            order by Height desc, BlockNum desc
            limit 10

        )sql";

        auto [elem1, elem2] = _constructSelectsBasedOnFilters(filters, selects, header, footer);
        auto& sql = elem1;
        auto& binds = elem2;

        EventsReconstructor reconstructor;
        SqlTransaction(__func__, [&]()
        {
            auto& stmt = Sql(sql);
            stmt.Bind(address);
            for (const auto& bind: binds) {
                bind(stmt, queryParams);
            }

            stmt.Select([&](Cursor& cursor) {
                while (cursor.Step())
                {
                    reconstructor.FeedRow(cursor);
                }
            });
        });

        return reconstructor.GetResult();
    }

    UniValue WebRpcRepository::GetNotifications(int64_t height, const std::set<ShortTxType>& filters)
    {
        struct QueryParams {
            // Handling all by reference
            const int64_t& height;
        } queryParams {height};

        // Static because it will not be changed for entire node run

        static const auto heightBinder =
            [](Stmt& stmt, QueryParams const& queryParams){
                stmt.Bind(queryParams.height);
            };

        static const std::map<ShortTxType, ShortFormSqlEntry<Stmt&, QueryParams>> selects = {
            {
                ShortTxType::Money, { R"sql(
                    -- Incoming money
                    select
                        null, -- Will be filled
                        null,
                        null,
                        null,
                        null,
                        (')sql" + ShortTxTypeConvertor::toString(ShortTxType::Money) + R"sql(')TP,
                        r.String,
                        t.Type,
                        null,
                        c.Height as Height,
                        c.BlockNum as BlockNum,
                        t.Time,
                        null,
                        null,
                        null,
                        (
                            select json_group_array(json_object(
                                'Value', o.Value,
                                'Number', o.Number,
                                'AddressHash', (select r.String from Registry r where r.RowId = o.AddressId),
                                'ScriptPubKey', (select r.String from Registry r where r.RowId = o.ScriptPubKeyId)
                            ))

                            from
                                TxInputs i indexed by TxInputs_SpentTxId_Number_TxId

                                join TxOutputs o indexed by TxOutputs_TxId_Number_AddressId on
                                    o.TxId = i.TxId and
                                    o.Number = i.Number

                            where
                                i.SpentTxId = t.RowId
                        ),
                        (
                            select json_group_array(json_object(
                                'Value', o.Value,
                                'AddressHash', (select r.String from Registry r where r.RowId = o.AddressId),
                                'ScriptPubKey', (select r.String from Registry r where r.RowId = o.ScriptPubKeyId),
                                'Account', json_object(
                                    'Lang', pna.String1,
                                    'Name', pna.String2,
                                    'Avatar', pna.String3
                                )
                            ))

                            from
                                TxOutputs o indexed by TxOutputs_TxId_Number_AddressId

                                left join Transactions na indexed by Transactions_Type_RegId1_RegId2_RegId3 on
                                    na.Type = 100 and
                                    na.RegId1 = o.AddressId and
                                    exists (select 1 from Last lna where lna.TxId = na.RowId)

                                left join Chain cna on
                                    cna.TxId = na.RowId

                                left join Payload pna on
                                    pna.TxId = na.RowId

                                left join Ratings rna indexed by Ratings_Type_Uid_Last_Height on
                                    rna.Type = 0 and
                                    rna.Uid = cna.Uid and
                                    rna.Last = 1

                            where
                                o.TxId = t.RowId
                            order by
                                o.Number
                        )

                    from
                        Transactions t

                        join Chain c indexed by Chain_TxId_Height on
                            c.TxId = t.RowId and
                            c.Height = ?

                        join Registry r on
                            r.RowId = t.HashId

                    where
                        t.Type in (1,2,3) -- 1 default money transfer, 2 coinbase, 3 coinstake
            )sql",
                heightBinder
            }},

            {
                ShortTxType::Referal, { R"sql(
                    -- referals
                    select
                        st.String2,
                        pna.String1,
                        pna.String2,
                        pna.String3,
                        ifnull(rna.Value,0),
                        (')sql" + ShortTxTypeConvertor::toString(ShortTxType::Referal) + R"sql(')TP,
                        st.Hash,
                        t.Type,
                        st.String1,
                        c.Height as Height,
                        c.BlockNum as BlockNum,
                        t.Time,
                        null,
                        null,
                        null,
                        null,
                        null,
                        null,
                        null,
                        null,
                        p.String1,
                        p.String2,
                        p.String3,
                        ifnull(r.Value,0) -- TODO (losty): do we need rating if referal is always a new user?

                    from Transactions t indexed by Transactions_Type_RegId1_RegId2_RegId3 -- TODO (losty): not covering index

                    cross join vTxStr st on
                        st.RowId = t.RowId

                    join Chain c on
                        c.TxId = t.RowId and
                        c.Height = ?

                    left join Payload p on
                        p.TxId = t.RowId

                    left join Ratings r indexed by Ratings_Type_Uid_Last_Height on
                        r.Type = 0 and
                        r.Uid = c.Uid and
                        r.Last = 1

                    join Transactions na indexed by Transactions_Type_RegId1_RegId2_RegId3 on
                        na.Type = 100 and
                        na.RegId1 = t.RegId2
                        and exists (select 1 from Last lna where lna.TxId = na.RowId)

                    join Chain cna on
                        cna.TxId = na.RowId

                    left join Payload pna on
                        pna.TxId = na.RowId

                    left join Ratings rna indexed by Ratings_Type_Uid_Last_Height on
                        rna.Type = 0 and
                        rna.Uid = cna.Uid and
                        rna.Last = 1

                    where
                        t.Type = 100 and
                        t.RegId2 > 0 and
                        exists (select 1 from First f where f.TxId = t.RowId) -- Only original;
            )sql",
                heightBinder
            }},

            {
                ShortTxType::Answer, { R"sql(
                    -- Comment answers
                    select
                        sc.String1,
                        pna.String1,
                        pna.String2,
                        pna.String3,
                        ifnull(rna.Value,0),
                        (')sql" + ShortTxTypeConvertor::toString(ShortTxType::Answer) + R"sql(')TP,
                        sa.Hash,
                        a.Type,
                        sa.String1,
                        ca.Height as Height,
                        ca.BlockNum as BlockNum,
                        a.Time,
                        sa.String2,
                        sa.String3,
                        null,
                        null,
                        null,
                        pa.String1,
                        sa.String4,
                        sa.String5,
                        paa.String1,
                        paa.String2,
                        paa.String3,
                        ifnull(ra.Value,0),
                        null,
                        spost.Hash,
                        post.Type,
                        spost.String1,
                        cpost.Height,
                        cpost.BlockNum,
                        post.Time,
                        spost.String2,
                        null,
                        null,
                        null,
                        null,
                        null,
                        null,
                        ppost.String2,
                        papost.String1,
                        papost.String2,
                        papost.String3,
                        ifnull(rapost.Value,0)

                    -- TODO (optimization): a lot missing indices
                    from Transactions a -- Other answers
                    cross join vTxStr sa on
                        sa.RowId = a.RowId

                    join Transactions c indexed by Transactions_Type_RegId2_RegId1 on -- My comments
                        c.Type in (204, 205) and
                        c.RegId2 = a.RegId5 and
                        c.RegId1 != a.RegId1 and
                        exists (select 1 from Last lc where lc.TxId = c.RowId)

                    cross join vTxStr sc on
                        sc.RowId = c.RowId

                    join Chain ca indexed by Chain_TxId_Height on
                        ca.TxId = a.RowId and
                        ca.Height = ?

                    left join Transactions post on
                        post.Type in (200,201,202,209,210) and
                        post.RegId2 = a.RegId3 and
                        exists (select 1 from Last lpost where lpost.TxId = post.RowId)
                    left join vTxStr spost on
                        spost.RowId = post.RowId

                    left join Chain cpost on
                        cpost.TxId = post.RowId

                    left join Payload ppost on
                        ppost.TxId = post.RowId

                    left join Transactions apost on
                        apost.Type = 100 and
                        apost.RegId1 = post.RegId1 and
                        exists (select 1 from Last lapost where lapost.TxId = apost.RowId)

                    left join Chain capost on
                        capost.TxId = apost.RowId

                    left join Payload papost on
                        papost.TxId = apost.RowId

                    left join Ratings rapost indexed by Ratings_Type_Uid_Last_Height on
                        rapost.Type = 0 and
                        rapost.Uid = capost.Uid and
                        rapost.Last = 1

                    left join Payload pa on
                        pa.TxId = a.RowId

                    left join Transactions aa on
                        aa.Type = 100 and
                        aa.RegId1 = a.RegId1 and
                        exists (select 1 from Last laa where laa.TxId = aa.RowId)

                    left join Chain caa on
                        caa.TxId = aa.RowId

                    left join Payload paa on
                        paa.TxId = aa.RowId

                    left join Ratings ra indexed by Ratings_Type_Uid_Last_Height on
                        ra.Type = 0 and
                        ra.Uid = caa.Uid and
                        ra.Last = 1

                    left join Transactions na on
                        na.Type = 100 and
                        na.RegId1 = c.RegId1 and
                        exists (select 1 from Last lna where lna.TxId = na.RowId)

                    left join Chain cna on
                        cna.TxId = na.RowId

                    left join Payload pna on
                        pna.TxId = na.RowId

                    left join Ratings rna indexed by Ratings_Type_Uid_Last_Height on
                        rna.Type = 0 and
                        rna.Uid = cna.Uid and
                        rna.Last = 1

                    where
                        a.Type = 204 -- only orig
            )sql",
                heightBinder
            }},

            {
                ShortTxType::Comment, { R"sql(
                    -- Comments for my content
                    select
                        sp.String1,
                        pna.String1,
                        pna.String2,
                        pna.String3,
                        ifnull(rna.Value,0),
                        (')sql" + ShortTxTypeConvertor::toString(ShortTxType::Comment) + R"sql(')TP,
                        sc.Hash,
                        c.Type,
                        sc.String1,
                        cc.Height as Height,
                        cc.BlockNum as BlockNum,
                        c.Time,
                        sc.String2,
                        sc.String3,
                        null,
                        (
                            select json_group_array(json_object(
                                'Value', o.Value,
                                'Number', o.Number,
                                'AddressHash', (select r.String from Registry r where r.RowId = o.AddressId),
                                'ScriptPubKey', (select r.String from Registry r where r.RowId = o.ScriptPubKeyId)
                            ))

                            from
                                TxInputs i indexed by TxInputs_SpentTxId_Number_TxId

                                join TxOutputs o indexed by TxOutputs_TxId_Number_AddressId on
                                    o.TxId = i.TxId and
                                    o.Number = i.Number

                            where
                                i.SpentTxId = c.RowId
                        ),
                        (
                            select json_group_array(json_object(
                                'Value', o.Value,
                                'AddressHash', so.AddressHash,
                                'ScriptPubKey', so.ScriptPubKey
                            ))

                            from TxOutputs o indexed by TxOutputs_TxId_Number_AddressId

                            where
                                o.TxId = c.RowId
                            order by
                                o.Number
                        ),
                        pc.String1,
                        null,
                        null,
                        pac.String1,
                        pac.String2,
                        pac.String3,
                        ifnull(rac.Value,0),
                        null,
                        sp.Hash,
                        p.Type,
                        null,
                        cp.Height,
                        cp.BlockNum,
                        p.Time,
                        sp.String2,
                        null,
                        null,
                        null,
                        null,
                        pp.String2

                    from Transactions c

                    join Chain cc indexed by Chain_TxId_Height on
                        cc.TxId = c.RowId and
                        cc.Height = ?

                    join vTxStr sc on
                        sc.RowId = c.RowId

                    join Transactions p on
                        p.Type in (200,201,202,209,210) and
                        p.RegId2 = c.RegId3 and
                        p.RegId1 != c.RegId1 and
                        exists (select 1 from Last lp where lp.TxId = p.RowId)

                    join vTxStr sp on
                        sp.RowId = p.RowId

                    join Chain cp on
                        cp.TxId = p.RowId

                    left join Payload pc on
                        pC.TxId = c.RowId

                    left join Transactions ac on
                        ac.RegId1 = c.RegId1 and
                        ac.Type = 100 and
                        exists (select 1 from Last lac where lac.TxId = ac.RowId)

                    left join Chain cac on
                        cac.TxId = ac.RowId

                    left join Payload pac on
                        pac.TxId = ac.RowId

                    left join Ratings rac indexed by Ratings_Type_Uid_Last_Height on
                        rac.Type = 0 and
                        rac.Uid = cac.Uid and
                        rac.Last = 1

                    left join Payload pp on
                        pp.TxId = p.RowId

                    left join Transactions na on
                        na.Type = 100 and
                        na.RegId1 = p.RegId1 and
                        exists (select 1 from Last lna where lna.TxId = na.RowId)

                    left join Chain cna on
                        cna.TxId = na.RowId

                    left join Payload pna on
                        pna.TxId = na.RowId

                    left join Ratings rna indexed by Ratings_Type_Uid_Last_Height on
                        rna.Type = 0 and
                        rna.Uid = cna.Uid and
                        rna.Last = 1

                    where 
                        c.Type = 204 and -- only orig
                        c.RegId4 is null and
                        c.RegId5 is null
            )sql",
                heightBinder
            }},

            {
                ShortTxType::Subscriber, { R"sql(
                    -- Subscribers
                    select
                        s.String2,
                        pna.String1,
                        pna.String2,
                        pna.String3,
                        ifnull(rna.Value,0),
                        (')sql" + ShortTxTypeConvertor::toString(ShortTxType::Subscriber) + R"sql(')TP,
                        s.Hash,
                        subs.Type,
                        s.String1,
                        c.Height as Height,
                        c.BlockNum as BlockNum,
                        subs.Time,
                        null,
                        null,
                        null,
                        null,
                        null,
                        null,
                        null,
                        null,
                        pu.String1,
                        pu.String2,
                        pu.String3,
                        ifnull(ru.Value,0)

                    from Transactions subs

                    cross join vTxStr s on
                        s.RowId = subs.RowId

                    join Chain c indexed by Chain_TxId_Height on
                        c.TxId = subs.RowId and
                        c.Height = ?

                    left join Transactions u on
                        u.Type in (100) and
                        u.RegId1 = subs.RegId1 and
                        exists (select 1 from Last lu where lu.TxId = u.RowId)

                    left join Chain cu on
                        cu.TxId = u.RowId

                    left join Payload pu on
                        pu.TxId = u.RowId

                    left join Ratings ru indexed by Ratings_Type_Uid_Last_Height on
                        ru.Type = 0 and
                        ru.Uid = cu.Uid and
                        ru.Last = 1

                    left join Transactions na on
                        na.Type = 100 and
                        na.RegId1 = subs.RegId2 and
                        exists (select 1 from Last lna where lna.TxId = na.RowId)

                    left join Chain cna on
                        cna.TxId = na.RowId

                    left join Payload pna on
                        pna.TxId = na.RowId

                    left join Ratings rna indexed by Ratings_Type_Uid_Last_Height on
                        rna.Type = 0 and
                        rna.Uid = cna.Uid and
                        rna.Last = 1

                    where
                        subs.Type in (302, 303) -- Ignoring unsubscribers?
            )sql",
                heightBinder
            }},

            {
                ShortTxType::CommentScore, { R"sql(
                    -- Comment scores
                    select
                        sc.String1,
                        pna.String1,
                        pna.String2,
                        pna.String3,
                        ifnull(rna.Value,0),
                        (')sql" + ShortTxTypeConvertor::toString(ShortTxType::CommentScore) + R"sql(')TP,
                        ss.Hash,
                        s.Type,
                        ss.String1,
                        cs.Height as Height,
                        cs.BlockNum as BlockNum,
                        s.Time,
                        null,
                        null,
                        s.Int1,
                        null,
                        null,
                        null,
                        null,
                        null,
                        pacs.String1,
                        pacs.String2,
                        pacs.String3,
                        ifnull(racs.Value,0),
                        null,
                        sc.Hash,
                        c.Type,
                        null,
                        cc.Height,
                        cc.BlockNum,
                        null,
                        sc.String2,
                        sc.String3,
                        null,
                        null,
                        null,
                        ps.String1,
                        sc.String4,
                        sc.String5

                    from Transactions s

                    cross join vTxStr ss on
                        ss.RowId = s.RowId

                    join Chain cs indexed by Chain_TxId_Height on
                        cs.TxId = s.RowId
                        and cs.Height = ?

                    join Transactions c indexed by Transactions_Type_RegId2_RegId1 on
                        c.Type in (204,205) and
                        c.RegId2 = s.RegId2 and
                        exists (select 1 from Last lc where lc.TxId = c.RowId)

                    cross join vTxStr sc on
                        sc.RowId = c.RowId

                    join Chain cc on
                        cc.TxId = c.RowId

                    left join Payload ps on
                        ps.TxId = c.RowId

                    join Transactions acs on
                        acs.Type = 100 and
                        acs.RegId1 = s.RegId1 and
                        exists (select 1 from Last lacs where lacs.TxId = acs.RowId)

                    join Chain cacs on
                        cacs.TxId = acs.RowId

                    left join Payload pacs on
                        pacs.TxId = acs.RowId

                    left join Ratings racs indexed by Ratings_Type_Uid_Last_Height on
                        racs.Type = 0 and
                        racs.Uid = cacs.Uid and
                        racs.Last = 1

                    left join Transactions na on
                        na.Type = 100 and
                        na.RegId1 = c.RegId1 and
                        exists (select 1 from Last lna where lna.TxId = na.RowId)

                    left join Chain cna on
                        cna.TxId = na.RowId

                    left join Payload pna on
                        pna.TxId = na.RowId

                    left join Ratings rna indexed by Ratings_Type_Uid_Last_Height on
                        rna.Type = 0 and
                        rna.Uid = cna.Uid and
                        rna.Last = 1

                    where
                        s.Type = 301
            )sql",
                heightBinder
            }},

            {
                ShortTxType::ContentScore, { R"sql(
                    -- Content scores
                    select
                        sc.String1,
                        pna.String1,
                        pna.String2,
                        pna.String3,
                        ifnull(rna.Value,0),
                        (')sql" + ShortTxTypeConvertor::toString(ShortTxType::CommentScore) + R"sql(')TP,
                        ss.Hash,
                        s.Type,
                        ss.String1,
                        cs.Height as Height,
                        cs.BlockNum as BlockNum,
                        s.Time,
                        null,
                        null,
                        s.Int1,
                        null,
                        null,
                        null,
                        null,
                        null,
                        pacs.String1,
                        pacs.String2,
                        pacs.String3,
                        ifnull(racs.Value,0),
                        null,
                        sc.Hash,
                        c.Type,
                        null,
                        cc.Height,
                        cc.BlockNum,
                        null,
                        sc.String2,
                        sc.String3,
                        null,
                        null,
                        null,
                        ps.String1

                    from Transactions s

                    cross join vTxStr ss on
                        ss.RowId = s.RowId

                    join Chain cs indexed by Chain_TxId_Height on
                        cs.TxId = s.RowId
                        and cs.Height = ?

                    join Transactions c indexed by Transactions_Type_RegId2_RegId1 on
                        c.Type in (200,201,202,209,210) and
                        c.RegId2 = s.RegId2 and
                        exists (select 1 from Last lc where lc.TxId = c.RowId)

                    cross join vTxStr sc on
                        sc.RowId = c.RowId

                    join Chain cc on
                        cc.TxId = c.RowId

                    left join Payload ps on
                        ps.TxId = c.RowId

                    join Transactions acs on
                        acs.Type = 100 and
                        acs.RegId1 = s.RegId1 and
                        exists (select 1 from Last lacs where lacs.TxId = acs.RowId)

                    join Chain cacs on
                        cacs.TxId = acs.RowId

                    left join Payload pacs on
                        pacs.TxId = acs.RowId

                    left join Ratings racs indexed by Ratings_Type_Uid_Last_Height on
                        racs.Type = 0 and
                        racs.Uid = cacs.Uid and
                        racs.Last = 1

                    left join Transactions na on
                        na.Type = 100 and
                        na.RegId1 = c.RegId1 and
                        exists (select 1 from Last lna where lna.TxId = na.RowId)

                    left join Chain cna on
                        cna.TxId = na.RowId

                    left join Payload pna on
                        pna.TxId = na.RowId

                    left join Ratings rna indexed by Ratings_Type_Uid_Last_Height on
                        rna.Type = 0 and
                        rna.Uid = cna.Uid and
                        rna.Last = 1

                    where
                        s.Type = 300
            )sql",
                heightBinder
            }},

            {
                ShortTxType::PrivateContent, { R"sql(
                    -- Content from private subscribers
                    select
                        ssubs.String1,
                        pna.String1,
                        pna.String2,
                        pna.String3,
                        ifnull(rna.Value,0),
                        (')sql" + ShortTxTypeConvertor::toString(ShortTxType::PrivateContent) + R"sql(')TP,
                        sc.Hash,
                        c.Type,
                        sc.String1,
                        cc.Height as Height,
                        cc.BlockNum as BlockNum,
                        c.Time,
                        sc.String2,
                        null,
                        null,
                        null,
                        null,
                        p.String2,
                        null,
                        null,
                        pac.String1,
                        pac.String2,
                        pac.String3,
                        ifnull(rac.Value,0),
                        null,
                        sr.Hash,
                        r.Type,
                        sr.String1,
                        cr.Height,
                        cr.BlockNum,
                        r.Time,
                        sr.String2,
                        null,
                        null,
                        null,
                        null,
                        pr.String2

                    from Transactions c -- content for private subscribers

                    cross join vTxStr sc on
                        sc.RowId = c.RowId

                    join Chain cc indexed by Chain_TxId_Height on
                        cc.TxId = c.RowId and
                        cc.Height = ?

                    join Transactions subs on -- Subscribers private
                        subs.Type = 303 and
                        subs.RegId2 = c.RegId1 and
                        exists (select 1 from Last ls where ls.TxId = subs.RowId)

                    cross join vTxStr ssubs on
                        ssubs.RowId = subs.RowId

                    left join Transactions r on -- related content - possible reposts
                        r.Type = 200 and
                        r.RegId2 = c.RegId3 and
                        exists (select 1 from Last rl where rl.TxId = r.RowId)

                    left join vTxStr sr on
                        sr.RowId = r.RowId

                    left join Chain cr on
                        cr.TxId = r.RowId

                    left join Payload pr on
                        pr.TxId = r.RowId

                    cross join Transactions ac on
                        ac.Type = 100 and
                        ac.RegId1 = c.RegId1 and
                        exists (select 1 from Last lac where lac.TxId = ac.RowId)

                    join Chain cac on
                        cac.TxId = ac.RowId

                    cross join Payload p on
                        p.TxId = c.RowId

                    cross join Payload pac on
                        pac.TxId = ac.RowId

                    left join Ratings rac indexed by Ratings_Type_Uid_Last_Height on
                        rac.Type = 0 and
                        rac.Uid = cac.Uid and
                        rac.Last = 1

                    left join Transactions na on
                        na.Type = 100 and
                        na.RegId1 = subs.RegId1 and
                        exists (select 1 from Last lna where lna.TxId = na.RowId)

                    left join Chain cna on
                        cna.TxId = na.RowId

                    left join Payload pna on
                        pna.TxId = na.RowId

                    left join Ratings rna indexed by Ratings_Type_Uid_Last_Height on
                        rna.Type = 0 and
                        rna.Uid = cna.Uid and
                        rna.Last = 1

                    where
                        c.Type in (200,201,202,209,210) and
                        exists (select 1 from First f where f.TxId = c.RowId)
            )sql",
                heightBinder
            }},

            {
                ShortTxType::Boost, { R"sql(
                    -- Boosts for my content
                    select
                        sc.String1,
                        pna.String1,
                        pna.String2,
                        pna.String3,
                        ifnull(rna.Value,0),
                        (')sql" + ShortTxTypeConvertor::toString(ShortTxType::Boost) + R"sql(')TP,
                        sb.Hash,
                        tboost.Type,
                        sb.String1,
                        c.Height as Height,
                        c.BlockNum as BlockNum,
                        tBoost.Time,
                        sb.String2,
                        null,
                        null,
                        (
                            select json_group_array(json_object(
                                'Value', o.Value,
                                'Number', o.Number,
                                'AddressHash', (select r.String from Registry r where r.RowId = o.AddressId),
                                'ScriptPubKey', (select r.String from Registry r where r.RowId = o.ScriptPubKeyId)
                            ))

                            from
                                TxInputs i indexed by TxInputs_SpentTxId_Number_TxId

                                join TxOutputs o indexed by TxOutputs_TxId_Number_AddressId on
                                    o.TxId = i.TxId and
                                    o.Number = i.Number

                            where
                                i.SpentTxId = tBoost.RowId
                        ),
                        (
                            select json_group_array(json_object(
                                'Value', o.Value,
                                'AddressHash', (select r.String from Registry r where r.RowId = o.AddressId),
                                'ScriptPubKey', (select r.String from Registry r where r.RowId = o.ScriptPubKeyId)
                            ))

                            from
                                TxOutputs o indexed by TxOutputs_TxId_Number_AddressId

                            where
                                o.TxId = tBoost.RowId
                            order by
                                o.Number
                        ),
                        null,
                        null,
                        null,
                        pac.String1,
                        pac.String2,
                        pac.String3,
                        ifnull(rac.Value,0),
                        null,
                        sc.Hash,
                        tContent.Type,
                        null,
                        cc.Height,
                        cc.BlockNum,
                        tContent.Time,
                        sc.String2,
                        null,
                        null,
                        null,
                        null,
                        pContent.String2

                    from Transactions tBoost

                    cross join vTxStr sb on
                        sb.RowId = tBoost.RowId

                    join Chain c indexed by Chain_TxId_Height on
                        c.TxId = tBoost.RowId and
                        c.Height = ?

                    left join Transactions tContent on
                        tContent.Type in (200,201,202,209,210) and
                        tContent.RegId2 = tBoost.RegId2 and
                        exists (select 1 from Last l where l.TxId = tContent.RowId)

                    left join Chain cc on
                        cc.TxId = tContent.RowId

                    left join vTxStr sc on
                        sc.RowId = tContent.RowId

                    left join Payload pContent on
                        pContent.TxId = tContent.RowId

                    left join Transactions ac on
                        ac.RegId1 = tBoost.RegId1 and
                        ac.Type = 100 and
                        exists (select 1 from Last lac where lac.TxId = ac.RowId)

                    left join Chain cac on
                        cac.TxId = ac.RowId

                    left join Payload pac on
                        pac.TxId = ac.RowId

                    left join Ratings rac indexed by Ratings_Type_Uid_Last_Height on
                        rac.Type = 0 and
                        rac.Uid = cac.Uid and
                        rac.Last = 1

                    left join Transactions na on
                        na.Type = 100 and
                        na.RegId1 = tContent.RegId1 and
                        exists (select 1 from Last lna where lna.TxId = na.RowId)

                    left join Chain cna on
                        cna.TxId = na.RowId

                    left join Payload pna on
                        pna.TxId = na.RowId

                    left join Ratings rna indexed by Ratings_Type_Uid_Last_Height on
                        rna.Type = 0 and
                        rna.Uid = cna.Uid and
                        rna.Last = 1

                    where
                        tBoost.Type in (208) and
                        exists (select 1 from Last l where l.TxId = tBoost.RowId)
            )sql",
                heightBinder
            }},

        {
            ShortTxType::Repost, { R"sql(
                -- Reposts
                select
                    sp.String1,
                    pna.String1,
                    pna.String2,
                    pna.String3,
                    ifnull(rna.Value,0),
                    (')sql" + ShortTxTypeConvertor::toString(ShortTxType::Repost) + R"sql(')TP,
                    sr.Hash,
                    r.Type,
                    sr.String1,
                    cr.Height as Height,
                    cr.BlockNum as BlockNum,
                    r.Time,
                    sr.String2,
                    null,
                    null,
                    null,
                    null,
                    pr.String2,
                    null,
                    null,
                    par.String1,
                    par.String2,
                    par.String3,
                    ifnull(rar.Value,0),
                    null,
                    sp.Hash,
                    p.Type,
                    null,
                    cp.Height,
                    cp.BlockNum,
                    p.Time,
                    sp.String2,
                    null,
                    null,
                    null,
                    null,
                    pp.String2

                from Transactions r

                cross join vTxStr sr on
                    sr.RowId = r.RowId

                join Chain cr indexed by Chain_TxId_Height on
                    cr.TxId = r.RowId and
                    cr.Height = ?

                join Transactions p on
                    p.Type = 200 and
                    p.RegId2 = r.RegId3 and
                    exists (select 1 from Last lp where lp.TxId = p.RowId)

                cross join vTxStr sp on
                    sp.RowId = p.RowId

                join Chain cp on
                    cp.TxId = p.RowId

                left join Payload pp on
                    pp.TxId = p.RowId

                left join Payload pr on
                    pr.TxId = r.RowId

                join Transactions ar on
                    ar.Type = 100 and
                    ar.RegId1 = r.RegId1 and
                    exists (select 1 from Last lar where lar.TxId = ar.RowId)

                join Chain car on
                    car.TxId = ar.RowId

                left join Payload par on
                    par.TxId = ar.RowId

                left join Ratings rar indexed by Ratings_Type_Uid_Last_Height on
                    rar.Type = 0 and
                    rar.Uid = car.Uid and
                    rar.Last = 1

                left join Transactions na on
                    na.Type = 100 and
                    na.RegId1 = p.RegId1 and
                    exists (select 1 from Last lna where lna.TxId = na.RowId)

                left join Chain cna on
                    cna.TxId = na.RowId

                left join Payload pna on
                    pna.TxId = na.RowId

                left join Ratings rna indexed by Ratings_Type_Uid_Last_Height on
                    rna.Type = 0 and
                    rna.Uid = cna.Uid and
                    rna.Last = 1

                where
                    r.Type = 200 and
                    r.RegId3 > 0 and
                    exists (select 1 from First f where f.TxId = r.RowId) -- Only orig
            )sql",
                heightBinder
            }},

            {
                ShortTxType::JuryAssigned, { R"sql(
                    select
                        -- Notifier data
                        su.String1,
                        p.String1,
                        p.String2,
                        p.String3,
                        ifnull(rn.Value, 0),
                        -- type of shortForm
                        (')sql" + ShortTxTypeConvertor::toString(ShortTxType::JuryAssigned) + R"sql('),
                        -- Flag data
                        sf.Hash, -- Jury Tx
                        f.Type,
                        null,
                        cf.Height,
                        cf.BlockNum,
                        f.Time,
                        null,
                        null, -- Tx of content
                        f.Int1,
                        null,
                        null,
                        null,
                        null,
                        null,
                        -- Account data for tx creator
                        null,
                        null,
                        null,
                        null,
                        -- Additional data
                        null,
                        -- Related Content
                        sc.Hash,
                        c.Type,
                        sc.String1,
                        cc.Height,
                        cc.BlockNum,
                        c.Time,
                        sc.String2,
                        sc.String3,
                        null,
                        null,
                        null,
                        (
                            case
                                when c.Type in (204,205) then cp.String1
                                else cp.String2
                            end
                        ),
                        sc.String4,
                        sc.String5,
                        -- Account data for related tx
                        null,
                        null,
                        null,
                        null,
                        null

                    from
                        Transactions f

                        cross join Chain cf indexed by Chain_TxId_Height on
                            cf.TxId = f.RowId and
                            cf.Height = ?

                        cross join vTxStr sf on
                            sf.RowId = f.RowId

                        cross join Jury j
                            on j.FlagRowId = f.ROWID

                        cross join Chain cu on
                            cu.Uid = j.AccountId and
                            exists (select 1 from Last l where l.TxId = cu.TxId)

                        cross join vTxStr su on
                            su.RowId = cu.TxId

                        cross join Payload p
                            on p.TxId = cu.TxId

                        cross join Transactions c indexed by Transactions_HashId
                            on c.HashId = f.RegId2

                        cross join Chain cc on
                            cc.TxId = c.RowId

                        cross join vTxStr sc on
                            sc.RowId = c.RowId

                        cross join Payload cp
                            on cp.TxId = c.RowId

                        left join Ratings rn indexed by Ratings_Type_Uid_Last_Height
                            on rn.Type = 0 and rn.Uid = j.AccountId and rn.Last = 1

                    where
                        f.Type = 410
                )sql",
                heightBinder
            }},

            {
                ShortTxType::JuryVerdict, { R"sql(
                    select
                        -- Notifier data
                        su.String1,
                        p.String1,
                        p.String2,
                        p.String3,
                        ifnull(rn.Value, 0),
                        -- type of shortForm
                        (')sql" + ShortTxTypeConvertor::toString(ShortTxType::JuryVerdict) + R"sql('),
                        -- Tx data
                        sv.Hash,
                        v.Type,
                        null,
                        cv.Height,
                        cv.BlockNum,
                        v.Time,
                        null,
                        sv.String2, -- Jury Tx
                        v.Int1, -- Value
                        null,
                        null,
                        null,
                        null,
                        null,
                        -- Account data for tx creator
                        null,
                        null,
                        null,
                        null,
                        -- Additional data
                        null,
                        -- Related Content
                        sc.Hash,
                        c.Type,
                        sc.String1,
                        cc.Height,
                        cc.BlockNum,
                        c.Time,
                        sc.String2,
                        sc.String3,
                        null,
                        null,
                        null,
                        (
                            case
                                when c.Type in (204,205) then cp.String1
                                else cp.String2
                            end
                        ),
                        sc.String4,
                        sc.String5,
                        -- Account data for related tx
                        null,
                        null,
                        null,
                        null,
                        null

                    from
                        Transactions v

                        cross join Chain cv indexed by Chain_TxId_Height on
                            cv.TxId = v.RowId and
                            cv.Height = ?

                        cross join vTxStr sv on
                            sv.RowId = v.RowId

                        cross join JuryBan jb
                            on jb.VoteRowId = v.ROWID

                        cross join Chain cu on
                            cu.Uid = jb.AccountId and
                            exists (select 1 from Last l where l.TxId = cu.TxId)

                        cross join vTxStr su on
                            su.RowId = cu.TxId

                        cross join Payload p
                            on p.TxId = cu.TxId

                        cross join JuryVerdict jv indexed by JuryVerdict_VoteRowId_FlagRowId_Verdict
                            on jv.VoteRowId = jb.VoteRowId

                        cross join Transactions f
                            on f.RowId = jv.FlagRowId

                        cross join Transactions c indexed by Transactions_HashId on
                            c.HashId = f.RegId2

                        cross join Chain cc on
                            cc.TxId = c.RowId

                        cross join vTxStr sc on
                            sc.RowId = c.RowId

                        cross join Payload cp on
                            cp.TxId = c.RowId

                        left join Ratings rn indexed by Ratings_Type_Uid_Last_Height on
                            rn.Type = 0 and rn.Uid = jb.AccountId and rn.Last = 1

                    where
                        v.Type = 420
                )sql",
                heightBinder
            }},

            {
                ShortTxType::JuryModerate, { R"sql(
                    select
                        -- Notifier data
                        su.String1,
                        p.String1,
                        p.String2,
                        p.String3,
                        ifnull(rn.Value, 0),
                        -- type of shortForm
                        (')sql" + ShortTxTypeConvertor::toString(ShortTxType::JuryModerate) + R"sql('),
                        -- Tx data
                        sf.Hash, -- Jury Tx
                        f.Type,
                        null,
                        cf.Height,
                        cf.BlockNum,
                        f.Time,
                        null,
                        null,
                        f.Int1, -- Value
                        null,
                        null,
                        null,
                        null,
                        null,
                        -- Account data for tx creator
                        null,
                        null,
                        null,
                        null,
                        -- Additional data
                        null,
                        -- Related Content
                        sc.Hash,
                        c.Type,
                        sc.String1,
                        cc.Height,
                        cc.BlockNum,
                        c.Time,
                        sc.String2,
                        sc.String3,
                        null,
                        null,
                        null,
                        (
                            case
                                when c.Type in (204,205) then cp.String1
                                else cp.String2
                            end
                        ),
                        sc.String4,
                        sc.String5,
                        -- Account data for related tx
                        suc.String1,
                        cpc.String1,
                        cpc.String2,
                        cpc.String3,
                        ifnull(rnc.Value, 0)

                    from
                        Transactions f

                        cross join Chain cf indexed by Chain_TxId_Height on
                            cf.TxId = f.RowId and
                            cf.Height = (? - 10)

                        cross join vTxStr sf on
                            sf.RowId = f.RowId

                        cross join JuryModerators jm
                            on jm.FlagRowId = f.RowId

                        cross join Chain cu on
                            cu.Uid = jm.AccountId and
                            exists (select 1 from Last l where l.TxId = cu.TxId)

                        cross join vTxStr su on
                            su.RowId = cu.TxId

                        cross join Payload p
                            on p.TxId = cu.TxId

                        left join Ratings rn indexed by Ratings_Type_Uid_Last_Height
                            on rn.Type = 0 and rn.Uid = jm.AccountId and rn.Last = 1

                        -- content
                        cross join Transactions c indexed by Transactions_HashId
                            on c.HashId = f.RegId2

                        cross join Chain cc on
                            cc.TxId = c.RowId

                        cross join vTxStr sc on
                            sc.RowId = c.RowId

                        cross join Payload cp
                            on cp.TxId = c.RowId

                        -- content author
                        cross join Transactions uc
                            on uc.Type = 100 and exists (select 1 from Last l where l.TxId = uc.RowId) and uc.RegId1 = c.RegId1

                        cross join Chain cuc on
                            cuc.TxId = uc.RowId

                        cross join vTxStr suc on
                            suc.RowId = uc.RowId

                        cross join Payload cpc
                            on cpc.TxId = uc.RowId

                        left join Ratings rnc indexed by Ratings_Type_Uid_Last_Height
                            on rnc.Type = 0 and rnc.Uid = cuc.Uid and rnc.Last = 1

                    where
                        f.Type = 410
                )sql",
                heightBinder
            }}

        };

        auto predicate = _choosePredicate(filters);
        
        NotificationsReconstructor reconstructor;
        for(const auto& select: selects) {
            if (predicate(select.first)) {
                const auto& selectData = select.second;
                SqlTransaction(__func__, [&]()
                {
                    auto& stmt = Sql(selectData.query);

                    selectData.binding(stmt, queryParams);

                    stmt.Select([&](Cursor& cursor) {
                        while (cursor.Step())
                            reconstructor.FeedRow(cursor);
                    });
                });
            }
        }
        auto notificationResult = reconstructor.GetResult();
        return notificationResult.Serialize();
    }

    // TODO (aok, api): implement
    std::vector<ShortForm> WebRpcRepository::GetEventsForAddresses(const std::string& address, int64_t heightMax, int64_t heightMin, int64_t blockNumMax, const std::set<ShortTxType>& filters)
    {
        // This is required because we want static bind functors for optimization so parameters can't be captured there 
        struct QueryParams {
            // Handling all by reference
            const std::string& address;
            const int64_t& heightMax;
            const int64_t& heightMin;
            const int64_t& blockNumMax;
        } queryParams {address, heightMax, heightMin, blockNumMax};

        static const std::map<ShortTxType, ShortFormSqlEntry<Stmt&, QueryParams>> selects = {
            {
                ShortTxType::Money, { R"sql(
                    -- Incoming money
                    select distinct
                        (')sql" + ShortTxTypeConvertor::toString(ShortTxType::Money) + R"sql(')TP,
                        st.Hash,
                        t.Type,
                        null,
                        ct.Height as Height,
                        ct.BlockNum as BlockNum,
                        null,
                        null,
                        null,
                        null,
                        (
                            select json_group_array(json_object(
                                'Value', o.Value,
                                'Number', o.Number,
                                'AddressHash', (select r.String from Registry r where r.RowId = o.AddressId),
                                'ScriptPubKey', (select r.String from Registry r where r.RowId = o.ScriptPubKeyId)
                            ))

                            from
                                TxInputs i indexed by TxInputs_SpentTxId_Number_TxId

                                join TxOutputs o indexed by TxOutputs_TxId_Number_AddressId on
                                    o.TxId = i.TxId and
                                    o.Number = i.Number

                            where
                                i.SpentTxId = t.RowId
                        ),
                        (
                            select json_group_array(json_object(
                                'Value', o.Value,
                                'AddressHash', (select r.String from Registry r where r.RowId = o.AddressId),
                                'ScriptPubKey', (select r.String from Registry r where r.RowId = o.ScriptPubKeyId),
                                'Account', json_object(
                                    'Lang', pna.String1,
                                    'Name', pna.String2,
                                    'Avatar', pna.String3,
                                    'Rep', ifnull(rna.Value,0)
                                )
                            ))

                            from
                                TxOutputs o indexed by TxOutputs_TxId_Number_AddressId

                                left join Transactions na indexed by Transactions_Type_RegId1_RegId2_RegId3 on
                                    na.Type = 100 and
                                    na.RegId1 = o.AddressId and
                                    exists (select 1 from Last l where l.TxId = na.RowId)

                                left join Chain cna on
                                    cna.TxId = na.RowId

                                left join Payload pna on
                                    pna.TxId = na.RowId

                                left join Ratings rna indexed by Ratings_Type_Uid_Last_Height on
                                    rna.Type = 0 and
                                    rna.Uid = cna.Uid and
                                    rna.Last = 1

                            where
                                o.TxId = t.RowId
                            order by
                                o.Number
                        ),
                        null,
                        null,
                        null,
                        null,
                        null,
                        null,
                        null,
                        null,
                        null,
                        null,
                        null,
                        null,
                        null,
                        null,
                        null,
                        null,
                        null,
                        null,
                        null,
                        null,
                        null,
                        null,
                        null,
                        null,
                        null,
                        null,
                        null

                    from
                        params,
                        TxOutputs o indexed by TxOutputs_AddressId_TxId_Number

                        join Transactions t on
                            t.RowId = o.TxId and
                            t.Type in (1,2,3)

                        join Chain ct indexed by Chain_Height_BlockNum on
                            ct.TxId = t.RowId and
                            ct.Height > params.min and
                            (ct.Height < params.max or (ct.Height = params.max and ct.BlockNum < params.blockNum))

                        cross join vTxStr st on
                            st.Rowid = t.RowId

                    where
                        o.AddressId = (select RowId from Registry where String = ?)
                )sql",
                [](Stmt& stmt, QueryParams const& queryParams) {
                    stmt.Bind(
                        queryParams.address
                    );
                }
            }},

            {
                ShortTxType::Referal, { R"sql(
                    -- referals
                    select
                        (')sql" + ShortTxTypeConvertor::toString(ShortTxType::Referal) + R"sql(')TP,
                        s.Hash,
                        t.Type,
                        s.String1,
                        c.Height as Height,
                        c.BlockNum as BlockNum,
                        null,
                        null,
                        null,
                        null,
                        null,
                        null,
                        null,
                        null,
                        null,
                        p.String1,
                        p.String2,
                        p.String3,
                        ifnull(r.Value,0),
                        null,
                        null,
                        null,
                        null,
                        null,
                        null,
                        null,
                        null,
                        null,
                        null,
                        null,
                        null,
                        null,
                        null,
                        null,
                        null,
                        null,
                        null,
                        null,
                        null

                    from
                        params,
                        Transactions t indexed by Transactions_Type_RegId2_RegId1

                        cross join vTxStr s on
                            s.RowId = t.RowId

                        join Chain c indexed by Chain_Height_BlockNum on
                            c.TxId = t.RowId and
                            c.Height > params.min and
                            (c.Height < params.max or (c.Height = params.max and c.BlockNum < params.blockNum))

                        left join Transactions tLast indexed by Transactions_Type_RegId1_RegId2_RegId3 on
                            tLast.Type = 100 and
                            tLast.RegId1 = t.RegId1 and
                            exists (select 1 from Last l where l.TxId = tLast.RowId)

                        left join Chain cLast on
                            cLast.TxId = tLast.RowId

                        left join Payload p on
                            p.TxId = tLast.RowId

                        left join Ratings r indexed by Ratings_Type_Uid_Last_Height on
                            r.Type = 0 and
                            r.Uid = cLast.Uid and
                            r.Last = 1

                    where
                        t.Type = 100 and
                        t.RegId2 = (select RowId from Registry where String = ?) and
                        exists (select 1 from First f where f.TxId = t.RowId)
                )sql",
                [](Stmt& stmt, QueryParams const& queryParams) {
                    stmt.Bind(
                        queryParams.address
                    );
                }
            }},

            {
                ShortTxType::Answer, { R"sql(
                    -- Comment answers
                    select
                        (')sql" + ShortTxTypeConvertor::toString(ShortTxType::Answer) + R"sql(')TP,
                        sa.Hash,
                        a.Type,
                        sa.String1,
                        ca.Height as Height,
                        ca.BlockNum as BlockNum,
                        null,
                        sa.String2,
                        sa.String3,
                        null,
                        null,
                        null,
                        pa.String1,
                        sa.String4,
                        sa.String5,
                        paa.String1,
                        paa.String2,
                        paa.String3,
                        ifnull(ra.Value,0),
                        null,
                        sc.Hash,
                        c.Type,
                        null,
                        cc.Height,
                        cc.BlockNum,
                        null,
                        sc.String2,
                        null,
                        null,
                        null,
                        null,
                        pc.String1,
                        null,
                        null,
                        null,
                        null,
                        null,
                        null,
                        null

                    from
                        params,
                        Transactions c indexed by Transactions_Type_RegId1_RegId2_RegId3

                        cross join Chain cc on
                            cc.TxId = c.RowId

                        cross join vTxStr sc on
                            sc.RowId = c.RowId

                        left join Payload pc on
                            pc.TxId = c.RowId

                        join Transactions a indexed by Transactions_Type_RegId5_RegId1 on
                            a.Type in (204, 205) and
                            a.RegId5 = c.RegId2 and
                            a.RegId1 != c.RegId1 and
                            exists (select 1 from First f where f.TxId = a.RowId)

                        cross join vTxStr sa on
                            sa.RowId = a.RowId

                        join Chain ca indexed by Chain_Height_BlockNum on
                            ca.TxId = a.RowId and
                            ca.Height > params.min and
                            (ca.Height < params.max or (ca.Height = params.max and ca.BlockNum < params.blockNum))

                        left join Transactions aLast indexed by Transactions_Type_RegId2_RegId1 on
                            aLast.Type in (204,205) and
                            aLast.RegId2 = a.HashId and
                            exists (select 1 from Last l where l.TxId = aLast.RowId)

                        left join Payload pa on
                            pa.TxId = aLast.RowId

                        left join Transactions aa indexed by Transactions_Type_RegId1_RegId2_RegId3 on
                            aa.Type = 100 and
                            aa.RegId1 = a.RegId1 and
                            exists (select 1 from Last l where l.TxId = aa.RowId)

                        left join Chain caa on
                            caa.TxId = aa.RowId

                        left join Payload paa on
                            paa.TxId = aa.RowId

                        left join Ratings ra indexed by Ratings_Type_Uid_Last_Height on
                            ra.Type = 0 and
                            ra.Uid = caa.Uid and
                            ra.Last = 1

                    where
                        c.Type in (204, 205) and
                        c.RegId1 = (select RowId from Registry where String = ?) and
                        exists (select 1 from Last l where l.TxId = c.RowId)
                )sql",
                [](Stmt& stmt, QueryParams const& queryParams) {
                    stmt.Bind(
                        queryParams.address
                    );
                }
            }},

            {
                ShortTxType::Comment, { R"sql(
                    -- Comments for my content
                    select
                        (')sql" + ShortTxTypeConvertor::toString(ShortTxType::Comment) + R"sql(')TP,
                        sc.Hash,
                        c.Type,
                        sc.String1,
                        cc.Height as Height,
                        cc.BlockNum as BlockNum,
                        null,
                        sc.String2,
                        null,
                        null,
                        (
                            select json_group_array(json_object(
                                'Value', o.Value,
                                'Number', o.Number,
                                'AddressHash', (select r.String from Registry r where r.RowId = o.AddressId),
                                'ScriptPubKey', (select r.String from Registry r where r.RowId = o.ScriptPubKeyId)
                            ))

                            from
                                TxInputs i indexed by TxInputs_SpentTxId_Number_TxId

                                join TxOutputs o indexed by TxOutputs_TxId_Number_AddressId on
                                    o.TxId = i.TxId and
                                    o.Number = i.Number

                            where
                                i.SpentTxId = c.RowId
                        ),
                        (
                            select json_group_array(json_object(
                                'Value', o.Value,
                                'AddressHash', (select r.String from Registry r where r.RowId = o.AddressId),
                                'ScriptPubKey', (select r.String from Registry r where r.RowId = o.ScriptPubKeyId)
                            ))

                            from
                                TxOutputs o indexed by TxOutputs_TxId_Number_AddressId

                            where
                                o.TxId = c.RowId
                            order by
                                o.Number
                        ),
                        pc.String1,
                        null,
                        null,
                        pac.String1,
                        pac.String2,
                        pac.String3,
                        ifnull(rac.Value,0),
                        null,
                        sp.Hash,
                        p.Type,
                        null,
                        cp.Height,
                        cp.BlockNum,
                        null,
                        sp.String2,
                        null,
                        null,
                        null,
                        null,
                        pp.String2,
                        null,
                        null,
                        null,
                        null,
                        null,
                        null,
                        null

                    from
                        params,
                        Transactions p indexed by Transactions_Type_RegId1_RegId2_RegId3

                        cross join Chain cp on
                            cp.TxId = p.RowId

                        cross join vTxStr sp on
                            sp.RowId = p.RowId

                        join Transactions c indexed by Transactions_Type_RegId3_RegId4_RegId5_RegId1 on
                            c.Type = 204 and
                            c.RegId3 = p.RegId2 and
                            c.RegId1 != p.RegId1 and
                            c.RegId4 is null and
                            c.RegId5 is null

                        join Chain cc indexed by Chain_Height_BlockNum on
                            cc.TxId = c.RowId and
                            cc.Height > params.min and
                            (cc.Height < params.max or (cc.Height = params.max and cc.BlockNum < params.blockNum))

                        cross join vTxStr sc on
                            sc.RowId = c.RowId

                        left join Transactions cLast indexed by Transactions_Type_RegId2_RegId1 on
                            cLast.Type in (204,205) and
                            cLast.RegId2 = c.HashId and
                            exists (select 1 from Last l where l.TxId = cLast.RowId)

                        left join Payload pc on
                            pC.TxId = cLast.RowId

                        join Transactions ac indexed by Transactions_Type_RegId1_RegId2_RegId3 on
                            ac.RegId1 = c.RegId1 and
                            ac.Type = 100 and
                            exists (select 1 from Last l where l.TxId = ac.RowId)

                        cross join Chain cac on
                            cac.TxId = ac.RowId

                        left join Payload pac on
                            pac.TxId = ac.RowId

                        left join Ratings rac indexed by Ratings_Type_Uid_Last_Height on
                            rac.Type = 0 and
                            rac.Uid = cac.Uid and
                            rac.Last = 1

                        left join Payload pp on
                            pp.TxId = p.RowId

                    where
                        p.Type in (200,201,202,209,210) and
                        p.RegId1 = (select RowId from Registry where String = ?) and
                        exists (select 1 from Last l where l.TxId = p.RowId)
                )sql",
                [](Stmt& stmt, QueryParams const& queryParams) {
                    stmt.Bind(
                        queryParams.address
                    );
                }
            }},

            {
                ShortTxType::Subscriber, { R"sql(
                    -- Subscribers
                    select
                        (')sql" + ShortTxTypeConvertor::toString(ShortTxType::Subscriber) + R"sql(')TP,
                        s.Hash,
                        subs.Type,
                        s.String1,
                        c.Height as Height,
                        c.BlockNum as BlockNum,
                        null,
                        null,
                        null,
                        null,
                        null,
                        null,
                        null,
                        null,
                        null,
                        pu.String1,
                        pu.String2,
                        pu.String3,
                        ifnull(ru.Value,0),
                        null,
                        null,
                        null,
                        null,
                        null,
                        null,
                        null,
                        null,
                        null,
                        null,
                        null,
                        null,
                        null,
                        null,
                        null,
                        null,
                        null,
                        null,
                        null,
                        null

                    from
                        params,
                        Transactions subs indexed by Transactions_Type_RegId2_RegId1

                        join Chain c on
                            c.TxId = subs.RowId and
                            c.Height > params.min and
                            (c.Height < params.max or (c.Height = params.max and c.BlockNum < params.blockNum))

                        cross join vTxStr s on
                            s.RowId = subs.RowId

                        join Transactions u indexed by Transactions_Type_RegId1_RegId2_RegId3 on
                            u.Type in (100) and
                            u.RegId1 = subs.RegId1 and
                            exists (select 1 from Last l where l.TxId = u.RowId)

                        cross join Chain cu on
                            cu.TxId = u.RowId

                        left join Payload pu on
                            pu.TxId = u.RowId

                        left join Ratings ru indexed by Ratings_Type_Uid_Last_Height on
                            ru.Type = 0 and
                            ru.Uid = cu.Uid and
                            ru.Last = 1

                    where
                        subs.Type in (302, 303, 304) and -- Ignoring unsubscribers?
                        subs.RegId2 = (select RowId from Registry where String = ?)
                )sql",
                [](Stmt& stmt, QueryParams const& queryParams) {
                    stmt.Bind(
                        queryParams.address
                    );
                }
            }},

            {
                ShortTxType::CommentScore, { R"sql(
                    -- Comment scores
                    select
                        (')sql" + ShortTxTypeConvertor::toString(ShortTxType::CommentScore) + R"sql(')TP,
                        ss.Hash,
                        s.Type,
                        ss.String1,
                        cs.Height as Height,
                        cs.BlockNum as BlockNum,
                        null,
                        null,
                        null,
                        s.Int1,
                        null,
                        null,
                        null,
                        null,
                        null,
                        pacs.String1,
                        pacs.String2,
                        pacs.String3,
                        ifnull(racs.Value,0),
                        null,
                        sc.Hash,
                        c.Type,
                        null,
                        cc.Height,
                        cc.BlockNum,
                        null,
                        sc.String2,
                        sc.String3,
                        null,
                        null,
                        null,
                        ps.String1,
                        sc.String4,
                        sc.String5,
                        null,
                        null,
                        null,
                        null,
                        null

                    from
                        params,
                        Transactions c indexed by Transactions_Type_RegId1_RegId2_RegId3

                        cross join Chain cc on
                            cc.TxId = c.RowId

                        cross join vTxStr sc on
                            sc.RowId = c.RowId

                        join Transactions s indexed by Transactions_Type_RegId2_RegId1 on
                            s.Type = 301 and
                            s.RegId2 = c.RegId2

                        join Chain cs indexed by Chain_Height_BlockNum on
                            cs.TxId = s.RowId and
                            cs.Height > params.min and
                            (cs.Height < params.max or (cs.Height = params.max and cs.BlockNum < params.blockNum))

                        cross join vTxStr ss on
                            ss.RowId = s.RowId

                        left join Transactions cLast indexed by Transactions_Type_RegId2_RegId1 on
                            cLast.Type in (204,205) and
                            cLast.RegId2 = c.RegId2 and
                            exists (select 1 from Last l where l.TxId = cLast.RowId)

                        left join Payload ps on
                            ps.TxId = cLast.RowId

                        join Transactions acs indexed by Transactions_Type_RegId1_RegId2_RegId3 on
                            acs.Type = 100 and
                            acs.RegId1 = s.RegId1 and
                            exists (select 1 from Last l where l.TxId = acs.RowId)

                        cross join Chain cacs on
                            cacs.TxId = acs.RowId

                        left join Payload pacs on
                            pacs.TxId = acs.RowId

                        left join Ratings racs indexed by Ratings_Type_Uid_Last_Height on
                            racs.Type = 0 and
                            racs.Uid = cacs.Uid and
                            racs.Last = 1

                    where
                        c.Type in (204) and
                        c.RegId1 = (select RowId from Registry where String = ?)
                )sql",
                [](Stmt& stmt, QueryParams const& queryParams) {
                    stmt.Bind(
                        queryParams.address
                    );
                }
            }},

            {
                ShortTxType::ContentScore, { R"sql(
                    -- Content scores
                    select
                        (')sql" + ShortTxTypeConvertor::toString(ShortTxType::ContentScore) + R"sql(')TP,
                        ss.Hash,
                        s.Type,
                        ss.String1,
                        cs.Height as Height,
                        cs.BlockNum as BlockNum,
                        null,
                        null,
                        null,
                        s.Int1,
                        null,
                        null,
                        null,
                        null,
                        null,
                        pacs.String1,
                        pacs.String2,
                        pacs.String3,
                        ifnull(racs.Value,0),
                        null,
                        sc.Hash,
                        c.Type,
                        null,
                        cc.Height,
                        cc.BlockNum,
                        null,
                        sc.String2,
                        null,
                        null,
                        null,
                        null,
                        pc.String2,
                        null,
                        null,
                        null,
                        null,
                        null,
                        null,
                        null

                    from
                        params,
                        Transactions c indexed by Transactions_Type_RegId1_RegId2_RegId3

                        cross join Chain cc on
                            cc.TxId = c.RowId

                        cross join vTxStr sc on
                            sc.RowId = c.RowId

                        join Transactions s indexed by Transactions_Type_RegId2_RegId1 on
                            s.Type = 300 and
                            s.RegId2 = c.RegId2

                        join Chain cs indexed by Chain_Height_BlockNum on
                            cs.TxId = s.RowId and
                            cs.Height > params.min and
                            (cs.Height < params.max or (cs.Height = params.max and cs.BlockNum < params.blockNum))

                        cross join vTxStr ss on
                            ss.RowId = s.RowId

                        left join Transactions cLast indexed by Transactions_Type_RegId2_RegId1 on
                            cLast.Type = c.Type and
                            cLast.RegId2 = c.RegId2 and 
                            exists (select 1 from Last l where l.TxId = cLast.RowId)

                        left join Payload pc on
                            pc.TxId = cLast.RowId

                        join Transactions acs indexed by Transactions_Type_RegId1_RegId2_RegId3 on
                            acs.Type = 100 and
                            acs.RegId1 = s.RegId1 and
                            exists (select 1 from Last l where l.TxId = acs.RowId)

                        join Chain cacs on
                            cacs.TxId = acs.RowId

                        left join Payload pacs on
                            pacs.TxId = acs.RowId

                        left join Ratings racs indexed by Ratings_Type_Uid_Last_Height on
                            racs.Type = 0 and
                            racs.Uid = cacs.Uid and
                            racs.Last = 1

                    where
                        c.Type in (200,201,202,209,210) and
                        c.RegId1 = (select RowId from Registry where String = ?) and
                        exists (select 1 from First f where f.TxId = c.RowId)
                )sql",
                [](Stmt& stmt, QueryParams const& queryParams) {
                    stmt.Bind(
                        queryParams.address
                    );
                }
            }},

            {
                ShortTxType::PrivateContent, { R"sql(
                    -- Content from private subscribers
                    select
                        (')sql" + ShortTxTypeConvertor::toString(ShortTxType::PrivateContent) + R"sql(')TP,
                        sc.Hash,
                        c.Type,
                        sc.String1,
                        cc.Height as Height,
                        cc.BlockNum as BlockNum,
                        null,
                        sc.String2,
                        null,
                        null,
                        null,
                        null,
                        p.String2,
                        null,
                        null,
                        pac.String1,
                        pac.String2,
                        pac.String3,
                        ifnull(rac.Value,0),
                        null,
                        null,
                        null,
                        null,
                        null,
                        null,
                        null,
                        null,
                        null,
                        null,
                        null,
                        null,
                        null,
                        null,
                        null,
                        null,
                        null,
                        null,
                        null,
                        null

                    from
                        params,
                        Transactions subs indexed by Transactions_Type_RegId1_RegId2_RegId3 -- Subscribers private

                        join Transactions c indexed by Transactions_Type_RegId1_RegId2_RegId3 on-- content for private subscribers
                            c.Type in (200,201,202,209,210) and
                            c.RegId1 = subs.RegId2 and
                            exists (select 1 from First f where f.TxId = c.RowId)

                        join Chain cc indexed by Chain_Height_BlockNum on
                            cc.TxId = c.RowId and
                            cc.Height > params.min and
                            (cc.Height < params.max or (cc.Height = params.max and cc.BlockNum < params.blockNum))

                        cross join vTxStr sc on
                            sc.RowId = c.RowId

                        left join Transactions cLast indexed by Transactions_Type_RegId2_RegId1 on
                            cLast.Type = c.Type and
                            cLast.RegId2 = c.RegId2 and
                            exists (select 1 from Last l where l.TxId = cLast.RowId)

                        left join Payload p on
                            p.TxId = cLast.RowId

                        left join Transactions ac indexed by Transactions_Type_RegId1_RegId2_RegId3 on
                            ac.Type = 100 and
                            ac.RegId1 = c.RegId1 and
                            exists (select 1 from Last l where l.TxId = ac.RowId)

                        left join Chain cac on
                            cac.TxId = ac.RowId

                        left join Payload pac on
                            pac.TxId = ac.RowId

                        left join Ratings rac indexed by Ratings_Type_Uid_Last_Height on
                            rac.Type = 0 and
                            rac.Uid = cac.Uid and
                            rac.Last = 1

                    where
                        subs.Type = 303 and
                        subs.RegId1 = (select RowId from Registry where String = ?) and
                        exists (select 1 from Last l where l.TxId = subs.RowId)
                )sql",
                [](Stmt& stmt, QueryParams const& queryParams) {
                    stmt.Bind(
                        queryParams.address
                    );
                }
            }},

            {
                ShortTxType::Boost, { R"sql(
                    -- Boosts for my content
                    select
                        (')sql" + ShortTxTypeConvertor::toString(ShortTxType::Boost) + R"sql(')TP,
                        sBoost.Hash,
                        tBoost.Type,
                        sBoost.String1,
                        cBoost.Height as Height,
                        cBoost.BlockNum as BlockNum,
                        null,
                        null,
                        null,
                        null,
                        (
                            select json_group_array(json_object(
                                'Value', o.Value,
                                'Number', o.Number,
                                'AddressHash', (select r.String from Registry r where r.RowId = o.AddressId),
                                'ScriptPubKey', (select r.String from Registry r where r.RowId = o.ScriptPubKeyId)
                            ))

                            from
                                TxInputs i indexed by TxInputs_SpentTxId_Number_TxId

                                join TxOutputs o indexed by TxOutputs_TxId_Number_AddressId on
                                    o.TxId = i.TxId and
                                    o.Number = i.Number

                            where
                                i.SpentTxId = tBoost.RowId
                        ),
                        (
                            select json_group_array(json_object(
                                'Value', o.Value,
                                'AddressHash', (select r.String from Registry r where r.RowId = o.AddressId),
                                'ScriptPubKey', (select r.String from Registry r where r.RowId = o.ScriptPubKeyId)
                            ))

                            from
                                TxOutputs o indexed by TxOutputs_TxId_Number_AddressId

                            where
                                o.TxId = tBoost.RowId
                            order by
                                o.Number
                        ),
                        null,
                        null,
                        null,
                        pac.String1,
                        pac.String2,
                        pac.String3,
                        ifnull(rac.Value,0),
                        null,
                        sContent.Hash,
                        tContent.Type,
                        null,
                        cContent.Height,
                        cContent.BlockNum,
                        null,
                        sContent.String2,
                        null,
                        null,
                        null,
                        null,
                        pContent.String2,
                        null,
                        null,
                        null,
                        null,
                        null,
                        null,
                        null

                    from
                        params,
                        Transactions tBoost indexed by Transactions_Type_RegId2_RegId1

                        join Chain cBoost indexed by Chain_Height_BlockNum on
                            cBoost.TxId = tBoost.RowId and
                            cBoost.Height > params.min and
                            (cBoost.Height < params.max or (cBoost.Height = params.max and cBoost.BlockNum < params.blockNum))

                        cross join vTxStr sBoost on
                            sBoost.RowId = tBoost.RowId

                        join Transactions tContent indexed by Transactions_Type_RegId1_RegId2_RegId3 on
                            tContent.Type in (200,201,202,209,210) and
                            tContent.RegId1 = (select RowId from Registry where String = ?) and
                            tContent.RegId2 = tBoost.RegId2 and
                            exists (select 1 from Last l where l.TxId = tContent.RowId)

                        cross join vTxStr sContent on
                            sContent.RowId = tContent.RowId

                        cross join Chain cContent on
                            cContent.TxId = tContent.RowId

                        left join Payload pContent on
                            pContent.TxId = tContent.RowId

                        left join Transactions ac indexed by Transactions_Type_RegId1_RegId2_RegId3 on
                            ac.Type = 100 and
                            ac.RegId1 = tBoost.RegId1 and
                            exists (select 1 from Last l where l.TxId = ac.RowId)

                        left join Chain cac on
                            cac.TxId = ac.RowId

                        left join Payload pac on
                            pac.TxId = ac.RowId

                        left join Ratings rac indexed by Ratings_Type_Uid_Last_Height on
                            rac.Type = 0 and
                            rac.Uid = cac.Uid and
                            rac.Last = 1

                    where
                        tBoost.Type in (208)
                )sql",
                [](Stmt& stmt, QueryParams const& queryParams) {
                    stmt.Bind(
                        queryParams.address
                    );
                }
            }},

            {
                ShortTxType::Repost, { R"sql(
                    -- Reposts
                    select
                        (')sql" + ShortTxTypeConvertor::toString(ShortTxType::Repost) + R"sql(')TP,
                        sr.Hash,
                        r.Type,
                        sr.String1,
                        cr.Height as Height,
                        cr.BlockNum as BlockNum,
                        null,
                        sr.String2,
                        null,
                        null,
                        null,
                        null,
                        pr.String2,
                        null,
                        null,
                        par.String1,
                        par.String2,
                        par.String3,
                        ifnull(rar.Value,0),
                        null,
                        sp.Hash,
                        p.Type,
                        null,
                        cp.Height,
                        cp.BlockNum,
                        null,
                        sp.String2,
                        null,
                        null,
                        null,
                        null,
                        pp.String2,
                        null,
                        null,
                        null,
                        null,
                        null,
                        null,
                        null

                    from
                        params,
                        Transactions p indexed by Transactions_Type_RegId1_RegId2_RegId3

                        cross join Chain cp on
                            cp.TxId = p.RowId

                        cross join vTxStr sp on
                            sp.RowId = p.RowId

                        left join Payload pp on
                            pp.TxId = p.RowId

                        join Transactions r indexed by Transactions_Type_RegId3_RegId1 on
                            r.Type = 200 and
                            r.RegId3 = p.RegId2 and
                            exists (select 1 from First f where f.TxId = r.RowId)

                        cross join vTxStr sr on
                            sr.RowId = r.RowId

                        join Chain cr on
                            cr.TxId = r.RowId and
                            cr.Height > params.min and
                            (cr.Height < params.max or (cr.Height = params.max and cr.BlockNum < params.blockNum))

                        left join Transactions rLast indexed by Transactions_Type_RegId2_RegId1 on
                            rLast.Type = r.Type and
                            rLast.RegId2 = r.RegId2 and
                            exists (select 1 from Last l where l.TxId = rLast.RowId)

                        left join Payload pr on
                            pr.TxId = rLast.RowId

                        left join Transactions ar indexed by Transactions_Type_RegId1_RegId2_RegId3 on
                            ar.Type = 100 and
                            ar.RegId1 = r.RegId1 and
                            exists (select 1 from Last l where l.TxId = ar.RowId)

                        left join Chain car on
                            car.TxId = ar.RowId

                        left join Payload par on
                            par.TxId = ar.RowId

                        left join Ratings rar indexed by Ratings_Type_Uid_Last_Height
                            on rar.Type = 0
                            and rar.Uid = ar.Uid
                            and rar.Last = 1

                    where
                        p.Type = 200 and
                        p.RegId1 = (select RowId from Registry where String = ?) and
                        exists (select 1 from Last l where l.TxId = p.RowId)
                )sql",
                [](Stmt& stmt, QueryParams const& queryParams) {
                    stmt.Bind(
                        queryParams.address
                    );
                }
            }},

            {
                ShortTxType::JuryAssigned, { R"sql(
                    select
                        (')sql" + ShortTxTypeConvertor::toString(ShortTxType::JuryAssigned) + R"sql('),
                        -- Flag data
                        sf.Hash, -- Jury Tx
                        f.Type,
                        null,
                        cf.Height as Height,
                        cf.BlockNum as BlockNum,
                        f.Time,
                        null,
                        null,
                        f.Int1,
                        null,
                        null,
                        null,
                        null,
                        null,
                        -- Account data for tx creator
                        null,
                        null,
                        null,
                        null,
                        -- Additional data
                        null,
                        -- Related Content
                        sc.Hash,
                        c.Type,
                        sc.String1,
                        cc.Height,
                        cc.BlockNum,
                        c.Time,
                        sc.String2,
                        sc.String3,
                        null,
                        null,
                        null,
                        (
                            case
                                when c.Type in (204,205) then cp.String1
                                else cp.String2
                            end
                        ),
                        sc.String4,
                        sc.String5,
                        null,
                        null,
                        null,
                        null,
                        null

                    from
                        params,
                        Transactions f indexed by Transactions_Type_RegId3_RegId1 -- TODO (losty): better RegId3_RegId2

                        join Chain cf indexed by Chain_Height_BlockNum on
                            cf.TxId = f.RowId and
                            cf.Height > params.min and
                            (cf.Height < params.max or (cf.Height = params.max and cf.BlockNum < params.blockNum))

                        cross join vTxStr sf on
                            sf.RowId = f.RowId

                        cross join Jury j on
                            j.FlagRowId = f.RowId

                        cross join Transactions c indexed by Transactions_HashId on
                            c.HashId = f.RegId2

                        cross join Chain cc on
                            cc.TxId = c.RowId

                        cross join vTxStr sc on
                            sc.RowId = c.RowId

                        cross join Payload cp on
                            cp.TxId = c.RowId

                    where
                        f.Type = 410 and
                        -- f.Last = 0 and -- TODO (aok): is it required only for index?
                        f.RegId3 = (select RowId from Registry where String = ?)
                )sql",
                [](Stmt& stmt, QueryParams const& queryParams) {
                    stmt.Bind(
                        queryParams.address
                    );
                }
            }},

            {
                ShortTxType::JuryModerate, { R"sql(
                    select
                        (')sql" + ShortTxTypeConvertor::toString(ShortTxType::JuryModerate) + R"sql('),
                        -- Tx data
                        sf.Hash, -- Jury Tx
                        f.Type,
                        null,
                        cf.Height as Height,
                        cf.BlockNum as BlockNum,
                        f.Time,
                        null,
                        null, -- Tx of content
                        f.Int1, -- Value
                        null,
                        null,
                        null, -- Jury Tx
                        null,
                        null,
                        -- Account data for tx creator
                        null,
                        null,
                        null,
                        null,
                        -- Additional data
                        null,
                        -- Related Content
                        sc.Hash,
                        c.Type,
                        sc.String1,
                        cc.Height,
                        cc.BlockNum,
                        c.Time,
                        sc.String2,
                        sc.String3,
                        null,
                        null,
                        null,
                        (
                            case
                                when c.Type in (204,205) then cp.String1
                                else cp.String2
                            end
                        ),
                        sc.String4,
                        sc.String5,
                        -- Account data for related tx
                        suc.String1,
                        cpc.String1,
                        cpc.String2,
                        cpc.String3,
                        ifnull(rnc.Value, 0)

                    from
                        params,
                        Transactions acc indexed by Transactions_Type_RegId1_RegId2_RegId3

                        cross join Chain cacc on
                            cacc.TxId = acc.RowId

                        cross join JuryModerators jm indexed by JuryModerators_AccountId_FlagRowId on
                            jm.AccountId = cacc.Uid

                        cross join Transactions f on
                            f.RowId = jm.FlagRowId

                        join Chain cf indexed by Chain_Height_BlockNum on
                            cf.TxId = f.RowId and
                            cf.Height > params.min and
                            (cf.Height < params.max or (cf.Height = params.max and cf.BlockNum < params.blockNum))

                        cross join vTxStr sf on
                            sf.RowId = f.RowId

                        -- content
                        cross join Transactions c indexed by Transactions_HashId on
                            c.HashId = f.RegId2

                        cross join Chain cc on
                            cc.TxId = c.RowId

                        cross join vTxStr sc on
                            sc.RowId = c.RowId

                        cross join Payload cp on
                            cp.TxId = c.RowId

                        -- content author
                        cross join Transactions uc indexed by Transactions_Type_RegId1_RegId2_RegId3 on
                            uc.Type = 100 and
                            uc.RegId1 = c.RegId1 and
                            exists (select 1 from Last l where l.TxId = uc.RowId)

                        cross join vTxStr suc on
                            suc.RowId = uc.RowId

                        cross join Chain cuc on
                            cuc.TxId = uc.RowId

                        cross join Payload cpc on
                            cpc.TxId = uc.RowId

                        left join Ratings rnc indexed by Ratings_Type_Uid_Last_Height on
                            rnc.Type = 0 and
                            rnc.Uid = cuc.Uid and
                            rnc.Last = 1

                    where
                        acc.Type = 100 and
                        acc.RegId1 = (select RowId from Registry where String = ?) and
                        exists (select 1 from Last l where l.TxId = acc.RowId)
                )sql",
                [](Stmt& stmt, QueryParams const& queryParams) {
                    stmt.Bind(
                        queryParams.address
                    );
                }
            }}

        };

        static const auto header = R"sql(
            with
                params as (
                    select
                        ? as max,
                        ? as min,
                        ? as blockNum
                )
        )sql";

        static const auto footer = R"sql(

            -- Global order and limit for pagination
            order by Height desc, BlockNum desc
            limit 10

        )sql";
        
        auto [elem1, elem2] = _constructSelectsBasedOnFilters(filters, selects, header, footer);
        // A bit dirty hack because structure bindings can't be captured by lambda function.
        auto& sql = elem1;
        auto& binds = elem2;

        EventsReconstructor reconstructor;
        SqlTransaction(__func__, [&]()
        {
            auto& stmt = Sql(sql);
            
            // Binds for header
            stmt.Bind(heightMax, heightMin, blockNumMax);

            for (const auto& bind: binds) {
                bind(stmt, queryParams);
            }

            stmt.Select([&](Cursor& cursor) {
                while (cursor.Step())
                {
                    reconstructor.FeedRow(cursor);
                }
            });
        });
        return reconstructor.GetResult();
    }

    std::map<std::string, std::map<ShortTxType, int>> WebRpcRepository::GetNotificationsSummary(int64_t heightMax, int64_t heightMin, const std::set<std::string>& addresses, const std::set<ShortTxType>& filters)
    {
        struct QueryParams {
            // Handling all by reference
            const int64_t& heightMax;
            const int64_t& heightMin;
            const std::set<std::string>& addresses;
        } queryParams {heightMax, heightMin, addresses};

        const std::map<ShortTxType, ShortFormSqlEntry<Stmt&, QueryParams>> selects = {
        {
            ShortTxType::Referal, { R"sql(
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
                    ?,
                    addr.hash
                from
                    addr
                cross join    
                    Transactions t indexed by Transactions_Type_RegId2_RegId1
                        on t.Type in (100) and t.RegId2 = addr.id
                cross join
                    First f
                        on f.TxId = t.RowId
                cross join
                    Chain c
                        on c.TxId = f.TxId and c.Height between ? and ?
            )sql",
            [](Stmt& stmt, QueryParams const& queryParams) {
                stmt.Bind(queryParams.addresses, ShortTxTypeConvertor::toString(ShortTxType::Referal), queryParams.heightMin, queryParams.heightMax);
            }
        }},

        {
            ShortTxType::Comment, { R"sql(
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
                    ?,
                    addr.hash
                from
                    addr
                cross join
                    Transactions p indexed by Transactions_Type_RegId1_RegId2_RegId3
                        on p.Type in (200, 201, 202, 209, 210) and p.RegId1 = addr.id
                cross join
                    Last lp
                        on lp.TxId = p.RowId
                cross join
                    Transactions c indexed by Transactions_Type_RegId3_RegId1
                        on c.Type in (204) and c.RegId3 = p.RegId2 and c.RegId1 != p.RegId1
                            and c.RegId4 is null and c.RegId5 is null
                cross join
                    Chain cc
                        on cc.TxId = c.RowId and cc.Height between ? and ?
            )sql",
            [](Stmt& stmt, QueryParams const& queryParams) {
                stmt.Bind(queryParams.addresses, ShortTxTypeConvertor::toString(ShortTxType::Comment), queryParams.heightMin, queryParams.heightMax);
            }
        }},

        {
            ShortTxType::Subscriber, { R"sql(
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
                    ?,
                    addr.hash
                from
                    addr
                cross join
                    Transactions s indexed by Transactions_Type_RegId2_RegId1
                        on s.Type in (302, 303) and s.RegId2 = addr.id
                cross join
                    Chain c
                        on c.TxId = s.RowId and c.Height between ? and ?
            )sql",
            [](Stmt& stmt, QueryParams const& queryParams) {
                stmt.Bind(queryParams.addresses, ShortTxTypeConvertor::toString(ShortTxType::Subscriber), queryParams.heightMin, queryParams.heightMax);
            }
        }},

        {
            ShortTxType::CommentScore, { R"sql(
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
                    ?,
                    addr.hash
                from
                    addr
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
                        on cs.TxId = s.RowId and cs.Height between ? and ?
            )sql",
            [](Stmt& stmt, QueryParams const& queryParams) {
                stmt.Bind(queryParams.addresses, ShortTxTypeConvertor::toString(ShortTxType::CommentScore), queryParams.heightMin, queryParams.heightMax);
            }
        }},

        {
            ShortTxType::ContentScore, { R"sql(
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
                    ?,
                    addr.hash
                from
                    addr
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
                        on cs.TxId = s.RowId and cs.Height between ? and ?
            )sql",
            [](Stmt& stmt, QueryParams const& queryParams) {
                stmt.Bind(queryParams.addresses, ShortTxTypeConvertor::toString(ShortTxType::ContentScore), queryParams.heightMin, queryParams.heightMax);
            }
        }},

        {
            ShortTxType::Repost, { R"sql(
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
                    ?,
                    addr.hash
                from
                    addr
                cross join
                    Transactions p indexed by Transactions_Type_RegId1_RegId2_RegId3
                        on p.Type in (200) and p.RegId1 = addr.id
                cross join
                    Last lp
                        on lp.TxId = p.RowId
                cross join
                    Transactions r indexed by Transactions_Type_RegId3_RegId1
                        on r.Type in (200) and r.RegId3 = p.RegId2 and r.RegId3 is not null
                cross join
                    First fr
                        on fr.TxId = r.RowId
                cross join
                    Chain cr
                        on cr.TxId = r.RowId and cr.Height between ? and ?
            )sql",
            [](Stmt& stmt, QueryParams const& queryParams) {
                stmt.Bind(queryParams.addresses, ShortTxTypeConvertor::toString(ShortTxType::Repost), queryParams.heightMin, queryParams.heightMax);
            }
        }}
        };
        
        auto [elem1, elem2] = _constructSelectsBasedOnFilters(filters, selects, "", "union all");
        auto& sql = elem1;
        auto& binds = elem2;

        NotificationSummaryReconstructor reconstructor;
        SqlTransaction(__func__, [&]()
        {
            auto& stmt = Sql(sql);

            for (const auto& bind: binds)
                bind(stmt, queryParams);

            stmt.Select([&](Cursor& cursor)
            {
                while (cursor.Step())
                    reconstructor.FeedRow(cursor);
            });
        });

        return reconstructor.GetResult();
    }

}
