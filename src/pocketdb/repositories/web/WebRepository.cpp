// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include "WebRepository.h"

void PocketDb::WebRepository::Init() {}

void PocketDb::WebRepository::Destroy() {}

UniValue PocketDb::WebRepository::GetUserAddress(std::string& name)
{
    UniValue result(UniValue::VARR);

    string sql = R"sql(
        SELECT p.String2, u.String1
        FROM Transactions u
        JOIN Payload p on u.Hash = p.TxHash
        WHERE   u.Type in (100, 101, 102)
            and p.String2 = ?
        LIMIT 1
    )sql";

    TryTransactionStep(__func__, [&]()
    {
        auto stmt = SetupSqlStatement(sql);

        TryBindStatementText(stmt, 1, name);

        while (sqlite3_step(*stmt) == SQLITE_ROW)
        {
            UniValue record(UniValue::VOBJ);

            if (auto[ok, valueStr] = TryGetColumnString(*stmt, 0); ok) record.pushKV("name", valueStr);
            if (auto[ok, valueStr] = TryGetColumnString(*stmt, 1); ok) record.pushKV("address", valueStr);

            result.push_back(record);
        }

        FinalizeSqlStatement(*stmt);
    });

    return result;
}

UniValue PocketDb::WebRepository::GetAddressesRegistrationDates(vector<string>& addresses)
{
    string sql = R"sql(
        WITH addresses (String1, Height) AS (
            SELECT u.String1, MIN(u.Height) AS Height
            FROM Transactions u
            WHERE u.Type in (100, 101, 102)
              and u.Height is not null
              and u.String1 in ( )sql" + join(vector<string>(addresses.size(), "?"), ",") + R"sql( )
              and u.Last in (0,1)
            GROUP BY u.String1   
        )
        SELECT u.String1, u.Time, u.Hash
        FROM Transactions u
        JOIN addresses a ON u.Type in (100, 101, 102) and u.String1 = a.String1 AND u.Height = a.Height
    )sql";

    auto result = UniValue(UniValue::VARR);

    TryTransactionStep(__func__, [&]()
    {
        auto stmt = SetupSqlStatement(sql);

        for (size_t i = 0; i < addresses.size(); i++)
            TryBindStatementText(stmt, (int) i + 1, addresses[i]);

        while (sqlite3_step(*stmt) == SQLITE_ROW)
        {
            UniValue record(UniValue::VOBJ);

            if (auto[ok, valueStr] = TryGetColumnString(*stmt, 0); ok) record.pushKV("address", valueStr);
            if (auto[ok, valueStr] = TryGetColumnString(*stmt, 1); ok) record.pushKV("time", valueStr);
            if (auto[ok, valueStr] = TryGetColumnString(*stmt, 2); ok) record.pushKV("txid", valueStr);

            result.push_back(record);
        }

        FinalizeSqlStatement(*stmt);
    });

    return result;
}

// Top addresses info
UniValue PocketDb::WebRepository::GetAddressInfo(int count)
{
    // shared_ptr<ListDto<AddressInfoDto>> result = make_shared<ListDto<AddressInfoDto>>();

    // return make_tuple(
    //     TryTransactionStep(__func__, [&]()
    //     {
    //         bool stepResult = true;
    //         auto stmt = SetupSqlStatement(R"sql(
    //             select o.AddressHash, sum(o.Value)
    //             from TxOutputs o
    //             where o.SpentHeight is null
    //             group by o.AddressHash
    //             order by sum(o.Value) desc
    //             limit ?
    //         )sql");

    //         auto countPtr = make_shared<int>(count);
    //         if (!TryBindStatementInt(stmt, 1, countPtr))
    //             stepResult = false;

    //         while (stepResult && sqlite3_step(*stmt) == SQLITE_ROW)
    //         {
    //             AddressInfoDto inf;
    //             inf.Address = GetColumnString(*stmt, 0);
    //             inf.Balance = GetColumnInt64(*stmt, 1);
    //             result->Add(inf);
    //         }

    //         FinalizeSqlStatement(*stmt);
    //         return stepResult;
    //     }),
    //     result
    // );
    // todo (brangr): implement
}

UniValue PocketDb::WebRepository::GetAccountState(const string& address)
{
    UniValue result(UniValue::VOBJ);

    // TODO (brangr): implement
    string sql = R"sql(
        select
            u.String1 as Address,

            (select reg.Time from Transactions reg indexed by Transactions_Id
                where reg.Id=u.Id and reg.Height=(select min(reg1.Height) from Transactions reg1 indexed by Transactions_Id where reg1.Id=reg.Id)) as RegistrationDate,

            (select r.Value from Ratings r where r.Type=0 and r.Id=u.Id and r.Height=(select max(r1.height) from Ratings r1 where r1.Type=0 and r1.Id=r.Id)) / 10 as Reputation,

            (select sum(o.Value) from TxOutputs o indexed by TxOutputs_AddressHash_SpentHeight_TxHeight
                where o.AddressHash=u.String1 and o.SpentHeight is null) as Balance,

            (select count(1) from Ratings r where r.Type=1 and r.Id=u.Id)

        from Transactions u indexed by Transactions_Type_Last_String1_Height
        where u.Type in (100, 102, 102)
          and u.Height is not null
          and u.String1 = ?
          and u.Last = 1
    )sql";

    TryTransactionStep(__func__, [&]()
    {
        auto stmt = SetupSqlStatement(sql);

        TryBindStatementText(stmt, 1, address);

        if (sqlite3_step(*stmt) == SQLITE_ROW)
        {
            UniValue record(UniValue::VOBJ);

            // if (auto [ok, valueStr] = TryGetColumnString(*stmt, 0); ok) record.pushKV("name", valueStr);
            // if (auto [ok, valueStr] = TryGetColumnString(*stmt, 1); ok) record.pushKV("address", valueStr);
            //
            // result.pushKV("address", address);
            // result.pushKV("user_reg_date", user_registration_date);
            // result.pushKV("reputation", reputation / 10.0);
            // result.pushKV("balance", balance);

            // todo  calculate
            // result.pushKV("trial", trial);
            // result.pushKV("mode", mode);
            // result.pushKV("likers", likers);

            // calculate
            // result.pushKV("post_unspent", post_unspent);
            // result.pushKV("post_spent", post_spent);
            // result.pushKV("video_unspent", video_unspent);
            // result.pushKV("video_spent", video_spent);
            // result.pushKV("score_unspent", score_unspent);
            // result.pushKV("score_spent", score_spent);
            // result.pushKV("complain_unspent", complain_unspent);
            // result.pushKV("complain_spent", complain_spent);
            // result.pushKV("comment_spent", comment_spent);
            // result.pushKV("comment_unspent", comment_unspent);
            // result.pushKV("comment_score_spent", comment_score_spent);
            // result.pushKV("comment_score_unspent", comment_score_unspent);

            // ??
            // result.pushKV("number_of_blocking", number_of_blocking);
            // result.pushKV("addr_reg_date", address_registration_date);
        }

        FinalizeSqlStatement(*stmt);
    });

    return result;
}

