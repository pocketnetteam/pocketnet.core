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
                INNER JOIN Payload pl ON pl.TxHash = c.PostTxHash
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
            INNER JOIN Payload pl ON pl.TxHash = t.PostTxHash
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
}