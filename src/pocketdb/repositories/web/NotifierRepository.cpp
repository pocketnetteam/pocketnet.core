// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include "NotifierRepository.h"

namespace PocketDb
{
    UniValue NotifierRepository::GetAccountInfoByAddress(const string& address)
    {
        UniValue result(UniValue::VOBJ);

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
            .Bind(address)
            .Select([&](Cursor& cursor) {
                if (cursor.Step())
                {
                    if (auto[ok, value] = cursor.TryGetColumnString(0); ok) result.pushKV("address", value);
                    if (auto[ok, value] = cursor.TryGetColumnString(1); ok) result.pushKV("name", value);
                    if (auto[ok, value] = cursor.TryGetColumnString(2); ok) result.pushKV("avatar", value);
                }
            });
        });

        return result;
    }

    UniValue NotifierRepository::GetPostLang(const string &postHash)
    {
        UniValue result(UniValue::VOBJ);

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
                )
                select p.String1 Lang
                from
                    tx
                    join Transactions t indexed by Transactions_HashId on
                        t.HashId = tx.id and t.Type in (200, 201, 202, 209, 210, 203)
                    join Payload p on
                        p.TxId = t.RowId
            )sql")
            .Bind(postHash)
            .Select([&](Cursor& cursor) {
                if (cursor.Step())
                {
                    if (auto[ok, value] = cursor.TryGetColumnString(0); ok) result.pushKV("lang", value);
                }
            });
        });

        return result;
    }

    UniValue NotifierRepository::GetPostInfo(const string& postHash)
    {
        UniValue result(UniValue::VOBJ);

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
                )
                select
                    tx.hash,
                    (select r.String from Registry r where r.RowId = t.RegId2)
                from
                    tx
                    join Transactions t indexed by Transactions_HashId on
                        t.HashId = tx.id and t.Type in (200, 201, 202, 209, 210, 203)
            )sql")
            .Bind(postHash)
            .Select([&](Cursor& cursor) {
                if (cursor.Step())
                {
                    if (auto[ok, value] = cursor.TryGetColumnString(0); ok) result.pushKV("hash", value);
                    if (auto[ok, value] = cursor.TryGetColumnString(1); ok) result.pushKV("rootHash", value);
                }
            });
        });

        return result;
    }

    UniValue NotifierRepository::GetBoostInfo(const string& boostHash)
    {
        UniValue result(UniValue::VOBJ);

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
                    join Transactions tBoost indexed by Transactions_HashId on
                        tBoost.HashId = tx.id and tBoost.Type in (208)
                    join Transactions tContent indexed by Transactions_Type_RegId2 on
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
            .Bind(boostHash)
            .Select([&](Cursor& cursor) {
                if (cursor.Step())
                {
                    if (auto[ok, value] = cursor.TryGetColumnString(0); ok) result.pushKV("hash", value);
                    if (auto[ok, value] = cursor.TryGetColumnString(1); ok) result.pushKV("boostAddress", value);
                    if (auto[ok, value] = cursor.TryGetColumnString(2); ok) result.pushKV("boostAmount", value);
                    if (auto[ok, value] = cursor.TryGetColumnString(3); ok) result.pushKV("boostName", value);
                    if (auto[ok, value] = cursor.TryGetColumnString(4); ok) result.pushKV("boostAvatar", value);
                    if (auto[ok, value] = cursor.TryGetColumnString(5); ok) result.pushKV("contentAddress", value);
                    if (auto[ok, value] = cursor.TryGetColumnString(6); ok) result.pushKV("contentHash", value);
                }
            });
        });

        return result;
    }

    UniValue NotifierRepository::GetOriginalPostAddressByRepost(const string &repostHash)
    {
        UniValue result(UniValue::VOBJ);

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
                )
                select
                    (select r.String from Registry r where r.RowId = t.RegId2) as RootTxHash,
                    (select r.String from Registry r where r.RowId = t.RegId1) as address,
                    (select r.String from Registry r where r.RowId = tRepost.RegId1) as addressRepost,
                    p.String2 as nameRepost,
                    p.String3 as avatarRepost
                from
                    tx
                    join Transactions tRepost indexed by Transactions_HashId on
                        tRepost.HashId = tx.id and tRepost.Type in (200, 201, 202, 209, 210, 203)
                    join Transactions t indexed by Transactions_HashId on
                        t.HashId = tRepost.RegId3
                    join Transactions u indexed by Transactions_Type_RegId1_RegId2_RegId3 on
                        u.Type in (100) and u.RegId1 = tRepost.RegId1
                    join Last lu on
                        lu.TxId = u.RowId
                    join Payload p on
                        p.TxId = u.RowId
            )sql")
            .Bind(repostHash)
            .Select([&](Cursor& cursor) {
                if (cursor.Step())
                {
                    if (auto[ok, value] = cursor.TryGetColumnString(0); ok) result.pushKV("hash", value);
                    if (auto[ok, value] = cursor.TryGetColumnString(1); ok) result.pushKV("address", value);
                    if (auto[ok, value] = cursor.TryGetColumnString(2); ok) result.pushKV("addressRepost", value);
                    if (auto[ok, value] = cursor.TryGetColumnString(3); ok) result.pushKV("nameRepost", value);
                    if (auto[ok, value] = cursor.TryGetColumnString(4); ok) result.pushKV("avatarRepost", value);
                }
            });
        });

        return result;
    }

    UniValue NotifierRepository::GetPrivateSubscribeAddressesByAddressTo(const string &addressTo)
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
                    (select r.String from Registry r where r.RowId = s.RegId1) as addressTo,
                    p.String2 as nameFrom,
                    p.String3 as avatarFrom
                from
                    addr
                    join Transactions s indexed by Transactions_Type_RegId2 on
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
            .Bind(addressTo)
            .Select([&](Cursor& cursor) {
                while (cursor.Step())
                {
                    UniValue record(UniValue::VOBJ);
                    if (auto[ok, value] = cursor.TryGetColumnString(0); ok) record.pushKV("addressTo", value);
                    if (auto[ok, value] = cursor.TryGetColumnString(1); ok) record.pushKV("nameFrom", value);
                    if (auto[ok, value] = cursor.TryGetColumnString(2); ok) record.pushKV("avatarFrom", value);
                    result.push_back(record);
                }
            });
        });

        return result;
    }

    // Not used. Not planned yet. Invalid request.