std::map<std::string, UniValue> PocketDb::WebRepository::GetUserProfile(std::vector<std::string>& addresses, bool shortForm, int option)
{
    std::map<std::string, UniValue> result{};

    string sql = R"sql(
        select
            u.String1 as Address,
            p.String2 as Name,
            u.Id as AccountId,
            p.String3 as Avatar,
            p.String7 as Donations,
            p.String4 as About,
            u.String2 as Referrer,
            p.String1 as Lang,
            p.String5 as Url,
            u.Time as LastUpdate,
            p.String6 as Pubkey,

            (select count(1) from Transactions ru indexed by Transactions_Type_Last_String2_Height
                where ru.Type in (100,101,102) and ru.Last=1 and ru.Height is not null and ru.String2=u.String1) as ReferrersCount,

            (select count(1) from Transactions po indexed by Transactions_Type_Last_String1_Height
                where po.Type in (200) and po.Last=1 and po.Height is not null and po.String1=u.String1) as PostsCount,

            (select r.Value from Ratings r indexed by Ratings_Height
                where r.Type=0 and r.Id=u.Id order by r.Height desc limit 1) / 10 as Reputation,

            (select reg.Time from Transactions reg indexed by Transactions_Id
                where reg.Id=u.Id and reg.Height is not null order by reg.Height asc limit 1) as RegistrationDate

        from Transactions u indexed by Transactions_Type_Last_String1_Height
        join Payload p on p.TxHash=u.Hash
        where u.Type in (100,101,102) and u.Last=1 and u.Height is not null
          and u.String1 in ()sql" + join(vector<string>(addresses.size(), "?"), ",") + R"sql()
    )sql";

    TryTransactionStep(__func__, [&]()
    {
        auto stmt = SetupSqlStatement(sql);

        size_t i = 1;
        for (const auto& address : addresses)
            TryBindStatementText(stmt, i++, address);

        while (sqlite3_step(*stmt) == SQLITE_ROW)
        {
            auto[ok, address] = TryGetColumnString(*stmt, 0);

            UniValue record(UniValue::VOBJ);

            record.pushKV("address", address);
            if (auto[ok, valueStr] = TryGetColumnString(*stmt, 1); ok) record.pushKV("name", valueStr);
            if (auto[ok, valueStr] = TryGetColumnString(*stmt, 2); ok)
                record.pushKV("id", valueStr); //TODO (brangr): check pls in pocketrpc.cpp was id + 1
            if (auto[ok, valueStr] = TryGetColumnString(*stmt, 3); ok) record.pushKV("i", valueStr);
            if (auto[ok, valueStr] = TryGetColumnString(*stmt, 4); ok) record.pushKV("b", valueStr);
            if (auto[ok, valueStr] = TryGetColumnString(*stmt, 6); ok) record.pushKV("r", valueStr);
            if (auto[ok, valueStr] = TryGetColumnString(*stmt, 11); ok) record.pushKV("rc", valueStr);
            if (auto[ok, valueStr] = TryGetColumnString(*stmt, 12); ok) record.pushKV("postcnt", valueStr);
            if (auto[ok, valueStr] = TryGetColumnString(*stmt, 13); ok) record.pushKV("reputation", valueStr);

            if (option == 1)
            {
                if (auto[ok, valueStr] = TryGetColumnString(*stmt, 5); ok) record.pushKV("a", valueStr);
            }

            if (shortForm)
            {
                result.insert_or_assign(address, record);
                continue;
            }

            if (auto[ok, valueStr] = TryGetColumnString(*stmt, 7); ok) record.pushKV("l", valueStr);
            if (auto[ok, valueStr] = TryGetColumnString(*stmt, 8); ok) record.pushKV("s", valueStr);
            if (auto[ok, valueStr] = TryGetColumnString(*stmt, 9); ok) record.pushKV("update", valueStr);
            if (auto[ok, valueStr] = TryGetColumnString(*stmt, 10); ok) record.pushKV("k", valueStr);
            if (auto[ok, valueStr] = TryGetColumnString(*stmt, 14); ok) record.pushKV("regdate", valueStr);

            result.insert_or_assign(address, record);
        }

        FinalizeSqlStatement(*stmt);
    });

    return result;
}

