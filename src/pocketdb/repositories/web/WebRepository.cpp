// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 Bitcoin developers
// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include "WebRepository.h"

PocketDb::WebRepository::param::param() { i.reset(); s.reset(); vi.reset(); vs.reset(); }
PocketDb::WebRepository::param::param(int _i) { i = _i; s.reset(); vi.reset(); vs.reset(); }
PocketDb::WebRepository::param::param(std::string _s) { s = _s; i.reset(); vi.reset(); vs.reset(); }
PocketDb::WebRepository::param::param(std::vector<int> _vi) { vi = _vi; i.reset(); s.reset(); vs.reset(); }
PocketDb::WebRepository::param::param(std::vector<std::string> _vs) { vs = _vs; i.reset(); s.reset(); vi.reset(); }

int PocketDb::WebRepository::param::get_int() { return i.value_or(0); }
std::string PocketDb::WebRepository::param::get_str() { return s.value_or(""); }
std::vector<int> PocketDb::WebRepository::param::get_vint() { return vi.value_or(std::vector<int>()); }
std::vector<std::string> PocketDb::WebRepository::param::get_vstring() { return vs.value_or(std::vector<std::string>()); }

void PocketDb::WebRepository::Init() {}

void PocketDb::WebRepository::Destroy() {}

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
    //TODO check reputation
    string sql = R"sql(
        SELECT sc.PostTxHash,
           sc.AddressHash,
           p.String2 AS Name,
           p.String3 AS Avatar,
           (SELECT Value FROM Ratings r WHERE r.Type = 0 AND r.Id = u.Id) AS Reputation,
           sc.Value,
           CASE
               WHEN sub.Type IS NULL THEN NULL
               WHEN sub.Type = 303 THEN 1
               ELSE 0
           END AS Private
        FROM vScoreContents sc
        LEFT JOIN vSubscribes sub ON sc.AddressHash = sub.AddressToHash AND sub.AddressHash = ?
        INNER JOIN vUsers u ON sc.AddressHash = u.AddressHash AND u.Last = 1
        INNER JOIN Payload p ON p.TxHash = u.Hash
        WHERE 1 = 1
    )sql";

    sql += "AND sc.PostTxHash IN ('";
    sql += postHashes[0];
    sql += "'";
    for (size_t i = 1; i < postHashes.size(); i++)
    {
        sql += ",'";
        sql += postHashes[i];
        sql += "'";
    }
    sql += ")";
    sql += "ORDER BY sub.Type IS NULL, sc.Time";

    auto result = UniValue(UniValue::VARR);

    TryTransactionStep(__func__, [&]()
    {
        auto stmt = SetupSqlStatement(sql);

        TryBindStatementText(stmt, 1, addressHash);

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

    auto[ok0, txHash] = TryGetColumnString(stmt, 1);
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

    TryTransactionStep(__func__, [&](){
      auto stmt = SetupSqlStatement(sql);

      while (sqlite3_step(*stmt) == SQLITE_ROW)
      {
          UniValue record(UniValue::VOBJ);
          auto [ok, address] = TryGetColumnString(*stmt, 0);

          if (auto [ok, valueStr] = TryGetColumnString(*stmt, 1); ok) record.pushKV("adddress", valueStr); //Not mistake
          if (auto [ok, valueStr] = TryGetColumnString(*stmt, 2); ok) record.pushKV("private", valueStr);

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

    TryTransactionStep(__func__, [&](){
      auto stmt = SetupSqlStatement(sql);

      while (sqlite3_step(*stmt) == SQLITE_ROW)
      {
          auto [ok, addressTo] = TryGetColumnString(*stmt, 0);
          auto [ok1, address] = TryGetColumnString(*stmt, 1);

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

    TryTransactionStep(__func__, [&](){
      auto stmt = SetupSqlStatement(sql);

      while (sqlite3_step(*stmt) == SQLITE_ROW)
      {
          auto [ok, address] = TryGetColumnString(*stmt, 0);
          auto [ok1, addressTo] = TryGetColumnString(*stmt, 1);

          result[address].push_back(addressTo);
      }

      FinalizeSqlStatement(*stmt);
    });

    return result;
}

std::map<std::string, UniValue> PocketDb::WebRepository::GetUserProfile(std::vector<std::string>& addresses, bool shortForm, int option)
{
    string sql = R"sql(
        SELECT u.AddressHash,
               u.Name,
               u.Id,
               u.Avatar,
               u.Donations,
               u.About,
               u.ReferrerAddressHash,
               u.Lang,
               u.Url,
               u.Time,
               u.Pubkey,
               (SELECT COUNT(1) FROM vUsers ru WHERE ru.ReferrerAddressHash = u.AddressHash AND ru.Last = 1) AS ReferrersCount,
               (SELECT COUNT(1) FROM vPosts po WHERE po.AddressHash = u.AddressHash AND po.Last = 1) AS PostsCount,
               (SELECT r.Value FROM Ratings r WHERE r.Type = 0 AND r.Id = u.Id ORDER BY r.Height DESC LIMIT 1) / 10 AS Reputation,
               (SELECT reg.Time FROM vUsers reg WHERE reg.Id = u.Id ORDER BY reg.Height LIMIT 1) AS RegistrationDate
        FROM vWebUsers u
        WHERE 1 = 1
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

    std::map<std::string, UniValue> result{};

    TryTransactionStep(__func__, [&]() {
      auto stmt = SetupSqlStatement(sql);

      while (sqlite3_step(*stmt) == SQLITE_ROW)
      {
          auto [ok, address] = TryGetColumnString(*stmt, 0);

          UniValue record(UniValue::VOBJ);

          record.pushKV("address", address);
          if (auto [ok, valueStr] = TryGetColumnString(*stmt, 1); ok) record.pushKV("name", valueStr);
          if (auto [ok, valueStr] = TryGetColumnString(*stmt, 2); ok) record.pushKV("id", valueStr); //TODO (brangr): check pls in pocketrpc.cpp was id + 1
          if (auto [ok, valueStr] = TryGetColumnString(*stmt, 3); ok) record.pushKV("i", valueStr);
          if (auto [ok, valueStr] = TryGetColumnString(*stmt, 4); ok) record.pushKV("b", valueStr);
          if (auto [ok, valueStr] = TryGetColumnString(*stmt, 6); ok) record.pushKV("r", valueStr);
          if (auto [ok, valueStr] = TryGetColumnString(*stmt, 11); ok) record.pushKV("rc", valueStr);
          if (auto [ok, valueStr] = TryGetColumnString(*stmt, 12); ok) record.pushKV("postcnt", valueStr);
          if (auto [ok, valueStr] = TryGetColumnString(*stmt, 13); ok) record.pushKV("reputation", valueStr);

          if (option == 1)
          {
              if (auto [ok, valueStr] = TryGetColumnString(*stmt, 5); ok) record.pushKV("a", valueStr);
          }

          if (shortForm)
          {
              result.insert_or_assign(address, record);
              continue;
          }

          if (auto [ok, valueStr] = TryGetColumnString(*stmt, 7); ok) record.pushKV("l", valueStr);
          if (auto [ok, valueStr] = TryGetColumnString(*stmt, 8); ok) record.pushKV("s", valueStr);
          if (auto [ok, valueStr] = TryGetColumnString(*stmt, 9); ok) record.pushKV("update", valueStr);
          if (auto [ok, valueStr] = TryGetColumnString(*stmt, 10); ok) record.pushKV("k", valueStr);
          if (auto [ok, valueStr] = TryGetColumnString(*stmt, 14); ok) record.pushKV("regdate", valueStr);

          result.insert_or_assign(address, record);
      }

      FinalizeSqlStatement(*stmt);
    });

    return result;
}

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
}