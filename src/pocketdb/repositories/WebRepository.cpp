// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 Bitcoin developers
// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include "WebRepository.h"

namespace PocketDb
{
    void WebRepository::Init() {}
    void WebRepository::Destroy() {}

    // Top addresses info
    UniValue WebRepository::GetAddressInfo(int count)
    {
        // shared_ptr<ListDto<AddressInfoDto>> result = make_shared<ListDto<AddressInfoDto>>();

        // return make_tuple(
        //     TryTransactionStep([&]()
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

    UniValue WebRepository::GetLastComments(int count, int height, std::string lang)
    {
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

        TryTransactionStep([&]()
        {
            auto stmt = SetupSqlStatement(sql);

            TryBindStatementInt(stmt, 1, height);
            TryBindStatementInt64(stmt, 2, GetAdjustedTime());
            TryBindStatementText(stmt, 3, make_shared<std::string>(lang.empty() ? "en" : lang));
            TryBindStatementInt(stmt, 4, count);

            while (sqlite3_step(*stmt) == SQLITE_ROW)
            {
                UniValue record(UniValue::VOBJ);

                auto txHash = GetColumnString(*stmt, 0);
                auto rootTxHash = GetColumnString(*stmt, 1);

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

                //TODO (joni) (brangr): тут может быть разрулим типом транзакции
                record.pushKV("edit", txHash != rootTxHash);

                result.push_back(record);
            }

            FinalizeSqlStatement(*stmt);
        });

        return result;
    }

    UniValue WebRepository::GetCommentsByPost(const std::string& postHash, const std::string& parentHash, const std::string& addressHash)
    {
        //TODO (joni): return Last =
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
               (SELECT COUNT(1) FROM vComments s WHERE s.ParentTxHash = c.RootTxHash) AS ChildrenCount --AND s.Last = 1
            FROM vComments c
            INNER JOIN vComments r ON c.RootTxHash = r.Hash
            INNER JOIN Payload pl ON pl.TxHash = c.Hash
            LEFT JOIN vScoreComments sc ON sc.CommentTxHash = C.RootTxHash AND IFNULL(sc.AddressHash, '') = ?
            WHERE c.PostTxHash = ?
                AND IFNULL(c.ParentTxHash, '') = ?
                --AND c.Last = 1
                AND c.Time < ?
        )sql";

        auto result = UniValue(UniValue::VARR);

        TryTransactionStep([&]() {
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

    UniValue WebRepository::GetCommentsByIds(string& addressHash, vector<string>& commentHashes)
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

        //TODO (joni): return Last =
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
               (SELECT COUNT(1) FROM vComments s WHERE s.ParentTxHash = c.RootTxHash) AS ChildrenCount --AND s.Last = 1
            FROM vComments c
            INNER JOIN vComments r ON c.RootTxHash = r.Hash
            INNER JOIN Payload pl ON pl.TxHash = c.Hash
            LEFT JOIN vScoreComments sc ON sc.CommentTxHash = C.RootTxHash AND IFNULL(sc.AddressHash, '') = ?
            WHERE c.Time < ?
                --AND c.Last = 1
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

        TryTransactionStep([&]() {
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

    UniValue WebRepository::ParseCommentRow(sqlite3_stmt* stmt)
    {
        UniValue record(UniValue::VOBJ);

        auto txHash = GetColumnString(stmt, 1);
        auto rootTxHash = GetColumnString(stmt, 2);

        record.pushKV("id", rootTxHash);
        if (auto [ok, valueStr] = TryGetColumnString(stmt, 3); ok) record.pushKV("postid", valueStr);

        if (auto [ok, valueStr] = TryGetColumnString(stmt, 8); ok)
        {
            record.pushKV("msg", valueStr);
        }
        else
        {
            record.pushKV("msg", "");
        }

        if (auto [ok, valueStr] = TryGetColumnString(stmt, 4); ok) record.pushKV("address", valueStr);
        if (auto [ok, valueStr] = TryGetColumnString(stmt, 5); ok) record.pushKV("time", valueStr);
        if (auto [ok, valueStr] = TryGetColumnString(stmt, 6); ok) record.pushKV("timeUpd", valueStr);
        if (auto [ok, valueStr] = TryGetColumnString(stmt, 7); ok) record.pushKV("block", valueStr);
        if (auto [ok, valueStr] = TryGetColumnString(stmt, 9); ok) record.pushKV("parentid", valueStr);
        if (auto [ok, valueStr] = TryGetColumnString(stmt, 10); ok) record.pushKV("answerid", valueStr);
        if (auto [ok, valueStr] = TryGetColumnString(stmt, 11); ok) record.pushKV("scoreUp", valueStr);
        if (auto [ok, valueStr] = TryGetColumnString(stmt, 12); ok) record.pushKV("scoreDown", valueStr);
        if (auto [ok, valueStr] = TryGetColumnString(stmt, 13); ok) record.pushKV("reputation", valueStr);
        if (auto [ok, valueStr] = TryGetColumnString(stmt, 14); ok) record.pushKV("myScore", valueStr);
        if (auto [ok, valueStr] = TryGetColumnString(stmt, 15); ok) record.pushKV("children", valueStr);

        switch (static_cast<PocketTxType>(GetColumnInt(stmt, 0)))
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
        }

        return record;
    }
}