UniValue PocketDb::WebRepository::GetLastComments(int count, int height, std::string lang)
{
    //TODO check Reputation
    auto sql = R"sql(
        WITH RowIds AS (
            SELECT MAX(RowId) as RowId
            FROM vComments c
            INNER JOIN Payload pl ON pl.TxHash = c.Hash
            WHERE c.Height <= ?
            AND c.Time < ?
            AND pl.String1 = ?
            GROUP BY Id
            )
        SELECT t.Hash,
               t.RootTxHash,
               t.PostTxHash,
               t.AddressHash,
               t.Time,
               t.Height,
               t.ParentTxHash,
               t.AnswerTxHash,
               (SELECT COUNT(1) FROM vScoreComments sc WHERE sc.CommentTxHash = t.Hash  AND sc.Value = 1) as ScoreUp,
               (SELECT COUNT(1) FROM vScoreComments sc WHERE sc.CommentTxHash = t.Hash  AND sc.Value = -1) as ScoreDown,
               (SELECT r.Value FROM Ratings r WHERE r.Id = t.Id  AND r.Type = 3) as Reputation,
               pl.String2 AS Msg
        FROM vComments t
        INNER JOIN RowIds rid on t.RowId = rid.RowId
        INNER JOIN Payload pl ON pl.TxHash = t.Hash
        ORDER BY Height desc, Time desc
        LIMIT ?;
    )sql";

    auto result = UniValue(UniValue::VARR);

    TryTransactionStep(__func__, [&]()
    {
        auto stmt = SetupSqlStatement(sql);

        TryBindStatementInt(stmt, 1, height);
        TryBindStatementInt64(stmt, 2, GetAdjustedTime());
        TryBindStatementText(stmt, 3, lang.empty() ? "en" : lang);
        TryBindStatementInt(stmt, 4, count);

        while (sqlite3_step(*stmt) == SQLITE_ROW)
        {
            UniValue record(UniValue::VOBJ);

            auto[ok0, txHash] = TryGetColumnString(*stmt, 0);
            auto[ok1, rootTxHash] = TryGetColumnString(*stmt, 1);

            record.pushKV("id", rootTxHash);
            if (auto[ok, valueStr] = TryGetColumnString(*stmt, 2); ok) record.pushKV("postid", valueStr);
            if (auto[ok, valueStr] = TryGetColumnString(*stmt, 3); ok) record.pushKV("address", valueStr);
            if (auto[ok, valueStr] = TryGetColumnString(*stmt, 4); ok) record.pushKV("time", valueStr);
            if (auto[ok, valueStr] = TryGetColumnString(*stmt, 4); ok) record.pushKV("timeUpd", valueStr);
            if (auto[ok, valueStr] = TryGetColumnString(*stmt, 5); ok) record.pushKV("block", valueStr);
            if (auto[ok, valueStr] = TryGetColumnString(*stmt, 6); ok) record.pushKV("parentid", valueStr);
            if (auto[ok, valueStr] = TryGetColumnString(*stmt, 7); ok) record.pushKV("answerid", valueStr);
            if (auto[ok, valueStr] = TryGetColumnString(*stmt, 8); ok) record.pushKV("scoreUp", valueStr);
            if (auto[ok, valueStr] = TryGetColumnString(*stmt, 9); ok) record.pushKV("scoreDown", valueStr);
            if (auto[ok, valueStr] = TryGetColumnString(*stmt, 10); ok) record.pushKV("reputation", valueStr);

            if (auto[ok, valueStr] = TryGetColumnString(*stmt, 11); ok)
            {
                record.pushKV("msg", valueStr);
                record.pushKV("deleted", false);
            }
            else
            {
                record.pushKV("msg", "");
                record.pushKV("deleted", true);
            }

            record.pushKV("edit", txHash != rootTxHash);

            result.push_back(record);
        }

        FinalizeSqlStatement(*stmt);
    });

    return result;
}

UniValue PocketDb::WebRepository::GetCommentsByPost(const std::string& postHash, const std::string& parentHash,
    const std::string& addressHash)
{
    //TODO check Reputation
    auto sql = R"sql(
        SELECT c.Type,
           c.Hash,
           c.RootTxHash,
           c.PostTxHash,
           c.AddressHash,
           r.Time AS RootTime,
           c.Time,
           c.Height,
           pl.String2 AS Msg,
           c.ParentTxHash,
           c.AnswerTxHash,
           (SELECT COUNT(1) FROM vScoreComments sc WHERE sc.CommentTxHash = c.Hash  AND sc.Value = 1) as ScoreUp,
           (SELECT COUNT(1) FROM vScoreComments sc WHERE sc.CommentTxHash = c.Hash  AND sc.Value = -1) as ScoreDown,
           (SELECT r.Value FROM Ratings r WHERE r.Id = c.Id  AND r.Type = 3) as Reputation,
           IFNULL(SC.Value, 0) AS MyScore,
           (SELECT COUNT(1) FROM vComments s WHERE s.ParentTxHash = c.RootTxHash AND s.Last = 1) AS ChildrenCount
        FROM vComments c
        INNER JOIN vComments r ON c.RootTxHash = r.Hash
        INNER JOIN Payload pl ON pl.TxHash = c.Hash
        LEFT JOIN vScoreComments sc ON sc.CommentTxHash = C.RootTxHash AND IFNULL(sc.AddressHash, '') = ?
        WHERE c.PostTxHash = ?
            AND IFNULL(c.ParentTxHash, '') = ?
            AND c.Last = 1
            AND c.Time < ?
    )sql";

    auto result = UniValue(UniValue::VARR);

    TryTransactionStep(__func__, [&]()
    {
        auto stmt = SetupSqlStatement(sql);

        TryBindStatementText(stmt, 1, addressHash);
        TryBindStatementText(stmt, 2, postHash);
        TryBindStatementText(stmt, 3, parentHash);
        TryBindStatementInt64(stmt, 4, GetAdjustedTime());

        while (sqlite3_step(*stmt) == SQLITE_ROW)
        {
            auto record = ParseCommentRow(*stmt);

            result.push_back(record);
        }

        FinalizeSqlStatement(*stmt);
    });

    return result;
}

UniValue PocketDb::WebRepository::GetCommentsByIds(string& addressHash, vector<string>& commentHashes)
{
    //TODO get cmnIds and address
//        g_pocketdb->Select(
//            Query("Comment")
//                .Where("otxid", CondSet, cmnids)
//                .Where("last", CondEq, true)
//                .Where("time", CondLe, GetAdjustedTime())
//                .InnerJoin("otxid", "txid", CondEq, Query("Comment").Where("txid", CondSet, cmnids).Limit(1))
//                .LeftJoin("otxid", "commentid", CondEq, Query("CommentScores").Where("address", CondEq, address).Limit(1)),
//            commRes);

    //TODO Check reputation
    string sql = R"sql(
        SELECT c.Type,
           c.Hash,
           c.RootTxHash,
           c.PostTxHash,
           c.AddressHash,
           r.Time AS RootTime,
           c.Time,
           c.Height,
           pl.String2 AS Msg,
           c.ParentTxHash,
           c.AnswerTxHash,
           (SELECT COUNT(1) FROM vScoreComments sc WHERE sc.CommentTxHash = c.Hash  AND sc.Value = 1) as ScoreUp,
           (SELECT COUNT(1) FROM vScoreComments sc WHERE sc.CommentTxHash = c.Hash  AND sc.Value = -1) as ScoreDown,
           (SELECT r.Value FROM Ratings r WHERE r.Id = c.Id  AND r.Type = 3) as Reputation,
           IFNULL(SC.Value, 0) AS MyScore,
           (SELECT COUNT(1) FROM vComments s WHERE s.ParentTxHash = c.RootTxHash AND s.Last = 1) AS ChildrenCount
        FROM vComments c
        INNER JOIN vComments r ON c.RootTxHash = r.Hash
        INNER JOIN Payload pl ON pl.TxHash = c.Hash
        LEFT JOIN vScoreComments sc ON sc.CommentTxHash = C.RootTxHash AND IFNULL(sc.AddressHash, '') = ?
        WHERE c.Time < ?
            AND c.Last = 1
    )sql";

    sql += "AND c.RootTxHash IN ('";
    sql += commentHashes[0];
    sql += "'";
    for (size_t i = 1; i < commentHashes.size(); i++)
    {
        sql += ",'";
        sql += commentHashes[i];
        sql += "'";
    }
    sql += ")";

    auto result = UniValue(UniValue::VARR);

    TryTransactionStep(__func__, [&]()
    {
        auto stmt = SetupSqlStatement(sql);

        TryBindStatementText(stmt, 1, addressHash);
        TryBindStatementInt64(stmt, 2, GetAdjustedTime());

        while (sqlite3_step(*stmt) == SQLITE_ROW)
        {
            auto record = ParseCommentRow(*stmt);

            result.push_back(record);
        }

        FinalizeSqlStatement(*stmt);
    });

    return result;
}