//    UniValue NotifierRepository::GetUserReferrerAddress(const string &userHash)
//    {
//        UniValue result(UniValue::VOBJ);
//
//        string sql = R"sql(
//            select
//                r.String2 as referrerAddress,
//                p.String2 as referralName,
//                p.String3 as referralAvatar
//            from Transactions r
//            join Transactions u indexed by Transactions_Type_Last_String1_Height_Id on u.String1 = r.String1
//            join Payload p on p.TxHash = u.Hash
//            where r.Type in (100)
//              and r.String2 is not null
//              and r.Hash = ?
//              and u.Type in (100)
//              and u.Last=1
//              and u.Height is not null
//        )sql";
//
//        SqlTransaction(__func__, [&]()
//        {
//            auto& cursor = Sql(sql);
//
//            TryBindStatementText(cursor, 1, userHash);
//
//            if (sqlite3_step(*cursor) == SQLITE_ROW)
//            {
//                if (auto[ok, value] = cursor.TryGetColumnString(*cursor, 0); ok) result.pushKV("referrerAddress", value);
//                if (auto[ok, value] = cursor.TryGetColumnString(*cursor, 1); ok) result.pushKV("referralName", value);
//                if (auto[ok, value] = cursor.TryGetColumnString(*cursor, 2); ok) result.pushKV("referralAvatar", value);
//            }
//
//            FinalizeSqlStatement(*cursor);
//        });
//
//        return result;
//    }

    UniValue NotifierRepository::GetPostInfoAddressByScore(const string &postScoreHash)
    {
        UniValue result(UniValue::VOBJ);

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
                )
                select
                    (select r.String from Registry r where r.RowId = score.RegId2) as postTxHash,
                    score.Int1 value,
                    (select r.String from Registry r where r.RowId = post.RegId1) as postAddress,
                    p.String2 as scoreName,
                    p.String3 as scoreAvatar
                from
                    tx
                    join Transactions score indexed by Transactions_HashId on
                        score.HashId = tx.id and score.Type in (300)
                    join Transactions post on
                        post.HashId = score.RegId2
                    join Transactions u indexed by Transactions_Type_RegId1_RegId2_RegId3 on
                        u.Type in (100) and u.RegId1 = score.RegId1
                    join Last lu on
                        lu.TxId = u.RowId
                    join Payload p on
                        p.TxId = u.RowId
            )sql")
            .Bind(postScoreHash)
            .Select([&](Cursor& cursor) {
                if (cursor.Step())
                {
                    if (auto[ok, value] = cursor.TryGetColumnString(0); ok) result.pushKV("postTxHash", value);
                    if (auto[ok, value] = cursor.TryGetColumnString(1); ok) result.pushKV("value", value);
                    if (auto[ok, value] = cursor.TryGetColumnString(2); ok) result.pushKV("postAddress", value);
                    if (auto[ok, value] = cursor.TryGetColumnString(3); ok) result.pushKV("scoreName", value);
                    if (auto[ok, value] = cursor.TryGetColumnString(4); ok) result.pushKV("scoreAvatar", value);
                }
            });
        });

        return result;
    }

    // TODO (aok, api): implement
    UniValue NotifierRepository::GetSubscribeAddressTo(const string &subscribeHash)
    {
        UniValue result(UniValue::VOBJ);

        string sql = R"sql(
            select
              s.String2 addressTo,
              p.String2 as nameFrom,
              p.String3 as avatarFrom
            from Transactions s
            cross join Transactions u indexed by Transactions_Type_Last_String1_Height_Id
                on u.Type in (100)
               and u.Last=1
               and u.String1 = s.String1
               and u.Height is not null
            cross join Payload p on p.TxHash = u.Hash
            where s.Type in (302, 303, 304)
              and s.Hash = ?
        )sql";

        SqlTransaction(__func__, [&]()
        {
            Sql(sql)
            .Bind(subscribeHash)
            .Select([&](Cursor& cursor) {
                if (cursor.Step())
                {
                    if (auto[ok, value] = cursor.TryGetColumnString(0); ok) result.pushKV("addressTo", value);
                    if (auto[ok, value] = cursor.TryGetColumnString(1); ok) result.pushKV("nameFrom", value);
                    if (auto[ok, value] = cursor.TryGetColumnString(2); ok) result.pushKV("avatarFrom", value);
                }
            });
        });

        return result;
    }

    // TODO (aok, api): implement
    UniValue NotifierRepository::GetCommentInfoAddressByScore(const string &commentScoreHash)
    {
        UniValue result(UniValue::VOBJ);

        string sql = R"sql(
            select
                score.String2 commentHash,
                score.Int1 value,
                comment.String1 commentAddress,
                p.String2 as scoreCommentName,
                p.String3 as scoreCommentAvatar
            from Transactions score
            join Transactions comment on score.String2 = comment.Hash
            join Transactions u indexed by Transactions_Type_Last_String1_Height_Id on u.String1 = score.String1
            join Payload p on p.TxHash = u.Hash
            where score.Hash = ?
                and u.Type in (100)
                and u.Last=1
                and u.Height is not null
        )sql";

        SqlTransaction(__func__, [&]()
        {
            Sql(sql)
            .Bind(commentScoreHash)
            .Select([&](Cursor& cursor) {
                if (cursor.Step())
                {
                    if (auto[ok, value] = cursor.TryGetColumnString(0); ok) result.pushKV("commentHash", value);
                    if (auto[ok, value] = cursor.TryGetColumnString(1); ok) result.pushKV("value", value);
                    if (auto[ok, value] = cursor.TryGetColumnString(2); ok) result.pushKV("commentAddress", value);
                    if (auto[ok, value] = cursor.TryGetColumnString(3); ok) result.pushKV("scoreCommentName", value);
                    if (auto[ok, value] = cursor.TryGetColumnString(4); ok) result.pushKV("scoreCommentAvatar", value);
                }
            });
        });

        return result;
    }

    // TODO (aok, api): implement
    UniValue NotifierRepository::GetFullCommentInfo(const string &commentHash)
    {
        UniValue result(UniValue::VOBJ);

        string sql = R"sql(
            select
                comment.String3 PostHash,
                comment.String4 ParentHash,
                comment.String5 AnswerHash,
                comment.String2 RootHash,
                content.String1 ContentAddress,
                answer.String1 AnswerAddress,
                p.String2 as commentName,
                p.String3 as commentAvatar,
                (
                    select o.Value
                    from TxOutputs o indexed by TxOutputs_TxHash_AddressHash_Value
                    where o.TxHash = comment.Hash and o.AddressHash = content.String1 and o.AddressHash != comment.String1
                ) as Donate
            from Transactions comment -- sqlite_autoindex_Transactions_1 (Hash)
            cross join Transactions u indexed by Transactions_Type_Last_String1_Height_Id on u.String1 = comment.String1
            cross join Payload p on p.TxHash = u.Hash
            cross join Transactions content -- sqlite_autoindex_Transactions_1 (Hash)
                on content.Type in (200, 201, 202, 209, 210) and content.Hash = comment.String3
            left join Transactions answer indexed by Transactions_Type_Last_String2_Height
                on answer.Type in (204, 205) and answer.Last = 1 and answer.String2 = comment.String5
            where comment.Type in (204, 205)
              and comment.Hash = ?
              and u.Type in (100)
              and u.Last=1
              and u.Height is not null
        )sql";

        SqlTransaction(__func__, [&]()
        {
            Sql(sql)
            .Bind(commentHash)
            .Select([&](Cursor& cursor) {
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
                    if (auto[ok, value] = cursor.TryGetColumnString(8); ok)
                    {
                        result.pushKV("donation", "true");
                        result.pushKV("amount", value);
                    }
                }
            });
        });

        return result;
    }

    // TODO (aok, api): implement
    UniValue NotifierRepository::GetPostCountFromMySubscribes(const string& address, int height)
    {
        UniValue result(UniValue::VOBJ);

        string sql = R"sql(
            select count(1) as cntTotal,
                   ifnull((case when post.Type = 200 then 1 else 0 end),0) as cntPost,
                   ifnull((case when post.Type = 201 then 1 else 0 end),0) as cntVideo,
                   ifnull((case when post.Type = 202 then 1 else 0 end),0) as cntArticle,
                   ifnull((case when post.Type = 209 then 1 else 0 end),0) as cntStream,
                   ifnull((case when post.Type = 210 then 1 else 0 end),0) as cntAudio
            from Transactions sub
            join Transactions post
                on post.String1 = sub.String2 and post.Type in (200, 201, 202, 209, 210, 203) and post.Last = 1
            where sub.Type in (302, 303)
              and sub.Last = 1
              and post.Height = ?
              and sub.String1 = ?
        )sql";

        SqlTransaction(__func__, [&]()
        {
            Sql(sql)
            .Bind(height, address)
            .Select([&](Cursor& cursor) {
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
        });

        return result;
    }
}