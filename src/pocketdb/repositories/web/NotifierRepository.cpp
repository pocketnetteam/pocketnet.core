// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include "NotifierRepository.h"
#include "pocketdb/helpers/ShortFormRepositoryHelper.h"


namespace PocketDb
{
    UniValue NotifierRepository::GetAccountInfoByAddress(const string& address)
    {
        UniValue result(UniValue::VOBJ);

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
                            r.String in (?)
                    )
                    select
                        addr.hash,
                        p.String2 as Name,
                        p.String3 as Avatar
                    from
                        addr
                        join Transactions u indexed by Transactions_Type_RegId1_RegId2_RegId3 on
                            u.Type in (100) and u.RegId1 = addr.id
                        join Last l on
                            l.TxId = u.RowId
                        join Payload p on
                            p.TxId = u.RowId
                )sql")
                .Bind(address);
            },
            [&](Stmt& stmt) {
                stmt.Select([&](Cursor& cursor) {
                    if (cursor.Step())
                    {
                        if (auto[ok, value] = cursor.TryGetColumnString(0); ok) result.pushKV("address", value);
                        if (auto[ok, value] = cursor.TryGetColumnString(1); ok) result.pushKV("name", value);
                        if (auto[ok, value] = cursor.TryGetColumnString(2); ok) result.pushKV("avatar", value);
                    }
                });
            }
        );

        return result;
    }

    UniValue NotifierRepository::GetPostLang(const string &postHash)
    {
        UniValue result(UniValue::VOBJ);

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
                        p.String1 Lang
                    from
                        tx
                    cross join Transactions t on
                        t.RowId = tx.id and
                        t.Type in (200, 201, 202, 209, 210, 203)
                    cross join Payload p on
                        p.TxId = t.RowId
                )sql")
                .Bind(postHash);
            },
            [&](Stmt& stmt) {
                stmt.Select([&](Cursor& cursor) {
                    if (cursor.Step())
                    {
                        if (auto[ok, value] = cursor.TryGetColumnString(0); ok) result.pushKV("lang", value);
                    }
                });
            }
        );

        return result;
    }

    UniValue NotifierRepository::GetPostInfo(const string& postHash)
    {
        UniValue result(UniValue::VOBJ);

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
                        tx.hash,
                        (select r.String from Registry r where r.RowId = t.RegId2)
                    from
                        tx
                        join Transactions t on
                            t.RowId = tx.id and t.Type in (200, 201, 202, 209, 210, 203)
                )sql")
                .Bind(postHash);
            },
            [&](Stmt& stmt) {
                stmt.Select([&](Cursor& cursor) {
                    if (cursor.Step())
                    {
                        if (auto[ok, value] = cursor.TryGetColumnString(0); ok) result.pushKV("hash", value);
                        if (auto[ok, value] = cursor.TryGetColumnString(1); ok) result.pushKV("rootHash", value);
                    }
                });
            }
        );

        return result;
    }

    UniValue NotifierRepository::GetBoostInfo(const string& boostHash)
    {
        UniValue result(UniValue::VOBJ);

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
                        tx.hash,
                        (select r.String from Registry r where r.RowId = tBoost.RegId1) as boostAddress,
                        tBoost.Int1 as boostAmount,
                        p.String2 as boostName,
                        p.String3 as boostAvatar,
                        (select r.String from Registry r where r.RowId = tContent.RegId1) as contentAddress,
                        (select r.String from Registry r where r.RowId = tContent.RegId2) as contentHash
                    from
                        tx
                        join Transactions tBoost on
                            tBoost.RowId = tx.id and tBoost.Type in (208)
                        join Transactions tContent indexed by Transactions_Type_RegId2_RegId1 on
                            tContent.RegId2 = tBoost.RegId2 and tContent.Type in (200, 201, 202, 209, 210)
                        join Last lc on
                            lc.TxId = tContent.RowId
                        join Transactions u indexed by Transactions_Type_RegId1_RegId2_RegId3 on
                            u.Type in (100) and u.RegId1 = tBoost.RegId1
                        join Last lu on
                            lu.TxId = u.RowId
                        join Payload p on
                            p.TxId = u.RowId
                )sql")
                .Bind(boostHash);
            },
            [&](Stmt& stmt) {
                stmt.Select([&](Cursor& cursor) {
                    if (cursor.Step())
                    {
                        if (auto[ok, value] = cursor.TryGetColumnString(0); ok) result.pushKV("hash", value);
                        if (auto[ok, value] = cursor.TryGetColumnString(1); ok) result.pushKV("boostAddress", value);
                        if (auto[ok, value] = cursor.TryGetColumnInt(2); ok) result.pushKV("boostAmount", to_string(value));
                        if (auto[ok, value] = cursor.TryGetColumnString(3); ok) result.pushKV("boostName", value);
                        if (auto[ok, value] = cursor.TryGetColumnString(4); ok) result.pushKV("boostAvatar", value);
                        if (auto[ok, value] = cursor.TryGetColumnString(5); ok) result.pushKV("contentAddress", value);
                        if (auto[ok, value] = cursor.TryGetColumnString(6); ok) result.pushKV("contentHash", value);
                    }
                });
            }
        );

        return result;
    }

    UniValue NotifierRepository::GetOriginalPostAddressByRepost(const string &repostHash)
    {
        UniValue result(UniValue::VOBJ);

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
                        (select r.String from Registry r where r.RowId = t.RegId2) as RootTxHash,
                        (select r.String from Registry r where r.RowId = t.RegId1) as address,
                        (select r.String from Registry r where r.RowId = tRepost.RegId1) as addressRepost,
                        p.String2 as nameRepost,
                        p.String3 as avatarRepost
                    from
                        tx
                        join Transactions tRepost on
                            tRepost.RowId = tx.id and tRepost.Type in (200, 201, 202, 209, 210, 203)
                        join Transactions t on
                            t.RowId = tRepost.RegId3
                        join Transactions u indexed by Transactions_Type_RegId1_RegId2_RegId3 on
                            u.Type in (100) and u.RegId1 = tRepost.RegId1
                        join Last lu on
                            lu.TxId = u.RowId
                        join Payload p on
                            p.TxId = u.RowId
                )sql")
                .Bind(repostHash);
            },
            [&](Stmt& stmt) {
                stmt.Select([&](Cursor& cursor) {
                    if (cursor.Step())
                    {
                        if (auto[ok, value] = cursor.TryGetColumnString(0); ok) result.pushKV("hash", value);
                        if (auto[ok, value] = cursor.TryGetColumnString(1); ok) result.pushKV("address", value);
                        if (auto[ok, value] = cursor.TryGetColumnString(2); ok) result.pushKV("addressRepost", value);
                        if (auto[ok, value] = cursor.TryGetColumnString(3); ok) result.pushKV("nameRepost", value);
                        if (auto[ok, value] = cursor.TryGetColumnString(4); ok) result.pushKV("avatarRepost", value);
                    }
                });
            }
        );

        return result;
    }

    UniValue NotifierRepository::GetPrivateSubscribeAddressesByAddressTo(const string &addressTo)
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
                        (select r.String from Registry r where r.RowId = s.RegId1) as addressTo,
                        p.String2 as nameFrom,
                        p.String3 as avatarFrom
                    from
                        addr
                        join Transactions s indexed by Transactions_Type_RegId2_RegId1 on
                            s.Type in (303) and s.RegId2 = addr.id
                        join Last ls on
                            ls.TxId = s.RowId
                        join Transactions u indexed by Transactions_Type_RegId1_RegId2_RegId3 on
                            u.Type in (100) and u.RegId1 = addr.id
                        join Last lu on
                            lu.TxId = u.RowId
                        join Payload p on
                            p.TxId = u.RowId
                )sql")
                .Bind(addressTo);
            },
            [&](Stmt& stmt) {
                stmt.Select([&](Cursor& cursor) {
                    while (cursor.Step())
                    {
                        UniValue record(UniValue::VOBJ);
                        if (auto[ok, value] = cursor.TryGetColumnString(0); ok) record.pushKV("addressTo", value);
                        if (auto[ok, value] = cursor.TryGetColumnString(1); ok) record.pushKV("nameFrom", value);
                        if (auto[ok, value] = cursor.TryGetColumnString(2); ok) record.pushKV("avatarFrom", value);
                        result.push_back(record);
                    }
                });
            }
        );

        return result;
    }

    UniValue NotifierRepository::GetPostInfoAddressByScore(const string &postScoreHash)
    {
        UniValue result(UniValue::VOBJ);

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
                        (select r.String from Registry r where r.RowId = score.RegId2) as postTxHash,
                        score.Int1 value,
                        (select r.String from Registry r where r.RowId = post.RegId1) as postAddress,
                        p.String2 as scoreName,
                        p.String3 as scoreAvatar
                    from
                        tx
                        join Transactions score on
                            score.RowId = tx.id and score.Type in (300)
                        join Transactions post on
                            post.RowId = score.RegId2
                        join Transactions u indexed by Transactions_Type_RegId1_RegId2_RegId3 on
                            u.Type in (100) and u.RegId1 = score.RegId1
                        join Last lu on
                            lu.TxId = u.RowId
                        join Payload p on
                            p.TxId = u.RowId
                )sql")
                .Bind(postScoreHash);
            },
            [&](Stmt& stmt) {
                stmt.Select([&](Cursor& cursor) {
                    if (cursor.Step())
                    {
                        if (auto[ok, value] = cursor.TryGetColumnString(0); ok) result.pushKV("postTxHash", value);
                        if (auto[ok, value] = cursor.TryGetColumnInt(1); ok) result.pushKV("value", to_string(value));
                        if (auto[ok, value] = cursor.TryGetColumnString(2); ok) result.pushKV("postAddress", value);
                        if (auto[ok, value] = cursor.TryGetColumnString(3); ok) result.pushKV("scoreName", value);
                        if (auto[ok, value] = cursor.TryGetColumnString(4); ok) result.pushKV("scoreAvatar", value);
                    }
                });
            }
        );

        return result;
    }

    UniValue NotifierRepository::GetSubscribeAddressTo(const string &subscribeHash)
    {
        UniValue result(UniValue::VOBJ);

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
                        (select r.String from Registry r where r.RowId = s.RegId2) addressTo,
                        p.String2 as nameFrom,
                        p.String3 as avatarFrom
                    from
                        tx
                        join Transactions s on
                            s.RowId = tx.id and s.Type in (302, 303, 304)
                        join Transactions u indexed by Transactions_Type_RegId1_RegId2_RegId3
                            on u.Type in (100) and u.RegId1 = s.RegId1
                        join Last lu on
                            lu.TxId = u.RowId
                        join Payload p on
                            p.TxId = u.RowId
                )sql")
                .Bind(subscribeHash);
            },
            [&](Stmt& stmt) {
                stmt.Select([&](Cursor& cursor) {
                    if (cursor.Step())
                    {
                        if (auto[ok, value] = cursor.TryGetColumnString(0); ok) result.pushKV("addressTo", value);
                        if (auto[ok, value] = cursor.TryGetColumnString(1); ok) result.pushKV("nameFrom", value);
                        if (auto[ok, value] = cursor.TryGetColumnString(2); ok) result.pushKV("avatarFrom", value);
                    }
                });
            }
        );

        return result;
    }

    UniValue NotifierRepository::GetCommentInfoAddressByScore(const string &commentScoreHash)
    {
        UniValue result(UniValue::VOBJ);

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
                        (select r.String from Registry r where r.RowId = score.RegId2) as commentHash,
                        score.Int1 value,
                        (select r.String from Registry r where r.RowId = comment.RegId1) as commentAddress,
                        p.String2 as scoreCommentName,
                        p.String3 as scoreCommentAvatar
                    from
                        tx
                        join Transactions score on
                            score.RowId = tx.id
                        join Transactions comment on
                            comment.RowId = score.RegId2
                        join Transactions u indexed by Transactions_Type_RegId1_RegId2_RegId3 on
                            u.Type in (100) and u.RegId1 = score.RegId1
                        join Last l on
                            l.TxId = u.RowId
                        join Payload p on
                            p.TxId = u.RowId
                )sql")
                .Bind(commentScoreHash);
            },
            [&](Stmt& stmt) {
                stmt.Select([&](Cursor& cursor) {
                    if (cursor.Step())
                    {
                        if (auto[ok, value] = cursor.TryGetColumnString(0); ok) result.pushKV("commentHash", value);
                        if (auto[ok, value] = cursor.TryGetColumnInt(1); ok) result.pushKV("value", to_string(value));
                        if (auto[ok, value] = cursor.TryGetColumnString(2); ok) result.pushKV("commentAddress", value);
                        if (auto[ok, value] = cursor.TryGetColumnString(3); ok) result.pushKV("scoreCommentName", value);
                        if (auto[ok, value] = cursor.TryGetColumnString(4); ok) result.pushKV("scoreCommentAvatar", value);
                    }
                });
            }
        );

        return result;
    }

    UniValue NotifierRepository::GetFullCommentInfo(const string &commentHash)
    {
        UniValue result(UniValue::VOBJ);

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
                        (select r.String from Registry r where r.RowId = comment.RegId3) as PostHash,
                        (select r.String from Registry r where r.RowId = comment.RegId4) as ParentHash,
                        (select r.String from Registry r where r.RowId = comment.RegId5) as AnswerHash,
                        (select r.String from Registry r where r.RowId = comment.RegId2) as RootHash,
                        (select r.String from Registry r where r.RowId = content.RegId1) as ContentAddress,
                        (select r.String from Registry r where r.RowId = answer.RegId1) as AnswerAddress,
                        p.String2 as commentName,
                        p.String3 as commentAvatar,
                        (
                            select
                                o.Value
                            from
                                TxOutputs o indexed by TxOutputs_AddressId_TxIdDesc_Number
                            where
                                o.AddressId = content.RegId1 and
                                o.TxId = comment.RowId and
                                o.AddressId != comment.RegId1
                            limit 1
                        ) as Donate
                    from
                        tx
                        join Transactions comment on
                            comment.RowId = tx.id and comment.Type in (204, 205)
                        join Transactions u indexed by Transactions_Type_RegId1_RegId2_RegId3 on
                            u.Type in (100) and u.RegId1 = comment.RegId1
                        join Last lu on
                            lu.TxId = u.RowId
                        join Payload p on
                            p.TxId = u.RowId
                        join Transactions content on
                            content.RowId = comment.RegId3 and content.Type in (200, 201, 202, 209, 210)
                        left join Transactions answer indexed by Transactions_Type_RegId2_RegId1 on
                            answer.Type in (204, 205) and answer.RegId2 = comment.RegId5
                )sql")
                .Bind(commentHash);
            },
            [&](Stmt& stmt) {
                stmt.Select([&](Cursor& cursor) {
                    if (cursor.Step())
                    {
                        if (auto[ok, value] = cursor.TryGetColumnString(0); ok) result.pushKV("postHash", value);
                        if (auto[ok, value] = cursor.TryGetColumnString(1); ok) result.pushKV("parentHash", value); else result.pushKV("parentHash", "");
                        if (auto[ok, value] = cursor.TryGetColumnString(2); ok) result.pushKV("answerHash", value); else result.pushKV("answerHash", "");
                        if (auto[ok, value] = cursor.TryGetColumnString(3); ok) result.pushKV("rootHash", value);
                        if (auto[ok, value] = cursor.TryGetColumnString(4); ok) result.pushKV("postAddress", value);
                        if (auto[ok, value] = cursor.TryGetColumnString(5); ok) result.pushKV("answerAddress", value);
                        if (auto[ok, value] = cursor.TryGetColumnString(6); ok) result.pushKV("commentName", value);
                        if (auto[ok, value] = cursor.TryGetColumnString(7); ok) result.pushKV("commentAvatar", value);
                        if (auto[ok, value] = cursor.TryGetColumnInt64(8); ok)
                        {
                            result.pushKV("donation", "true");
                            result.pushKV("amount", value);
                        }
                    }
                });
            }
        );

        return result;
    }

    UniValue NotifierRepository::GetPostCountFromMySubscribes(const string& address, int height)
    {
        UniValue result(UniValue::VOBJ);

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
                        count() as cntTotal,
                        sum(ifnull((case when post.Type = 200 then 1 else 0 end),0)) as cntPost,
                        sum(ifnull((case when post.Type = 201 then 1 else 0 end),0)) as cntVideo,
                        sum(ifnull((case when post.Type = 202 then 1 else 0 end),0)) as cntArticle,
                        sum(ifnull((case when post.Type = 209 then 1 else 0 end),0)) as cntStream,
                        sum(ifnull((case when post.Type = 210 then 1 else 0 end),0)) as cntAudio
                    from
                        addr
                        cross join Transactions sub indexed by Transactions_Type_RegId1_RegId2_RegId3 on
                            sub.Type in (302, 303) and sub.RegId1 = addr.id
                        cross join Last lsub on
                            lsub.TxId = sub.RowId
                        cross join Chain cpost indexed by Chain_Height_Uid on
                            cpost.Height = ?
                        cross join Transactions post on
                            post.RowId = cpost.TxId and post.Type in (200, 201, 202, 209, 210, 203) and post.RegId1 = sub.RegId2
                        cross join Last lpost on
                            lpost.TxId = post.RowId
                )sql")
                .Bind(address, height);
            },
            [&](Stmt& stmt) {
                stmt.Select([&](Cursor& cursor) {
                    if (cursor.Step())
                    {
                        if (auto[ok, value] = cursor.TryGetColumnInt(0); ok) result.pushKV("cntTotal", value);
                        if (auto[ok, value] = cursor.TryGetColumnInt(1); ok) result.pushKV("cntPost", value);
                        if (auto[ok, value] = cursor.TryGetColumnInt(2); ok) result.pushKV("cntVideo", value);
                        if (auto[ok, value] = cursor.TryGetColumnInt(3); ok) result.pushKV("cntArticle", value);
                        if (auto[ok, value] = cursor.TryGetColumnInt(4); ok) result.pushKV("cntStream", value);
                        if (auto[ok, value] = cursor.TryGetColumnInt(5); ok) result.pushKV("cntAudio", value);
                    }
                });
            }
        );

        return result;
    }


    // Choosing predicate for function above based on filters.
    function<bool(const ShortTxType&)> _choosePredicate(const set<ShortTxType>& filters) {
        if (filters.empty()) {
            // No filters mean that we should perform all selects
            return [&filters](...) { return true; };
        } else {
            // Perform only selects that are specified in filters.
            return [&filters](const ShortTxType& select) { return filters.find(select) != filters.end(); };
        }
    };

    // Method used to construct sql query and required bindings from provided selects based on filters
    static inline string _unionSelectsBasedOnFilters(
                const set<ShortTxType>& filters,
                const map<ShortTxType, string>& selects,
                const string& header,
                const string& footer, const string& separator = "union")
    {
        auto predicate = _choosePredicate(filters);

        // Query elemets that will be used to construct full query
        vector<string> queryElems;
        for (const auto& select: selects) {
            if (predicate(select.first)) {
                queryElems.emplace_back(select.second);
                queryElems.emplace_back(separator);
            }
        }

        if (queryElems.empty()) {
            throw runtime_error("Failed to construct query for requested filters");
        }
        queryElems.pop_back(); // Dropping last "union"

        stringstream ss;
        ss << header;
        for (const auto& elem: queryElems) {
            ss << elem;
        }
        ss << footer;

        // Full query and required binds in correct order
        return ss.str();
    }

    // Method used to construct sql query and required bindings from provided selects based on filters
    static inline tuple<bool, string> _constructSelectsBasedOnFilters(
                const set<ShortTxType>& filters,
                const pair<ShortTxType, string>& select,
                const string& header)
    {
        auto predicate = _choosePredicate(filters);

        // Query elemets that will be used to construct full query
        if (!predicate(select.first))
            return { false, "" };
        
        stringstream ss;
        ss << header << select.second;

        // Full query and required binds in correct order
        return { true, ss.str() };
    }

    vector<ShortForm> NotifierRepository::GetActivities(const string& address, int64_t heightMax, int64_t heightMin, int64_t blockNumMax, const set<ShortTxType>& filters)
    {
        static const map<ShortTxType, string> selects = {
            // ShortTxType::Answer
            {
                ShortTxType::Answer, R"sql(
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
                        params,
                        Transactions a -- My comments

                        cross join vTxStr sa on
                            sa.RowId = a.RowId

                        cross join Chain ca on
                            ca.TxId = a.RowId and
                            ca.Height > params.min and
                            (ca.Height < params.max or (ca.Height = params.max and ca.BlockNum < params.blockNum))

                        cross join Transactions c indexed by Transactions_Type_RegId2_RegId1 on -- Other answers
                            c.Type in (204, 205) and
                            c.RegId2 = a.RegId5 and
                            c.RegId1 != a.RegId1 and
                            exists (select 1 from Last l where l.TxId = c.RowId)

                        cross join Chain cc on
                            cc.TxId = c.RowId

                        cross join vTxStr sc on
                            sc.RowId = c.RowId

                        left join Payload pc on
                            pc.TxId = c.RowId

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
                        a.Type in (204, 205, 206) and
                        a.RegId1 = params.addressId

                )sql"
            },

            // ShortTxType::Comment
            {
                ShortTxType::Comment, R"sql(
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
                    params,
                    Transactions c
                    
                    cross join vTxStr sc on
                        sc.RowId = c.RowId
                        
                    cross join Chain cc on
                        cc.TxId = c.RowId and
                        cc.Height > params.min and
                        (cc.Height < params.max or (cc.Height = params.max and cc.BlockNum < params.blockNum))

                    cross join Transactions p indexed by Transactions_Type_RegId2_RegId1 on
                        p.Type in (200,201,202,209,210) and
                        p.RegId2 = c.RegId3 and
                        p.RegId1 != c.RegId1 and
                        exists (select 1 from Last l where l.TxId = p.RowId)

                    cross join Chain cp on
                        cp.TxId = p.RowId

                    cross join vTxStr sp on
                        sp.RowId = p.RowId

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
                    c.Type in (204, 205, 206) and
                    c.RegId4 is null and
                    c.RegId5 is null and
                    c.RegId1 = params.addressId

                )sql"
            },

            // ShortTxType::Subscriber
            {
                ShortTxType::Subscriber, R"sql(
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
                    params,
                    Transactions subs indexed by Transactions_Type_RegId1_RegId2_RegId3

                    cross join Chain csubs on
                        csubs.TxId = subs.RowId and
                        csubs.Height > params.min and
                        (csubs.Height < params.max or (csubs.Height = params.max and csubs.BlockNum < params.max))

                    cross join vTxStr ssubs on
                        ssubs.RowId = subs.RowId

                    cross join Transactions u indexed by Transactions_Type_RegId1_RegId2_RegId3 on
                        u.Type in (100) and
                        u.RegId1 = subs.RegId2 and
                        exists (select 1 from Last l where l.TxId = u.RowId)

                    cross join Chain cu on
                        cu.TxId = u.RowId

                    cross join vTxStr su on
                        su.RowId = u.RowId

                    left join Payload pu on
                        pu.TxId = u.RowId

                    left join Ratings ru indexed by Ratings_Type_Uid_Last_Height on
                        ru.Type = 0 and
                        ru.Uid = cu.Uid and
                        ru.Last = 1

                where
                    subs.Type in (302, 303, 304) and
                    subs.RegId1 = params.addressId

                )sql"
            },

            // ShortTxType::CommentScore
            {
                ShortTxType::CommentScore, R"sql(
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
                    params,
                    Transactions s indexed by Transactions_Type_RegId1_RegId2_RegId3 -- TODO (optimization): only (Type)

                    cross join Chain cs on
                        cs.TxId = s.RowId and
                        cs.Height > params.min and
                        (cs.Height < params.max or (cs.Height = params.max and cs.BlockNum < params.blockNum))

                    cross join vTxStr ss on
                        ss.RowId = s.RowId
                        
                    cross join Transactions c indexed by Transactions_Type_RegId2_RegId1 on
                        c.Type in (204,205) and
                        c.RegId2 = s.RegId2 and
                        exists (select 1 from Last l where l.TxId = c.RowId)

                    cross join Chain cc on
                        cc.TxId = c.RowId

                    cross join vTxStr sc on
                        sc.RowId = c.RowId

                    left join Payload pc on
                        pc.TxId = c.RowId

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
                    s.Type = 301 and
                    s.RegId1 = params.addressId

            )sql"
            },

            // ShortTxType::ContentScore
            {
                ShortTxType::ContentScore, R"sql(
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
                    params,
                    Transactions s indexed by Transactions_Type_RegId1_RegId2_RegId3

                    cross join Chain cs on
                        cs.TxId = s.RowId and
                        cs.Height > params.min and
                        (cs.Height < params.max or (cs.Height = params.max and cs.BlockNum < params.blockNum))

                    cross join vTxStr ss on
                        ss.RowId = s.RowId

                    cross join Transactions c indexed by Transactions_Type_RegId2_RegId1 on
                        c.Type in (200,201,202,209,210) and
                        c.RegId2 = s.RegId2 and
                        exists (select 1 from Last l where l.TxId = c.RowId)

                    cross join Chain cc on
                        cc.TxId = c.RowId

                    cross join vTxStr sc on
                        sc.RowId = c.RowId

                    left join Payload pc on
                        pc.TxId = c.RowId

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
                    s.Type = 300 and
                    s.RegId1 = params.addressId

            )sql"
            },

            // ShortTxType::Boost
            {
                ShortTxType::Boost, R"sql(
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
                    params,
                    Transactions tBoost indexed by Transactions_Type_RegId1_RegId2_RegId3

                    cross join Chain cBoost on
                        cBoost.TxId = tBoost.RowId and
                        cBoost.Height > params.min and
                        (cBoost.Height < params.max or (cBoost.Height = params.max and cBoost.BlockNum < params.blockNum))
        
                    cross join vTxStr sBoost on
                        sBoost.RowId = tBoost.RowId
        
                    cross join Transactions tContent indexed by Transactions_Type_RegId2_RegId1 on
                        tContent.Type in (200,201,202,209,210) and
                        tContent.RegId2 = tBoost.RegId2 and
                        exists (select 1 from Last l where l.TxId = tContent.RowId)
        
                    cross join Chain cContent on
                        cContent.TxId = tContent.RowId
        
                    cross join vtxStr sContent on
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
                    tBoost.RegId1 = params.addressId

            )sql"
            },

            // ShortTxType::Blocking
            {
                ShortTxType::Blocking, R"sql(
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

                from
                    params,
                    Transactions b indexed by Transactions_Type_RegId1_RegId2_RegId3

                    cross join Chain cb on
                        cb.TxId = b.RowId and
                        cb.Height > params.min and
                        (cb.Height < params.max or (cb.Height = params.max and cb.BlockNum < params.blockNum))

                    cross join vTxStr sb on
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
                        b.RegId1 = params.addressId

            )sql"
            },
        };

        static const auto header = R"sql(
            -- Common 'with' for all unions
            with
                params as (
                    select
                        ? as max,
                        ? as min,
                        ? as blockNum,
                        r.RowId as addressId
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

        auto sql = _unionSelectsBasedOnFilters(filters, selects, header, footer);

        EventsReconstructor reconstructor;
        SqlTransaction(
            __func__,
            [&]() -> Stmt& {
                return Sql(sql)
                .Bind(heightMax, heightMin, blockNumMax, address);
            },
            [&] (Stmt& stmt) {
                stmt.Select([&](Cursor& cursor) {
                    while (cursor.Step())
                    {
                        reconstructor.FeedRow(cursor);
                    }
                });
            }
        );

        return reconstructor.GetResult();
    }

    UniValue NotifierRepository::GetNotifications(int64_t height, const set<ShortTxType>& filters)
    {
        // Static because it will not be changed for entire node run
        static const map<ShortTxType, string> selects = {
            {
                ShortTxType::Money, R"sql(
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

                        join Chain c indexed by Chain_Height_Uid on
                            c.TxId = t.RowId and
                            c.Height = ?

                        join Registry r on
                            r.RowId = t.RowId

                    where
                        t.Type in (1,2,3) -- 1 default money transfer, 2 coinbase, 3 coinstake
            )sql"
            },

            {
                ShortTxType::Referal, R"sql(
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

                    from Transactions t not indexed -- Accessing by rowid

                    cross join vTxStr st on
                        st.RowId = t.RowId

                    join Chain c indexed by Chain_Height_Uid on
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
            )sql"
            },

            {
                ShortTxType::Answer, R"sql(
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

                    join Chain ca indexed by Chain_Height_Uid on
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
            )sql"
            },

            {
                ShortTxType::Comment, R"sql(
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
                                'AddressHash', (select r.String from Registry r where r.RowId = o.AddressId),
                                'ScriptPubKey', (select r.String from Registry r where r.RowId = o.ScriptPubKeyId)
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

                    join Chain cc indexed by Chain_Height_Uid on
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
            )sql"
            },

            {
                ShortTxType::Subscriber, R"sql(
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

                    join Chain c indexed by Chain_Height_Uid on
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
            )sql"
            },

            {
                ShortTxType::CommentScore, R"sql(
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

                    join Chain cs indexed by Chain_Height_Uid on
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
            )sql"
            },

            {
                ShortTxType::ContentScore, R"sql(
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

                    join Chain cs indexed by Chain_Height_Uid on
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
            )sql"
            },

            {
                ShortTxType::PrivateContent, R"sql(
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

                    from Transactions c not indexed -- content for private subscribers

                    cross join vTxStr sc on
                        sc.RowId = c.RowId

                    join Chain cc indexed by Chain_Height_Uid on
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
            )sql"
            },

            {
                ShortTxType::Boost, R"sql(
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

                    join Chain c indexed by Chain_Height_Uid on
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
            )sql"
            },

            {
            ShortTxType::Repost, R"sql(
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

                from Transactions r not indexed

                cross join vTxStr sr on
                    sr.RowId = r.RowId

                join Chain cr indexed by Chain_Height_Uid on
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
                    exists (select 1 from First f where f.TxId = r.RowId) -- Only orig
            )sql"
            },

            {
                ShortTxType::JuryAssigned, R"sql(
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
                        sf.Hash, -- Description (Jury Tx)
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

                        cross join Chain cf indexed by Chain_Height_Uid on
                            cf.TxId = f.RowId and
                            cf.Height = ?

                        cross join vTxStr sf on
                            sf.RowId = f.RowId

                        cross join Jury j
                            on j.FlagRowId = f.RowId

                        cross join Chain cu on
                            cu.Uid = j.AccountId and
                            exists (select 1 from Last l where l.TxId = cu.TxId)

                        cross join vTxStr su on
                            su.RowId = cu.TxId

                        cross join Payload p
                            on p.TxId = cu.TxId

                        cross join Transactions c
                            on c.RowId = f.RegId2

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
                )sql"
            },

            {
                ShortTxType::JuryVerdict, R"sql(
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
                        null,
                        v.Int1, -- Value
                        null,
                        null,
                        sv.String2, -- Description (Jury Tx)
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

                        cross join Chain cv indexed by Chain_Height_Uid on
                            cv.TxId = v.RowId and
                            cv.Height = ?

                        cross join vTxStr sv on
                            sv.RowId = v.RowId

                        cross join Jury j
                            on j.FlagRowId = v.RegId2

                        cross join Chain cu on
                            cu.Uid = j.AccountId and
                            exists (select 1 from Last l where l.TxId = cu.TxId)

                        cross join vTxStr su on
                            su.RowId = cu.TxId

                        cross join Payload p
                            on p.TxId = cu.TxId

                        cross join JuryVerdict jv indexed by JuryVerdict_VoteRowId_FlagRowId_Verdict
                            on jv.VoteRowId = v.RowId

                        cross join Transactions f
                            on f.RowId = jv.FlagRowId

                        cross join Transactions c on
                            c.RowId = f.RegId2

                        cross join Chain cc on
                            cc.TxId = c.RowId

                        cross join vTxStr sc on
                            sc.RowId = c.RowId

                        cross join Payload cp on
                            cp.TxId = c.RowId

                        left join Ratings rn indexed by Ratings_Type_Uid_Last_Height on
                            rn.Type = 0 and rn.Uid = j.AccountId and rn.Last = 1

                    where
                        v.Type = 420
                )sql"
            },

            {
                ShortTxType::JuryModerate, R"sql(
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
                        sf.Hash, -- Description (Jury Tx)
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

                        cross join Chain cf indexed by Chain_Height_Uid on
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
                        cross join Transactions c
                            on c.RowId = f.RegId2

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
                )sql"
            }

        };

        auto predicate = _choosePredicate(filters);
        
        NotificationsReconstructor reconstructor;
        for(const auto& select: selects) {
            const auto& type = select.first;
            if (predicate(type)) {
                const auto& query = select.second;

                SqlTransaction(
                    __func__,
                    [&]() -> Stmt& {
                        return Sql(query)
                            .Bind(height);
                    },
                    [&] (Stmt& stmt) {
                        stmt.Select([&](Cursor& cursor) {
                            while (cursor.Step())
                                reconstructor.FeedRow(cursor);
                        });
                    }
                );
            }
        }
        auto notificationResult = reconstructor.GetResult();
        return notificationResult.Serialize();
    }

    vector<ShortForm> NotifierRepository::GetEventsForAddresses(const string& address, int64_t heightMax, int64_t heightMin, int64_t blockNumMax, const set<ShortTxType>& filters)
    {
        static const map<ShortTxType, string> selects = {
            {
                ShortTxType::Money, R"sql(
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
                        TxOutputs o indexed by TxOutputs_AddressId_TxIdDesc_Number

                        join Transactions t not indexed on
                            t.RowId = o.TxId and
                            t.Type in (1,2,3)

                        cross join Chain ct not indexed on
                            ct.TxId = t.RowId and
                            ct.Height > params.min and
                            (ct.Height < params.max or (ct.Height = params.max and ct.BlockNum < params.blockNum))

                        cross join vTxStr st on
                            st.Rowid = t.RowId

                    where
                        o.AddressId = params.addressId
                    order by ct.Height desc, ct.BlockNum desc
                    limit 10
                )sql"
            },

            {
                ShortTxType::Referal, R"sql(
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
                        Transactions t

                        cross join vTxStr s on
                            s.RowId = t.RowId

                        cross join Chain c not indexed on
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
                        t.RegId2 = params.addressId and
                        exists (select 1 from First f where f.TxId = t.RowId)
                    order by c.Height desc, c.BlockNum desc
                    limit 10
                )sql"
            },

            {
                ShortTxType::Answer, R"sql(
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
                        params
                    cross join Transactions c indexed by Transactions_Type_RegId1_RegId2_RegId3 on
                        c.Type in (204, 205) and
                        c.RegId1 = params.addressId
                    cross join Last lc on
                        lc.TxId = c.RowId
                    cross join Chain cc on
                        cc.TxId = c.RowId
                    cross join vTxStr sc on
                        sc.RowId = c.RowId
                    cross join Payload pc on
                        pc.TxId = c.RowId
                    cross join Transactions a indexed by Transactions_Type_RegId5_RegId1 on
                        a.Type in (204, 205) and
                        a.RegId5 = c.RegId2 and
                        a.RegId1 != c.RegId1
                    cross join First fa on
                        fa.TxId = a.RowId
                    cross join vTxStr sa on
                        sa.RowId = a.RowId
                    cross join Chain ca not indexed on
                        ca.TxId = a.RowId and
                        ca.Height > params.min and
                        (ca.Height < params.max or (ca.Height = params.max and ca.BlockNum < params.blockNum))
                    cross join Transactions aLast indexed by Transactions_Type_RegId2_RegId1 on
                        aLast.Type in (204,205) and
                        aLast.RegId2 = a.RowId
                    cross join Last laLast on
                        laLast.TxId = aLast.RowId
                    cross join Payload pa on
                        pa.TxId = aLast.RowId
                    cross join Transactions aa indexed by Transactions_Type_RegId1_RegId2_RegId3 on
                        aa.Type = 100 and
                        aa.RegId1 = a.RegId1
                    cross join Last laa on
                        laa.TxId = aa.RowId
                    cross join Chain caa on
                        caa.TxId = aa.RowId
                    cross join Payload paa on
                        paa.TxId = aa.RowId
                    left join Ratings ra indexed by Ratings_Type_Uid_Last_Height on
                        ra.Type = 0 and
                        ra.Uid = caa.Uid and
                        ra.Last = 1
                    order by cc.Height desc, cc.BlockNum desc
                    limit 10
                )sql"
            },

            {
                ShortTxType::Comment, R"sql(
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
                        params
                    cross join Transactions p indexed by Transactions_Type_RegId1_RegId2_RegId3 on
                        p.Type in (200,201,202,209,210) and
                        p.RegId1 = params.addressId
                    cross join Last lp on
                        lp.TxId = p.RowId
                    cross join Chain cp on
                        cp.TxId = p.RowId
                    cross join vTxStr sp on
                        sp.RowId = p.RowId
                    cross join Transactions c indexed by Transactions_Type_RegId3_RegId1 on
                        c.Type = 204 and
                        c.RegId3 = p.RegId2 and
                        c.RegId1 != p.RegId1 and
                        c.RegId4 is null and
                        c.RegId5 is null
                    cross join Chain cc not indexed on
                        cc.TxId = c.RowId and
                        cc.Height > params.min and
                        (cc.Height < params.max or (cc.Height = params.max and cc.BlockNum < params.blockNum))
                    cross join vTxStr sc on
                        sc.RowId = c.RowId
                    cross join Transactions cLast indexed by Transactions_Type_RegId2_RegId1 on
                        cLast.Type in (204,205) and
                        cLast.RegId2 = c.RowId
                    cross join Last lcLast on
                        lcLast.TxId = cLast.RowId
                    cross join Payload pc on
                        pC.TxId = cLast.RowId
                    cross join Transactions ac indexed by Transactions_Type_RegId1_RegId2_RegId3 on
                        ac.RegId1 = c.RegId1 and
                        ac.Type = 100
                    cross join Last lac on
                        lac.TxId = ac.RowId
                    cross join Chain cac on
                        cac.TxId = ac.RowId
                    cross join Payload pac on
                        pac.TxId = ac.RowId
                    cross join Payload pp on
                        pp.TxId = p.RowId
                    left join Ratings rac indexed by Ratings_Type_Uid_Last_Height on
                        rac.Type = 0 and
                        rac.Uid = cac.Uid and
                        rac.Last = 1
                    order by cc.Height desc, cc.BlockNum desc
                    limit 10
                )sql"
            },

            {
                ShortTxType::Subscriber, R"sql(
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

                        cross join Chain c not indexed on
                            c.TxId = subs.RowId and
                            c.Height > params.min and
                            (c.Height < params.max or (c.Height = params.max and c.BlockNum < params.blockNum))

                        cross join vTxStr s on
                            s.RowId = subs.RowId

                        join Transactions u indexed by Transactions_Type_RegId1_RegId2_RegId3 on
                            u.Type in (100) and
                            u.RegId1 = subs.RegId1 and
                            exists (select 1 from Last l where l.TxId = u.RowId)

                        left join Chain cu on
                            cu.TxId = u.RowId
                        left join Payload pu on
                            pu.TxId = u.RowId
                        left join Ratings ru indexed by Ratings_Type_Uid_Last_Height on
                            ru.Type = 0 and
                            ru.Uid = cu.Uid and
                            ru.Last = 1
                    where
                        subs.Type in (302, 303, 304) and -- Ignoring unsubscribers?
                        subs.RegId2 = params.addressId
                    order by c.Height desc, c.BlockNum desc
                    limit 10
                )sql"
            },

            {
                ShortTxType::CommentScore, R"sql(
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

                        cross join Transactions s indexed by Transactions_Type_RegId2_RegId1 on
                            s.Type = 301 and
                            s.RegId2 = c.RegId2

                        cross join Chain cs not indexed on
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

                        left join Transactions acs indexed by Transactions_Type_RegId1_RegId2_RegId3 on
                            acs.Type = 100 and
                            acs.RegId1 = s.RegId1 and
                            exists (select 1 from Last l where l.TxId = acs.RowId)
                        left join Chain cacs on
                            cacs.TxId = acs.RowId
                        left join Payload pacs on
                            pacs.TxId = acs.RowId
                        left join Ratings racs indexed by Ratings_Type_Uid_Last_Height on
                            racs.Type = 0 and
                            racs.Uid = cacs.Uid and
                            racs.Last = 1
                    where
                        c.Type in (204) and
                        c.RegId1 = params.addressId
                    order by cs.Height desc, cs.BlockNum desc
                    limit 10
                )sql"
            },

            {
                // TODO (losty): optimize
                ShortTxType::ContentScore, R"sql(
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

                        cross join Transactions s indexed by Transactions_Type_RegId2_RegId1 on
                            s.Type = 300 and
                            s.RegId2 = c.RegId2

                        cross join Chain cs not indexed on
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

                        left join Transactions acs indexed by Transactions_Type_RegId1_RegId2_RegId3 on
                            acs.Type = 100 and
                            acs.RegId1 = s.RegId1 and
                            exists (select 1 from Last l where l.TxId = acs.RowId)

                        left join Chain cacs on
                            cacs.TxId = acs.RowId

                        left join Payload pacs on
                            pacs.TxId = acs.RowId

                        left join Ratings racs indexed by Ratings_Type_Uid_Last_Height on
                            racs.Type = 0 and
                            racs.Uid = cacs.Uid and
                            racs.Last = 1
                    where
                        c.Type in (200,201,202,209,210) and
                        c.RegId1 = params.addressId
                    order by cs.Height desc, cs.BlockNum desc
                    limit 10
                )sql"
            },

            {
                ShortTxType::PrivateContent, R"sql(
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

                        cross join Transactions c indexed by Transactions_Type_RegId1_RegId2_RegId3 on-- content for private subscribers
                            c.Type in (200,201,202,209,210) and
                            c.RegId1 = subs.RegId2 and
                            exists (select 1 from First f where f.TxId = c.RowId)

                        cross join Chain cc not indexed on
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
                        subs.RegId1 = params.addressId and
                        exists (select 1 from Last l where l.TxId = subs.RowId)
                    order by cc.Height desc, cc.BlockNum desc
                    limit 10
                )sql"
            },

            {
                ShortTxType::Boost, R"sql(
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

                        cross join Chain cBoost not indexed on
                            cBoost.TxId = tBoost.RowId and
                            cBoost.Height > params.min and
                            (cBoost.Height < params.max or (cBoost.Height = params.max and cBoost.BlockNum < params.blockNum))

                        cross join vTxStr sBoost on
                            sBoost.RowId = tBoost.RowId

                        cross join Transactions tContent indexed by Transactions_Type_RegId1_RegId2_RegId3 on
                            tContent.Type in (200,201,202,209,210) and
                            tContent.RegId1 = params.addressId and
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
                    order by cBoost.Height desc, cBoost.BlockNum desc
                    limit 10
                )sql"
            },

            {
                ShortTxType::Repost, R"sql(
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

                        cross join Transactions r indexed by Transactions_Type_RegId3_RegId1 on
                            r.Type = 200 and
                            r.RegId3 = p.RegId2 and
                            exists (select 1 from First f where f.TxId = r.RowId)

                        cross join vTxStr sr on
                            sr.RowId = r.RowId

                        cross join Chain cr not indexed on
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
                            and rar.Uid = car.Uid
                            and rar.Last = 1
                    where
                        p.Type = 200 and
                        p.RegId1 = params.addressId and
                        exists (select 1 from Last l where l.TxId = p.RowId)
                    order by cr.Height desc, cr.BlockNum desc
                    limit 10
                )sql"
            },

            {
                ShortTxType::JuryAssigned, R"sql(
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
                        sf.Hash, -- Description (Jury Tx)
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
                        params
                    cross join Transactions f indexed by Transactions_Type_RegId3_RegId1 on
                        f.Type = 410 and
                        f.RegId3 = params.addressId
                    cross join Chain cf indexed by Chain_TxId_Height on
                        cf.TxId = f.RowId and
                        cf.Height > params.min and
                        (cf.Height < params.max or (cf.Height = params.max and cf.BlockNum < params.blockNum))
                    cross join vTxStr sf on
                        sf.RowId = f.RowId
                    cross join Jury j on
                        j.FlagRowId = f.RowId
                    cross join Transactions c on
                        c.RowId = f.RegId2
                    cross join Chain cc on
                        cc.TxId = c.RowId
                    cross join vTxStr sc on
                        sc.RowId = c.RowId
                    cross join Payload cp on
                        cp.TxId = c.RowId
                    order by cf.Height desc, cf.BlockNum desc
                    limit 10
                )sql"
            },

            {
                ShortTxType::JuryVerdict, R"sql(
                    select
                        (')sql" + ShortTxTypeConvertor::toString(ShortTxType::JuryVerdict) + R"sql('),
                        -- Tx data
                        sv.Hash, -- Vote Tx
                        v.Type,
                        null,
                        cv.Height as Height,
                        cv.BlockNum as BlockNum,
                        v.Time,
                        null,
                        null, -- Tx of content
                        v.Int1, -- Value
                        null,
                        null,
                        sv.String2, -- Description (Jury Tx)
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
                        params,
                        Transactions acc indexed by Transactions_Type_RegId1_RegId2_RegId3

                        cross join Chain cacc on
                            cacc.TxId = acc.RowId

                        cross join Jury j on
                            j.AccountId = cacc.Uid

                        cross join JuryVerdict jv on
                            jv.FlagRowId = j.FlagRowId

                        cross join Transactions v on
                            v.RowId = jv.VoteRowId

                        cross join Chain cv indexed by Chain_Height_Uid on
                            cv.TxId = v.RowId and
                            cv.Height > params.min and
                            (cv.Height < params.max or (cv.Height = params.max and cv.BlockNum < params.blockNum))

                        cross join vTxStr sv on
                            sv.RowId = v.RowId

                        cross join Chain cu on
                            cu.Uid = j.AccountId and
                            exists (select 1 from Last l where l.TxId = cu.TxId)

                        cross join vTxStr su on
                            su.RowId = cu.TxId

                        cross join Payload p
                            on p.TxId = cu.TxId

                        cross join Transactions f
                            on f.RowId = jv.FlagRowId

                        cross join Transactions c on
                            c.RowId = f.RegId2

                        cross join Chain cc on
                            cc.TxId = c.RowId

                        cross join vTxStr sc on
                            sc.RowId = c.RowId

                        cross join Payload cp on
                            cp.TxId = c.RowId

                        left join Ratings rn indexed by Ratings_Type_Uid_Last_Height on
                            rn.Type = 0 and rn.Uid = j.AccountId and rn.Last = 1

                    where
                        acc.Type = 100 and
                        acc.RegId1 = params.addressId and
                        exists (select 1 from Last l where l.TxId = acc.RowId)
                )sql"
            },

            {
                ShortTxType::JuryModerate, R"sql(
                    select
                        (')sql" + ShortTxTypeConvertor::toString(ShortTxType::JuryModerate) + R"sql('),
                        -- Tx data
                        sf.Hash, -- Flag Tx
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
                        sf.Hash, -- Description (Jury Tx)
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

                        cross join Chain cf not indexed on
                            cf.TxId = f.RowId and
                            cf.Height > params.min and
                            (cf.Height < params.max or (cf.Height = params.max and cf.BlockNum < params.blockNum))

                        cross join vTxStr sf on
                            sf.RowId = f.RowId

                        -- content
                        cross join Transactions c on
                            c.RowId = f.RegId2

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
                        acc.RegId1 = params.addressId and
                        exists (select 1 from Last l where l.TxId = acc.RowId)
                    order by cf.Height desc, cf.BlockNum desc
                    limit 10
                )sql"
            }
        };

        static const auto header = R"sql(
            with
                params as (
                    select
                        ? as max,
                        ? as min,
                        ? as blockNum,
                        r.RowId as addressId
                    from
                        Registry r
                    where
                        r.String = ?
                )
        )sql";

        EventsReconstructor reconstructor;
        for (const auto& reqSql : selects)
        {
            auto[ok, sql] = _constructSelectsBasedOnFilters(filters, reqSql, header);
            if (!ok)
                continue;
            
            string _sql = sql;
            SqlTransaction(
                __func__,
                [&]() -> Stmt& {
                    return Sql(_sql).Bind(heightMax, heightMin, blockNumMax, address);
                },
                [&] (Stmt& stmt) {
                    stmt.Select([&](Cursor& cursor) {
                        while (cursor.Step())
                            reconstructor.FeedRow(cursor);
                    });
                }
            );
        }

        auto result = reconstructor.GetResult();

        sort(result.begin(), result.end(), [](auto& a, auto& b) {
            return tie(*a.GetTxData().GetHeight(), *a.GetTxData().GetHeight()) < tie(*b.GetTxData().GetHeight(), *b.GetTxData().GetHeight());
        });
        
        vector<PocketDb::ShortForm> shortResult(result.begin(), result.begin() + min(10, (int)result.size()));
        return shortResult;
    }

    map<string, map<ShortTxType, int>> NotifierRepository::GetNotificationsSummary(int64_t heightMax, int64_t heightMin, const vector<string>& addresses, const set<ShortTxType>& filters)
    {
        const map<ShortTxType, string> selects = {
            {
                ShortTxType::Referal, R"sql(
                    select
                        (')sql" + ShortTxTypeConvertor::toString(ShortTxType::Referal) + R"sql('),
                        addr.hash,
                        t.RowId
                    from
                        addr,
                        params
                        cross join Transactions t indexed by Transactions_Type_RegId2_RegId1 on
                            t.Type in (100) and
                            t.RegId2 = addr.id
                        cross join First f on
                            f.TxId = t.RowId
                        cross join Chain c on
                            c.TxId = f.TxId and c.Height between params.min and params.max
                )sql"
            },

            {
                ShortTxType::Comment, R"sql(
                    select
                        (')sql" + ShortTxTypeConvertor::toString(ShortTxType::Comment) + R"sql('),
                        addr.hash,
                        c.RowId
                    from
                        addr,
                        params
                    cross join Transactions p indexed by Transactions_Type_RegId1_RegId2_RegId3 on
                        p.Type in (200, 201, 202, 209, 210) and
                        p.RegId1 = addr.id
                    cross join Last lp on
                        lp.TxId = p.RowId
                    cross join Transactions c indexed by Transactions_Type_RegId3_RegId1 on
                        c.Type in (204) and
                        c.RegId3 = p.RegId2 and
                        c.RegId1 != p.RegId1 and
                        c.RegId4 is null and
                        c.RegId5 is null
                    cross join Chain cc on
                        cc.TxId = c.RowId and
                        cc.Height between params.min and params.max
                )sql"
            },

            {
                ShortTxType::Subscriber, R"sql(
                    select
                        (')sql" + ShortTxTypeConvertor::toString(ShortTxType::Subscriber) + R"sql('),
                        addr.hash,
                        s.RowId
                    from
                        addr,
                        params
                        cross join Transactions s indexed by Transactions_Type_RegId2_RegId1 on
                            s.Type in (302, 303) and
                            s.RegId2 = addr.id
                        cross join Chain c on
                            c.TxId = s.RowId and
                            c.Height between params.min and params.max
                )sql"
            },

            {
                ShortTxType::CommentScore, R"sql(
                    select
                        (')sql" + ShortTxTypeConvertor::toString(ShortTxType::CommentScore) + R"sql('),
                        addr.hash,
                        s.RowId
                    from
                        addr,
                        params
                        cross join Transactions c indexed by Transactions_Type_RegId1_RegId2_RegId3 on
                            c.Type in (204, 205) and c.RegId1 = addr.id
                        cross join Last lc on
                            lc.TxId = c.RowId
                        cross join Transactions s indexed by Transactions_Type_RegId2_RegId1 on
                            s.Type in (301) and s.RegId2 = c.RegId2
                        cross join Chain cs on
                            cs.TxId = s.RowId and cs.Height between params.min and params.max
                )sql"
            },

            {
                ShortTxType::ContentScore, R"sql(
                    select
                        (')sql" + ShortTxTypeConvertor::toString(ShortTxType::ContentScore) + R"sql('),
                        addr.hash,
                        s.RowId
                    from
                        addr,
                        params
                        cross join Transactions c indexed by Transactions_Type_RegId1_RegId2_RegId3 on
                            c.Type in (200, 201, 202, 209, 210) and
                            c.RegId1 = addr.id
                        cross join Last lc on
                            lc.TxId = c.RowId
                        cross join Transactions s indexed by Transactions_Type_RegId2_RegId1 on
                            s.Type in (300) and
                            s.RegId2 = c.RegId2
                        cross join Chain cs on
                            cs.TxId = s.RowId and
                            cs.Height between params.min and params.max
                )sql"
            },

            {
                ShortTxType::Repost, R"sql(
                    select
                        (')sql" + ShortTxTypeConvertor::toString(ShortTxType::Repost) + R"sql('),
                        addr.hash,
                        r.RowId
                    from
                        addr,
                        params
                        cross join Transactions p indexed by Transactions_Type_RegId1_RegId2_RegId3 on
                            p.Type in (200) and
                            p.RegId1 = addr.id
                        cross join Last lp on
                            lp.TxId = p.RowId
                        cross join Transactions r indexed by Transactions_Type_RegId3_RegId1 on
                            r.Type in (200) and
                            r.RegId3 = p.RegId2 and
                            r.RegId3 is not null
                        cross join First fr on
                            fr.TxId = r.RowId
                        cross join Chain cr on
                            cr.TxId = r.RowId and
                            cr.Height between params.min and params.max
                )sql"
            }
        };

        const auto header = R"sql(
            with
                params as (
                    select
                        ? as max,
                        ? as min
                ),
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

        auto sql = _unionSelectsBasedOnFilters(filters, selects, header, "");

        NotificationSummaryReconstructor reconstructor;
        SqlTransaction(
            __func__,
            [&]() -> Stmt& {
                return Sql(sql).Bind(heightMax, heightMin, addresses);
            },
            [&] (Stmt& stmt) {
                stmt.Select([&](Cursor& cursor)
                {
                    while (cursor.Step())
                        reconstructor.FeedRow(cursor);
                });
            }
        );

        return reconstructor.GetResult();
    }



}