UniValue PocketDb::WebRepository::GetPostScores(vector<string>& postHashes, string& addressHash)
{
    auto result = UniValue(UniValue::VARR);

    string sql = R"sql(
        select
            sc.String2, -- post
            sc.String1, -- address
            p.String2, -- name
            p.String3, -- avatar
            (select r.Value from Ratings r where r.Type=0 and r.Id=u.Id order by Height desc limit 1), -- user reputation
            sc.Int1, -- score value
            case
                when sub.Type is null then null
                when sub.Type = 303 then 1
                else 0
            end -- isPrivate
        from Transactions sc indexed by Transactions_Type_Last_String1_String2_Height

        join Transactions u indexed by Transactions_Type_Last_String1_String2_Height
            on u.Type in (100, 101, 102) and u.Last = 1 and u.String1=sc.String1 and u.Height is not null

        join Payload p ON p.TxHash=u.Hash

        left join Transactions sub indexed by Transactions_Type_Last_String1_String2_Height
            on sub.Type in (302, 303) and sub.Last=1 and sub.Height is not null and sub.String2=sc.String1 AND sub.String1=?

        where sc.Type in (300) and sc.Height is not null
            and sc.String2 in ()sql" + join(vector<string>(postHashes.size(), "?"), ",") + R"sql()
        order by sub.Type is null, sc.Time
    )sql";

    TryTransactionStep(__func__, [&]()
    {
        auto stmt = SetupSqlStatement(sql);

        TryBindStatementText(stmt, 1, addressHash);

        int i = 2;
        for (const auto& postHash : postHashes)
            TryBindStatementText(stmt, i++, postHash);

        while (sqlite3_step(*stmt) == SQLITE_ROW)
        {
            UniValue record(UniValue::VOBJ);

            if (auto[ok, valueStr] = TryGetColumnString(*stmt, 0); ok) record.pushKV("posttxid", valueStr);
            if (auto[ok, valueStr] = TryGetColumnString(*stmt, 1); ok) record.pushKV("address", valueStr);
            if (auto[ok, valueStr] = TryGetColumnString(*stmt, 2); ok) record.pushKV("name", valueStr);
            if (auto[ok, valueStr] = TryGetColumnString(*stmt, 3); ok) record.pushKV("avatar", valueStr);
            if (auto[ok, valueStr] = TryGetColumnString(*stmt, 4); ok) record.pushKV("reputation", valueStr);
            if (auto[ok, valueStr] = TryGetColumnString(*stmt, 5); ok) record.pushKV("value", valueStr);
            if (auto[ok, valueStr] = TryGetColumnString(*stmt, 6); ok) record.pushKV("isprivate", valueStr);

            result.push_back(record);
        }

        FinalizeSqlStatement(*stmt);
    });

    return result;
}

UniValue PocketDb::WebRepository::GetPageScores(std::vector<std::string>& commentHashes, std::string& addressHash)
{
    //TODO check reputation
    string sql = R"sql(
        SELECT
           c.RootTxHash,
           (SELECT COUNT(1) FROM vScoreComments sc WHERE sc.CommentTxHash = c.Hash AND sc.Value = 1) as ScoreUp,
           (SELECT COUNT(1) FROM vScoreComments sc WHERE sc.CommentTxHash = c.Hash AND sc.Value = -1) as ScoreDown,
           (SELECT r.Value FROM Ratings r WHERE r.Id = c.Id  AND r.Type = 3) as Reputation,
           msc.Value AS MyScore
        FROM vComments c
        LEFT JOIN vScoreComments msc ON msc.CommentTxHash = C.RootTxHash AND msc.AddressHash = ?
        WHERE c.Time < ?
            AND c.Last = 1
    )sql";

    sql += "AND c.RootTxHash IN ('";
    sql += commentHashes[0];
    sql += "'";
    for (size_t i = 1; i < commentHashes.size(); i++)
    {
        sql += ",'";
        sql += commentHashes[i];
        sql += "'";
    }
    sql += ")";

    auto result = UniValue(UniValue::VARR);

    TryTransactionStep(__func__, [&]()
    {
        auto stmt = SetupSqlStatement(sql);

        TryBindStatementText(stmt, 1, addressHash);
        TryBindStatementInt64(stmt, 2, GetAdjustedTime());

        while (sqlite3_step(*stmt) == SQLITE_ROW)
        {
            UniValue record(UniValue::VOBJ);

            if (auto[ok, valueStr] = TryGetColumnString(*stmt, 0); ok) record.pushKV("cmntid", valueStr);
            if (auto[ok, valueStr] = TryGetColumnString(*stmt, 1); ok) record.pushKV("scoreUp", valueStr);
            if (auto[ok, valueStr] = TryGetColumnString(*stmt, 2); ok) record.pushKV("scoreDown", valueStr);
            if (auto[ok, valueStr] = TryGetColumnString(*stmt, 3); ok) record.pushKV("reputation", valueStr);
            if (auto[ok, valueStr] = TryGetColumnString(*stmt, 4); ok) record.pushKV("myscore", valueStr);

            result.push_back(record);
        }

        FinalizeSqlStatement(*stmt);
    });

    return result;
}

