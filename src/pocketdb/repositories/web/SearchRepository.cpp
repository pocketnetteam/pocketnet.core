// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include "pocketdb/repositories/web/SearchRepository.h"

namespace PocketDb
{
    // TODO (aok, api): not used in PocketRpc
    UniValue SearchRepository::SearchTags(const SearchRequest& request)
    {
        UniValue result(UniValue::VARR);

        string keyword = "%" + request.Keyword + "%";

        SqlTransaction(
            __func__,
            [&]() -> Stmt& {
                return Sql(R"sql(
                    select Value
                    from Tags t indexed by Tags_Value
                    where t.Value match ?
                    limit ?
                    offset ?
                )sql")
                .Bind(keyword, request.PageSize, request.PageStart);
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

    vector<int64_t> SearchRepository::SearchIds(const SearchRequest& request)
    {
        vector<int64_t> ids;

        if (request.Keyword.empty())
            return ids;

        string _keyword = "\"" + request.Keyword + "\"" + " OR " + request.Keyword + "*";

        SqlTransaction(
            __func__,
            [&]() -> Stmt& {
                return Sql(R"sql(
                    with
                        keyword as (
                            select
                                cm.ContentId,
                                rank
                            from
                                web.Content c,
                                web.ContentMap cm
                            where
                                c.ROWID = cm.ROWID and
                                cm.FieldType in ( )sql" + join(request.FieldTypes | transformed(static_cast<string(*)(int)>(to_string)), ",") + R"sql( ) and
                                c.Value match ?
                            order by
                                rank
                        )
                    select
                        ct.Uid,
                        keyword.rank
                    from
                        keyword
                    cross join
                        Chain ct on --indexed by Chain_Uid_Height on
                            ct.Uid = keyword.ContentId and
                            (? or ct.Height <= ?) -- 
                    cross join
                        Transactions t on
                            ct.TxId = t.RowId and
                            t.Type in ( )sql" + join(request.TxTypes | transformed(static_cast<string(*)(int)>(to_string)), ",") + R"sql( ) and
                            (? or t.RegId1 in (
                                select
                                    RowId as id
                                from
                                    Registry
                                where
                                    String = ?
                            ))
                    cross join
                        Last lt on
                            lt.TxId = t.RowId
                    order by
                        keyword.rank desc
                    limit ?
                    offset ?
                )sql")
                .Bind(
                    _keyword,
                    !(request.TopBlock > 0),
                    request.TopBlock,
                    request.Address.empty(),
                    request.Address,
                    request.PageSize,
                    request.PageStart
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

        return ids;
    }

    vector<int64_t> SearchRepository::SearchUsers(const string& keyword)
    {
        vector<int64_t> result;

        string _keyword = "\"" + keyword + "\"" + " OR " + keyword + "*";

        SqlTransaction(
            __func__,
            [&]() -> Stmt& {
                return Sql(R"sql(
                    with
                        keyword as ( select ? as value )
                    select
                        names.*
                    from (
                        select
                            fm.ContentId,
                            ROW_NUMBER() OVER ( ORDER BY RANK, length(f.Value) ) as ROWNUMBER,
                            RANK as RNK,
                            r.Value as Rating
                        from
                            keyword,
                            web.Content f
                        cross join
                            web.ContentMap fm on
                                fm.ROWID = f.ROWID
                        left join
                            Ratings r on
                                r.Uid = fm.ContentId and
                                r.Last = 1 and
                                r.Type = 0
                        where
                            fm.FieldType in (?) and
                            f.Value match keyword.value
                        limit ?
                    ) names

                    union

                    select
                        about.*
                    from (
                        select
                            fm.ContentId,
                            ROW_NUMBER() OVER ( ORDER BY r.Value DESC) as ROWNUMBER,
                            RANK as RNK,
                            r.Value as Rating
                        from
                            keyword,
                            web.Content f
                        join
                            web.ContentMap fm on
                                fm.ROWID = f.ROWID
                        left join
                            Ratings r on
                                r.Uid = fm.ContentId and
                                r.Last = 1 and
                                r.Type = 0
                        where
                            fm.FieldType in (?) and
                            f.Value match keyword.value
                        limit ?
                    ) about

                    order by ROWNUMBER, RNK, Rating desc
                )sql")
                .Bind(
                    _keyword,
                    (int)ContentFieldType::ContentFieldType_AccountUserName,
                    10,
                    (int)ContentFieldType::ContentFieldType_AccountUserAbout,
                    10
                );
            },
            [&] (Stmt& stmt) {
                stmt.Select([&](Cursor& cursor) {
                    while (cursor.Step())
                    {
                        if (auto[ok, value] = cursor.TryGetColumnInt64(0); ok)
                            if (find(result.begin(), result.end(), value) == result.end())
                                result.push_back(value);
                    }
                });
            }
        );

        return result;
    }

    vector<string> SearchRepository::GetRecommendedAccountByAddressSubscriptions(const string& address, string& addressExclude, const vector<int>& contentTypes,
        const string& lang, int cntOut, int nHeight, int depth)
    {
        vector<string> ids;

        if (address.empty())
            return ids;

        int minReputation = 300;
        int limitSubscriptions = 30;
        int limitSubscriptionsTotal = 30;
        int cntRates = 1;

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
                        addrs as (
                            select
                                id,
                                rnk
                            from (
                                select
                                    1 as rnk,
                                    subscribes.RegId2 as id
                                from
                                    addr
                                cross join
                                    Transactions subscribes indexed by Transactions_Type_RegId1_RegId2_RegId3 on
                                        subscribes.Type in (302, 303) and
                                        subscribes.RegId1 = addr.id
                                cross join
                                    Last lSubscribes on
                                        lSubscribes.TxId = subscribes.RowId
                                cross join
                                    Transactions u indexed by Transactions_Type_RegId1_RegId2_RegId3 on
                                        u.Type in (100) and
                                        u.RegId1 = subscribes.RegId2
                                cross join
                                    Last lu on
                                        lu.TxId = u.RowId
                                cross join
                                    Chain cu on
                                        cu.TxId = u.RowId
                                cross join
                                    Ratings r indexed by Ratings_Type_Uid_Last_Value on
                                        r.Uid = cu.Uid and
                                        r.Type = 0 and
                                        r.Last = 1 and
                                        r.Value > ?
                                order by
                                    subscribes.RowId desc
                                limit ?
                            )

                            union

                            select
                                id,
                                rnk
                            from (
                                select
                                    2 as rnk,
                                    subscribers.RegId1 as id
                                from
                                    addr
                                cross join
                                    Transactions subscribers indexed by Transactions_Type_RegId2_RegId1 on
                                        subscribers.Type in (302, 303) and
                                        subscribers.RegId2 = addr.id
                                cross join
                                    Last lSubscribers on
                                        lSubscribers.TxId = subscribers.RowId
                                cross join
                                    Transactions u indexed by Transactions_Type_RegId1_RegId2_RegId3 on
                                        u.Type in (100) and
                                        u.RegId1 = subscribers.RegId1
                                cross join
                                    Last lu on
                                        lu.TxId = u.RowId
                                cross join
                                    Chain cu on
                                        cu.TxId = u.RowId
                                cross join
                                    Ratings r indexed by Ratings_Type_Uid_Last_Value on
                                        r.Uid = cu.Uid and
                                        r.Type = 0 and
                                        r.Last = 1 and
                                        r.Value > ?
                                order by
                                    random()
                                limit ?
                            )

                            order by
                                rnk
                            limit ?
                        )
                    select
                        (select String from Registry r where r.RowId = Contents.RegId1)
                    from
                        addrs
                    cross join
                        Transactions Rates indexed by Transactions_Type_RegId1_RegId2_RegId3 on
                            Rates.Type in (300) and
                            Rates.Int1 = 5 and
                            Rates.RegId1 = addrs.id
                    cross join
                        Chain cRates on
                            cRates.TxId = Rates.RowId
                    cross join Transactions Contents indexed by Transactions_Type_RegId2_RegId1 and
                            Contents.RegId2 = Rates.RegId2 and
                            Contents.Type in ( )sql" + join(vector<string>(contentTypes.size(), "?"), ",") + R"sql( ) and
                            (? or Contents.RegId1 not in (
                                select
                                    RowId
                                from
                                    Registry
                                where
                                    String = ?
                            ))
                    cross join
                        Last lContents on
                            lContents.TxId = Contents.RowId
                    cross join
                        Transactions u indexed by Transactions_Type_RegId1_RegId2_RegId3 on
                            u.Type in (100) and
                            u.RegId1 = Contents.RegId1
                    cross join
                        Last lu on
                            lu.TxId = u.RowId
                    cross join
                        Payload lang on
                            lang.TxId = u.RowId and
                            (? or lang.String1 = ?)
                    group by
                        Contents.RegId1
                    having
                        count() > ?
                    order by
                        count() desc
                    limit ?
                )sql")
                .Bind(
                    address,
                    minReputation,
                    limitSubscriptions,
                    minReputation,
                    limitSubscriptions,
                    limitSubscriptionsTotal,
                    contentTypes,
                    addressExclude.empty(),
                    addressExclude,
                    lang.empty(),
                    lang,
                    cntRates,
                    cntOut
                );
            },
            [&] (Stmt& stmt) {
                stmt.Select([&](Cursor& cursor) {
                    while (cursor.Step())
                    {
                        if (auto[ok, value] = cursor.TryGetColumnString(0); ok)
                            ids.push_back(value);
                    }
                });
            }
        );

        return ids;
    }

    vector<int64_t> SearchRepository::GetRecommendedContentByAddressSubscriptions(const string& contentAddress, string& addressExclude, const vector<int>& contentTypes,
        const string& lang, int cntOut, int nHeight, int depth)
    {
        vector<int64_t> ids;

        if (contentAddress.empty())
            return ids;

        int minReputation = 300;
        int limitSubscriptions = 50;
        int limitSubscriptionsTotal = 30;
        int cntRates = 1;

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
                        addrs as (
                            select
                                id,
                                rnk
                            from (
                                select
                                    1 as rnk,
                                    subscribes.RegId2 as id
                                from
                                    addr
                                cross join
                                    Transactions subscribes indexed by Transactions_Type_RegId1_RegId2_RegId3 on
                                        subscribes.Type in (302, 303) and
                                        subscribes.RegId1 = addr.id
                                cross join
                                    Last lSubscribes on
                                        lSubscribes.TxId = subscribes.RowId
                                cross join
                                    Transactions u indexed by Transactions_Type_RegId1_RegId2_RegId3 on
                                        u.Type in (100) and
                                        u.RegId1 = subscribes.RegId2
                                cross join
                                    Last lu on
                                        lu.TxId = u.RowId
                                cross join
                                    Chain cu on
                                        cu.TxId = u.RowId
                                cross join
                                    Ratings r indexed by Ratings_Type_Uid_Last_Value on
                                        r.Uid = cu.Uid and
                                        r.Type = 0 and
                                        r.Last = 1 and
                                        r.Value > ?
                                order by
                                    subscribes.RowId desc
                                limit ?
                            )

                            union

                            select
                                id,
                                rnk
                            from (
                                select
                                    2 as rnk,
                                    subscribers.RegId1 as id
                                from
                                    addr
                                cross join
                                    Transactions subscribers indexed by Transactions_Type_RegId2_RegId1 on
                                        subscribers.Type in (302, 303) and
                                        subscribers.RegId2 = addr.id
                                cross join
                                    Last lSubscribers on
                                        lSubscribers.TxId = subscribers.RowId
                                cross join
                                    Transactions u indexed by Transactions_Type_RegId1_RegId2_RegId3 on
                                        u.Type in (100) and
                                        u.RegId1 = subscribers.RegId1
                                cross join
                                    Last lu on
                                        lu.TxId = u.RowId
                                cross join
                                    Chain cu on
                                        cu.TxId = u.RowId
                                cross join
                                    Ratings r indexed by Ratings_Type_Uid_Last_Value on
                                        r.Uid = cu.Uid and
                                        r.Type = 0 and
                                        r.Last = 1 and
                                        r.Value > ?
                                order by
                                    random()
                                limit ?
                            )

                            order by
                                rnk
                            limit ?
                        )
                    select
                        recomendations.Uid
                    from (
                        select
                            Contents.RegId1,
                            cContents.Uid,
                            count() as count
                        from
                            addrs
                        cross join
                            Transactions Rates indexed by Transactions_Type_RegId1_RegId2_RegId3 on
                                Rates.Type in (300) and
                                Rates.Int1 = 5 and
                                Rates.RegId1 = addrs.id
                        cross join
                            Chain cRates indexed by Chain_TxId_Height on
                                cRates.TxId = Rates.RowId and
                                cRates.Height > ?
                        cross join Transactions Contents indexed by Transactions_Type_RegId2_RegId1 on
                                Contents.RegId2 = Rates.RegId2 and
                                Contents.Type in ( )sql" + join(vector<string>(contentTypes.size(), "?"), ",") + R"sql( ) and
                                (? or Contents.RegId1 not in (
                                    select
                                        RowId as id
                                    from
                                        Registry
                                    where
                                        String = ?
                                ))
                        cross join
                            Last lContents on
                                lContents.TxId = Contents.RowId
                        cross join
                            Chain cContents on
                                cContents.TxId = Contents.RowId
                        cross join
                            Transactions u indexed by Transactions_Type_RegId1_RegId2_RegId3 on
                                u.Type in (100) and
                                u.RegId1 = Contents.RegId1
                        cross join
                            Last lu on
                                lu.TxId = u.RowId
                        cross join
                            Payload lang on
                                lang.TxId = u.RowId and
                                (? or lang.String1 = ?)
                        group by
                            cContents.Uid
                        having count() > ?
                        order by
                            count() desc
                    ) recomendations
                    group by
                        recomendations.RegId1
                    order by
                        recomendations.count desc
                    limit ?
                )sql")
                .Bind(
                    contentAddress,
                    minReputation,
                    limitSubscriptions,
                    minReputation,
                    limitSubscriptions,
                    limitSubscriptionsTotal,
                    nHeight - depth,
                    contentTypes,
                    addressExclude.empty(),
                    addressExclude,
                    lang.empty(),
                    lang,
                    cntRates,
                    cntOut
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

        return ids;
    }

    vector<int64_t> SearchRepository::GetRandomContentByAddress(const string& contentAddress, const vector<int>& contentTypes, const string& lang, int cntOut)
    {
        vector<int64_t> ids;

        if (contentAddress.empty())
            return ids;

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
                        )
                    select
                        cc.Uid
                    from
                        addr
                    cross join
                        Transactions c indexed by Transactions_Type_RegId1_RegId2_RegId3 on
                            c.Type in ( )sql" + join(vector<string>(contentTypes.size(), "?"), ",") + R"sql( ) and
                            c.RegId1 = addr.id
                    cross join
                        Last lc on
                            lc.TxId = c.RowId
                    cross join
                        Chain cc on
                            cc.TxId = c.RowId
                    cross join
                        Payload lang on
                            lang.TxId = c.RowId and
                            (? or lang.String1 = ?)
                    order by
                        random()
                    limit ?
                )sql")
                .Bind(
                    contentAddress,
                    contentTypes,
                    lang.empty(),
                    lang,
                    cntOut
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

        return ids;
    }

    vector<int64_t> SearchRepository::GetContentFromAddressSubscriptions(const string& address, const vector<int>& contentTypes, const string& lang, int cntOut, bool rest)
    {
        vector<int64_t> ids;

        if (address.empty())
            return ids;

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
                        content as (
                            select
                                cc.Uid,
                                row_number() over (partition by c.RegId1 order by r.Value desc ) as rowNumber
                            from
                                addr
                            cross join
                                Transactions s indexed by Transactions_Type_RegId1_RegId2_RegId3 on
                                    s.Type in (302, 303) and
                                    s.RegId1 = addr.id
                            cross join
                                Last ls on
                                    ls.TxId = s.RowId
                            cross join
                                Transactions c indexed by Transactions_Type_RegId1_RegId2_RegId3 on
                                    c.Type in ( )sql" + join(vector<string>(contentTypes.size(), "?"), ",") + R"sql( ) and
                                    c.RegId1 = s.RegId2
                            cross join
                                Last lc on
                                    lc.TxId = c.RowId
                            cross join
                                Chain cc on
                                    cc.TxId = c.RowId
                            cross join Ratings r on
                                r.Uid = cc.Uid and
                                r.Last = 1 and
                                r.Type = 2
                            cross join
                                Payload lang on
                                    lang.TxId = c.RowId and
                                    (? or lang.String1 = ?)
                        )
                    select
                        content.Uid
                    from
                        content
                    where
                        content.rowNumber )sql" + (rest ? "=" : "<=") + R"sql(
                            floor(10 / (
                                select
                                    count(s.RegId2)
                                from
                                    addr
                                cross join
                                    Transactions s indexed by Transactions_Type_RegId1_RegId2_RegId3 on
                                        s.Type in (302, 303) and
                                        s.RegId1 = addr.id
                                cross join
                                    Last ls on
                                        ls.TxId = s.RowId
                            )) )sql" + (rest ? "+ 1" : "") + R"sql(
                    limit ?
                )sql")
                .Bind(
                    address,
                    contentTypes,
                    lang.empty(),
                    lang,
                    (rest ? cntOut : 9999)
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

        return ids;
    }
}