UniValue PocketDb::WebRepository::ParseCommentRow(sqlite3_stmt* stmt)
{
    UniValue record(UniValue::VOBJ);

    //auto[ok0, txHash] = TryGetColumnString(stmt, 1);
    auto[ok1, rootTxHash] = TryGetColumnString(stmt, 2);
    record.pushKV("id", rootTxHash);

    if (auto[ok, valueStr] = TryGetColumnString(stmt, 3); ok)
        record.pushKV("postid", valueStr);

    auto[ok8, msgValue] = TryGetColumnString(stmt, 8);
    record.pushKV("msg", msgValue);

    if (auto[ok, valueStr] = TryGetColumnString(stmt, 4); ok) record.pushKV("address", valueStr);
    if (auto[ok, valueStr] = TryGetColumnString(stmt, 5); ok) record.pushKV("time", valueStr);
    if (auto[ok, valueStr] = TryGetColumnString(stmt, 6); ok) record.pushKV("timeUpd", valueStr);
    if (auto[ok, valueStr] = TryGetColumnString(stmt, 7); ok) record.pushKV("block", valueStr);
    if (auto[ok, valueStr] = TryGetColumnString(stmt, 9); ok) record.pushKV("parentid", valueStr);
    if (auto[ok, valueStr] = TryGetColumnString(stmt, 10); ok) record.pushKV("answerid", valueStr);
    if (auto[ok, valueStr] = TryGetColumnString(stmt, 11); ok) record.pushKV("scoreUp", valueStr);
    if (auto[ok, valueStr] = TryGetColumnString(stmt, 12); ok) record.pushKV("scoreDown", valueStr);
    if (auto[ok, valueStr] = TryGetColumnString(stmt, 13); ok) record.pushKV("reputation", valueStr);
    if (auto[ok, valueStr] = TryGetColumnString(stmt, 14); ok) record.pushKV("myScore", valueStr);
    if (auto[ok, valueStr] = TryGetColumnString(stmt, 15); ok) record.pushKV("children", valueStr);

    if (auto[ok, value] = TryGetColumnInt(stmt, 0); ok)
    {
        switch (static_cast<PocketTxType>(value))
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

    return record;
}

std::map<std::string, UniValue> PocketDb::WebRepository::GetSubscribesAddresses(std::vector<std::string>& addresses)
{
    string sql = R"sql(
        SELECT AddressHash,
               AddressToHash,
               CASE
                   WHEN Type = 303 THEN 1
                   ELSE 0
               END AS Private
        FROM vSubscribes
        WHERE Last = 1
            AND Type IN (302, 303) -- Subscribe, Private Subscribe
    )sql";

    sql += "AND AddressHash IN ('";
    sql += addresses[0];
    sql += "'";
    for (size_t i = 1; i < addresses.size(); i++)
    {
        sql += ",'";
        sql += addresses[i];
        sql += "'";
    }
    sql += ")";

    auto result = map<std::string, UniValue>();
    for (const auto& address : addresses)
    {
        result[address] = UniValue(UniValue::VARR);
    }

    TryTransactionStep(__func__, [&]()
    {
        auto stmt = SetupSqlStatement(sql);

        while (sqlite3_step(*stmt) == SQLITE_ROW)
        {
            UniValue record(UniValue::VOBJ);
            auto[ok, address] = TryGetColumnString(*stmt, 0);

            if (auto[ok, valueStr] = TryGetColumnString(*stmt, 1); ok) record.pushKV("adddress", valueStr); //Not mistake
            if (auto[ok, valueStr] = TryGetColumnString(*stmt, 2); ok) record.pushKV("private", valueStr);

            result[address].push_back(record);
        }

        FinalizeSqlStatement(*stmt);
    });

    return result;
}

std::map<std::string, UniValue> PocketDb::WebRepository::GetSubscribersAddresses(std::vector<std::string>& addresses)
{
    string sql = R"sql(
        SELECT AddressToHash,
            AddressHash
        FROM vSubscribes
        WHERE Last = 1
            AND Type IN (302, 303) -- Subscribe, Private Subscribe
    )sql";

    sql += "AND AddressToHash IN ('";
    sql += addresses[0];
    sql += "'";
    for (size_t i = 1; i < addresses.size(); i++)
    {
        sql += ",'";
        sql += addresses[i];
        sql += "'";
    }
    sql += ")";

    auto result = map<std::string, UniValue>();
    for (const auto& address : addresses)
    {
        result[address] = UniValue(UniValue::VARR);
    }

    TryTransactionStep(__func__, [&]()
    {
        auto stmt = SetupSqlStatement(sql);

        while (sqlite3_step(*stmt) == SQLITE_ROW)
        {
            auto[ok, addressTo] = TryGetColumnString(*stmt, 0);
            auto[ok1, address] = TryGetColumnString(*stmt, 1);

            result[addressTo].push_back(address);
        }

        FinalizeSqlStatement(*stmt);
    });

    return result;
}

std::map<std::string, UniValue> PocketDb::WebRepository::GetBlockingToAddresses(std::vector<std::string>& addresses)
{
    string sql = R"sql(
        SELECT AddressHash, AddressToHash
        FROM vBlockings
        WHERE Last = 1
            AND Type = 305
    )sql";

    sql += "AND AddressHash IN ('";
    sql += addresses[0];
    sql += "'";
    for (size_t i = 1; i < addresses.size(); i++)
    {
        sql += ",'";
        sql += addresses[i];
        sql += "'";
    }
    sql += ")";

    auto result = map<std::string, UniValue>();
    for (const auto& address : addresses)
    {
        result[address] = UniValue(UniValue::VARR);
    }

    TryTransactionStep(__func__, [&]()
    {
        auto stmt = SetupSqlStatement(sql);

        while (sqlite3_step(*stmt) == SQLITE_ROW)
        {
            auto[ok, address] = TryGetColumnString(*stmt, 0);
            auto[ok1, addressTo] = TryGetColumnString(*stmt, 1);

            result[address].push_back(addressTo);
        }

        FinalizeSqlStatement(*stmt);
    });

    return result;
}

std::map<std::string, UniValue>
PocketDb::WebRepository::GetContents(int nHeight, std::string start_txid, int countOut, std::string lang, std::vector<string> tags,
    std::vector<int> contentTypes, std::vector<string> txidsExcluded, std::vector<string> adrsExcluded, std::vector<string> tagsExcluded,
    std::string address)
{
    string sql = R"sql(
        SELECT t.String2 as RootTxHash,
               case when t.Hash != t.String2 then 'true' else '' end edit,
               t.String3 as RelayTxHash,
               t.String1 as AddressHash,
               t.Time,
               p.String1 as Lang,
               t.Type,
               p.String2 as Caption,
               p.String3 as Message,
               p.String7 as Url,
               p.String4 as Tags,
               p.String5 as Images,
               p.String6 as Settings
        FROM Transactions t indexed by Transactions_Height_Time
            JOIN Payload p on t.Hash = p.TxHash
        where t.Id > ifnull((select max(t0.Id) from Transactions t0 indexed by Transactions_Type_Last_String2_Height where t0.Type in (200, 201) and t0.String2 = ? and t0.Last = 1),0)
          and t.Last = 1
          and t.Height <= ?
          and t.Time <= ?
          and t.String3 is null
          and p.String1 = ?
          and t.Type in (200, 201)
        order by t.Height desc, t.Time desc
        limit ?
    )sql";

    std::map<std::string, UniValue> result{};

    TryTransactionStep(__func__, [&]()
    {
        auto stmt = SetupSqlStatement(sql);

        TryBindStatementText(stmt, 1, start_txid);
        TryBindStatementInt(stmt, 2, nHeight);
        TryBindStatementInt64(stmt, 3, GetAdjustedTime());
        TryBindStatementText(stmt, 4, lang);
        TryBindStatementInt(stmt, 5, countOut);

        while (sqlite3_step(*stmt) == SQLITE_ROW)
        {
            UniValue record(UniValue::VOBJ);

            auto[ok, txid] = TryGetColumnString(*stmt, 0);
            record.pushKV("txid", txid);

            if (auto[ok, valueStr] = TryGetColumnString(*stmt, 1); ok) record.pushKV("edit", valueStr);
            if (auto[ok, valueStr] = TryGetColumnString(*stmt, 2); ok) record.pushKV("repost", valueStr);
            if (auto[ok, valueStr] = TryGetColumnString(*stmt, 3); ok) record.pushKV("address", valueStr);
            if (auto[ok, valueStr] = TryGetColumnString(*stmt, 4); ok) record.pushKV("time", valueStr);
            if (auto[ok, valueStr] = TryGetColumnString(*stmt, 5); ok) record.pushKV("l", valueStr); // lang
            if (auto[ok, valueStr] = TryGetColumnString(*stmt, 6); ok) record.pushKV("type", valueStr);
            if (auto[ok, valueStr] = TryGetColumnString(*stmt, 7); ok) record.pushKV("c", valueStr); // caption
            if (auto[ok, valueStr] = TryGetColumnString(*stmt, 8); ok) record.pushKV("m", valueStr); // message
            if (auto[ok, valueStr] = TryGetColumnString(*stmt, 9); ok) record.pushKV("u", valueStr); // url
            if (auto[ok, valueStr] = TryGetColumnString(*stmt, 10); ok)
            {
                UniValue t(UniValue::VARR);
                record.pushKV("t", t);
            }
            if (auto[ok, valueStr] = TryGetColumnString(*stmt, 11); ok)
            {
                UniValue t(UniValue::VARR);
                record.pushKV("i", t);
            }
            if (auto[ok, valueStr] = TryGetColumnString(*stmt, 12); ok) record.pushKV("settings", valueStr);
            //if (auto [ok, valueStr] = TryGetColumnString(*stmt, 0); ok) record.pushKV("scoreSum", valueStr);
            //if (auto [ok, valueStr] = TryGetColumnString(*stmt, 0); ok) record.pushKV("scoreCnt", valueStr);
            //if (auto [ok, valueStr] = TryGetColumnString(*stmt, 0); ok) record.pushKV("myVal", valueStr);
            //if (auto [ok, valueStr] = TryGetColumnString(*stmt, 0); ok) record.pushKV("comments", valueStr);
            //if (auto [ok, valueStr] = TryGetColumnString(*stmt, 0); ok) record.pushKV("lastComment", valueStr);
            //if (auto [ok, valueStr] = TryGetColumnString(*stmt, 0); ok) record.pushKV("reposted", valueStr);
            //if (auto [ok, valueStr] = TryGetColumnString(*stmt, 0); ok) record.pushKV("userprofile", valueStr);

            //          record.pushKV("address", address);
            //          if (auto [ok, valueStr] = TryGetColumnString(*stmt, 1); ok) record.pushKV("name", valueStr);
            //          if (auto [ok, valueStr] = TryGetColumnString(*stmt, 2); ok) record.pushKV("id", valueStr);
            //          if (auto [ok, valueStr] = TryGetColumnString(*stmt, 3); ok) record.pushKV("i", valueStr);
            //          if (auto [ok, valueStr] = TryGetColumnString(*stmt, 4); ok) record.pushKV("b", valueStr);
            //          if (auto [ok, valueStr] = TryGetColumnString(*stmt, 6); ok) record.pushKV("r", valueStr);
            //          if (auto [ok, valueStr] = TryGetColumnString(*stmt, 11); ok) record.pushKV("rc", valueStr);
            //          if (auto [ok, valueStr] = TryGetColumnString(*stmt, 12); ok) record.pushKV("postcnt", valueStr);
            //          if (auto [ok, valueStr] = TryGetColumnString(*stmt, 13); ok) record.pushKV("reputation", valueStr);
            //
            ////          if (option == 1)
            ////          {
            ////              if (auto [ok, valueStr] = TryGetColumnString(*stmt, 5); ok) record.pushKV("a", valueStr);
            ////          }
            ////
            ////          if (shortForm)
            ////          {
            ////              result.insert_or_assign(address, record);
            ////              continue;
            ////          }
            //
            //          if (auto [ok, valueStr] = TryGetColumnString(*stmt, 7); ok) record.pushKV("l", valueStr);
            //          if (auto [ok, valueStr] = TryGetColumnString(*stmt, 8); ok) record.pushKV("s", valueStr);
            //          if (auto [ok, valueStr] = TryGetColumnString(*stmt, 9); ok) record.pushKV("update", valueStr);
            //          if (auto [ok, valueStr] = TryGetColumnString(*stmt, 10); ok) record.pushKV("k", valueStr);
            //          if (auto [ok, valueStr] = TryGetColumnString(*stmt, 14); ok) record.pushKV("regdate", valueStr);

            result.insert_or_assign(txid, record);
        }

        FinalizeSqlStatement(*stmt);
    });

    return result;
}

/*
std::map<std::string, UniValue> PocketDb::WebRepository::GetContentsData(std::vector<std::string>& txids)
{
    string sql = R"sql(
        SELECT c.RootTxHash,
               case when c.Hash!=c.RootTxHash then 'true' else '' end edit,
               c.RelayTxHash,
               c.AddressHash,
               c.Time,
               c.Lang,
               c.Type,
               c.Caption,
               c.Message,
               c.Url,
               c.Tags,
               c.Images,
               c.Settings
        FROM vWebContents c
        WHERE 1 = 1
    )sql";

    sql += "AND c.RootTxHash IN ('";
    sql += txids[0];
    sql += "'";
    for (size_t i = 1; i < txids.size(); i++)
    {
        sql += ",'";
        sql += txids[i];
        sql += "'";
    }
    sql += ")";

    std::map<std::string, UniValue> result{};

    TryTransactionStep(__func__, [&]() {
      auto stmt = SetupSqlStatement(sql);

      while (sqlite3_step(*stmt) == SQLITE_ROW)
      {
          UniValue record(UniValue::VOBJ);

          auto [ok, txid] = TryGetColumnString(*stmt, 0);
          record.pushKV("txid", txid);

          if (auto [ok, valueStr] = TryGetColumnString(*stmt, 1); ok) record.pushKV("edit", valueStr);
          if (auto [ok, valueStr] = TryGetColumnString(*stmt, 2); ok) record.pushKV("repost", valueStr);
          if (auto [ok, valueStr] = TryGetColumnString(*stmt, 3); ok) record.pushKV("address", valueStr);
          if (auto [ok, valueStr] = TryGetColumnString(*stmt, 4); ok) record.pushKV("time", valueStr);
          if (auto [ok, valueStr] = TryGetColumnString(*stmt, 5); ok) record.pushKV("l", valueStr); // lang
          if (auto [ok, valueStr] = TryGetColumnString(*stmt, 6); ok) record.pushKV("type", valueStr);
          if (auto [ok, valueStr] = TryGetColumnString(*stmt, 7); ok) record.pushKV("c", valueStr); // caption
          if (auto [ok, valueStr] = TryGetColumnString(*stmt, 8); ok) record.pushKV("m", valueStr); // message
          if (auto [ok, valueStr] = TryGetColumnString(*stmt, 9); ok) record.pushKV("u", valueStr); // url
          if (auto [ok, valueStr] = TryGetColumnString(*stmt, 10); ok) {
              UniValue t(UniValue::VARR);
              record.pushKV("t", t);
          }
          if (auto [ok, valueStr] = TryGetColumnString(*stmt, 11); ok) {
              UniValue t(UniValue::VARR);
              record.pushKV("i", t);
          }
          if (auto [ok, valueStr] = TryGetColumnString(*stmt, 12); ok) record.pushKV("settings", valueStr);
          //if (auto [ok, valueStr] = TryGetColumnString(*stmt, 0); ok) record.pushKV("scoreSum", valueStr);
          //if (auto [ok, valueStr] = TryGetColumnString(*stmt, 0); ok) record.pushKV("scoreCnt", valueStr);
          //if (auto [ok, valueStr] = TryGetColumnString(*stmt, 0); ok) record.pushKV("myVal", valueStr);
          //if (auto [ok, valueStr] = TryGetColumnString(*stmt, 0); ok) record.pushKV("comments", valueStr);
          //if (auto [ok, valueStr] = TryGetColumnString(*stmt, 0); ok) record.pushKV("lastComment", valueStr);
          //if (auto [ok, valueStr] = TryGetColumnString(*stmt, 0); ok) record.pushKV("reposted", valueStr);
          //if (auto [ok, valueStr] = TryGetColumnString(*stmt, 0); ok) record.pushKV("userprofile", valueStr);

//          record.pushKV("address", address);
//          if (auto [ok, valueStr] = TryGetColumnString(*stmt, 1); ok) record.pushKV("name", valueStr);
//          if (auto [ok, valueStr] = TryGetColumnString(*stmt, 2); ok) record.pushKV("id", valueStr); //TODO (brangr): check pls in pocketrpc.cpp was id + 1
//          if (auto [ok, valueStr] = TryGetColumnString(*stmt, 3); ok) record.pushKV("i", valueStr);
//          if (auto [ok, valueStr] = TryGetColumnString(*stmt, 4); ok) record.pushKV("b", valueStr);
//          if (auto [ok, valueStr] = TryGetColumnString(*stmt, 6); ok) record.pushKV("r", valueStr);
//          if (auto [ok, valueStr] = TryGetColumnString(*stmt, 11); ok) record.pushKV("rc", valueStr);
//          if (auto [ok, valueStr] = TryGetColumnString(*stmt, 12); ok) record.pushKV("postcnt", valueStr);
//          if (auto [ok, valueStr] = TryGetColumnString(*stmt, 13); ok) record.pushKV("reputation", valueStr);
//
////          if (option == 1)
////          {
////              if (auto [ok, valueStr] = TryGetColumnString(*stmt, 5); ok) record.pushKV("a", valueStr);
////          }
////
////          if (shortForm)
////          {
////              result.insert_or_assign(address, record);
////              continue;
////          }
//
//          if (auto [ok, valueStr] = TryGetColumnString(*stmt, 7); ok) record.pushKV("l", valueStr);
//          if (auto [ok, valueStr] = TryGetColumnString(*stmt, 8); ok) record.pushKV("s", valueStr);
//          if (auto [ok, valueStr] = TryGetColumnString(*stmt, 9); ok) record.pushKV("update", valueStr);
//          if (auto [ok, valueStr] = TryGetColumnString(*stmt, 10); ok) record.pushKV("k", valueStr);
//          if (auto [ok, valueStr] = TryGetColumnString(*stmt, 14); ok) record.pushKV("regdate", valueStr);

          result.insert_or_assign(txid, record);
      }

      FinalizeSqlStatement(*stmt);
    });

    return result;
}

std::map<std::string, UniValue> PocketDb::WebRepository::GetContents(std::map<std::string, param>& conditions, std::optional<int> &counttotal)
{
    string sql = R"sql(
        SELECT c.RootTxHash,
               case when c.Hash!=c.RootTxHash then 'true' else '' end edit,
               c.RelayTxHash,
               c.AddressHash,
               c.Time,
               c.Lang,
               c.Type,
               c.Caption,
               c.Message,
               c.Url,
               c.Tags,
               c.Images,
               c.Settings
        FROM vWebContents c
        WHERE 1 = 1
    )sql";

    sql += strprintf(" AND c.Time <= %d", GetAdjustedTime());
    sql += " AND c.RelayTxHash is null";
    if (conditions.count("height")) sql += strprintf(" AND c.Height <= %i", conditions["height"].get_int());
    if (conditions.count("lang")) sql += strprintf(" AND c.Lang = '%s'", conditions["lang"].get_str().c_str());
    //if (conditions.count("tags")) sql += strprintf(" AND c.Tags = '%s'", conditions["tags"].get_str().c_str());
    if (conditions.count("type") && conditions["types"].get_vint().size()) {
        sql += " AND c.Type in (";
        sql += conditions["types"].get_vint()[0];
        for (size_t i = 1; i < conditions["types"].get_vint().size(); i++)
        {
            sql += ",";
            sql += conditions["types"].get_vint()[i];
        }
        sql += ")";
    }
    sql += " order by c.Height desc, c.Time desc";
    sql += strprintf(" limit %i", conditions.count("count") ? conditions["count"].get_int() : 10);

    std::map<std::string, UniValue> result{};

    TryTransactionStep(__func__, [&]() {
      auto stmt = SetupSqlStatement(sql);

      while (sqlite3_step(*stmt) == SQLITE_ROW)
      {
          UniValue record(UniValue::VOBJ);

          auto [ok, txid] = TryGetColumnString(*stmt, 0);
          record.pushKV("txid", txid);

          if (auto [ok, valueStr] = TryGetColumnString(*stmt, 1); ok) record.pushKV("edit", valueStr);
          if (auto [ok, valueStr] = TryGetColumnString(*stmt, 2); ok) record.pushKV("repost", valueStr);
          if (auto [ok, valueStr] = TryGetColumnString(*stmt, 3); ok) record.pushKV("address", valueStr);
          if (auto [ok, valueStr] = TryGetColumnString(*stmt, 4); ok) record.pushKV("time", valueStr);
          if (auto [ok, valueStr] = TryGetColumnString(*stmt, 5); ok) record.pushKV("l", valueStr); // lang
          if (auto [ok, valueStr] = TryGetColumnString(*stmt, 6); ok) record.pushKV("type", valueStr);
          if (auto [ok, valueStr] = TryGetColumnString(*stmt, 7); ok) record.pushKV("c", valueStr); // caption
          if (auto [ok, valueStr] = TryGetColumnString(*stmt, 8); ok) record.pushKV("m", valueStr); // message
          if (auto [ok, valueStr] = TryGetColumnString(*stmt, 9); ok) record.pushKV("u", valueStr); // url
          if (auto [ok, valueStr] = TryGetColumnString(*stmt, 10); ok) {
              UniValue t(UniValue::VARR);
              record.pushKV("t", t);
          }
          if (auto [ok, valueStr] = TryGetColumnString(*stmt, 11); ok) {
              UniValue t(UniValue::VARR);
              record.pushKV("i", t);
          }
          if (auto [ok, valueStr] = TryGetColumnString(*stmt, 12); ok) record.pushKV("settings", valueStr);
          //if (auto [ok, valueStr] = TryGetColumnString(*stmt, 0); ok) record.pushKV("scoreSum", valueStr);
          //if (auto [ok, valueStr] = TryGetColumnString(*stmt, 0); ok) record.pushKV("scoreCnt", valueStr);
          //if (auto [ok, valueStr] = TryGetColumnString(*stmt, 0); ok) record.pushKV("myVal", valueStr);
          //if (auto [ok, valueStr] = TryGetColumnString(*stmt, 0); ok) record.pushKV("comments", valueStr);
          //if (auto [ok, valueStr] = TryGetColumnString(*stmt, 0); ok) record.pushKV("lastComment", valueStr);
          //if (auto [ok, valueStr] = TryGetColumnString(*stmt, 0); ok) record.pushKV("reposted", valueStr);
          //if (auto [ok, valueStr] = TryGetColumnString(*stmt, 0); ok) record.pushKV("userprofile", valueStr);

//          record.pushKV("address", address);
//          if (auto [ok, valueStr] = TryGetColumnString(*stmt, 1); ok) record.pushKV("name", valueStr);
//          if (auto [ok, valueStr] = TryGetColumnString(*stmt, 2); ok) record.pushKV("id", valueStr);
//          if (auto [ok, valueStr] = TryGetColumnString(*stmt, 3); ok) record.pushKV("i", valueStr);
//          if (auto [ok, valueStr] = TryGetColumnString(*stmt, 4); ok) record.pushKV("b", valueStr);
//          if (auto [ok, valueStr] = TryGetColumnString(*stmt, 6); ok) record.pushKV("r", valueStr);
//          if (auto [ok, valueStr] = TryGetColumnString(*stmt, 11); ok) record.pushKV("rc", valueStr);
//          if (auto [ok, valueStr] = TryGetColumnString(*stmt, 12); ok) record.pushKV("postcnt", valueStr);
//          if (auto [ok, valueStr] = TryGetColumnString(*stmt, 13); ok) record.pushKV("reputation", valueStr);
//
////          if (option == 1)
////          {
////              if (auto [ok, valueStr] = TryGetColumnString(*stmt, 5); ok) record.pushKV("a", valueStr);
////          }
////
////          if (shortForm)
////          {
////              result.insert_or_assign(address, record);
////              continue;
////          }
//
//          if (auto [ok, valueStr] = TryGetColumnString(*stmt, 7); ok) record.pushKV("l", valueStr);
//          if (auto [ok, valueStr] = TryGetColumnString(*stmt, 8); ok) record.pushKV("s", valueStr);
//          if (auto [ok, valueStr] = TryGetColumnString(*stmt, 9); ok) record.pushKV("update", valueStr);
//          if (auto [ok, valueStr] = TryGetColumnString(*stmt, 10); ok) record.pushKV("k", valueStr);
//          if (auto [ok, valueStr] = TryGetColumnString(*stmt, 14); ok) record.pushKV("regdate", valueStr);

          result.insert_or_assign(txid, record);
      }

      FinalizeSqlStatement(*stmt);
    });

    return result;
}

std::map<std::string, UniValue> PocketDb::WebRepository::GetContents(std::map<std::string, param>& conditions)
{
    //return this->GetContents(conditions, &std::nullopt);
}*/
