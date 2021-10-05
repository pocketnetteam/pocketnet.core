// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include "pocketdb/repositories/web/WebRepository.h"

namespace PocketDb
{
    void WebRepository::Init() {}

    void WebRepository::Destroy() {}

    vector<Tag> WebRepository::GetContentTags(const string& blockHash)
    {
        vector<Tag> result;

        string sql = R"sql(
            select distinct p.Id, json_each.value
            from Transactions p indexed by Transactions_BlockHash
            join Payload pp on pp.TxHash = p.Hash
            join json_each(pp.String4)
            where p.Type in (200, 201)
              and p.Last = 1
              and p.BlockHash = ?
        )sql";

        TryTransactionStep(__func__, [&]()
        {
            auto stmt = SetupSqlStatement(sql);
            TryBindStatementText(stmt, 1, blockHash);

            while (sqlite3_step(*stmt) == SQLITE_ROW)
            {
                auto[okId, id] = TryGetColumnInt64(*stmt, 0);
                if (!okId) continue;

                auto[okValue, value] = TryGetColumnString(*stmt, 1);
                if (!okValue) continue;

                result.emplace_back(Tag(id, value));
            }

            FinalizeSqlStatement(*stmt);
        });

        return result;
    }

    void WebRepository::UpsertContentTags(const vector<Tag>& contentTags)
    {
        // build distinct lists
        vector<string> tags;
        vector<int> ids;
        for (auto& contentTag : contentTags)
        {
            if (find(tags.begin(), tags.end(), contentTag.Value) == tags.end())
                tags.emplace_back(contentTag.Value);

            if (find(ids.begin(), ids.end(), contentTag.ContentId) == ids.end())
                ids.emplace_back(contentTag.ContentId);
        }

        // Next work in transaction
        TryTransactionStep(__func__, [&]()
        {
            // Insert new tags and ignore exists
            int i = 1;
            auto tagsStmt = SetupSqlStatement(R"sql(
                insert or ignore
                into web.Tags (Value)
                values )sql" + join(vector<string>(tags.size(), "(?)"), ",") + R"sql(
            )sql");
            for (const auto& tag: tags) TryBindStatementText(tagsStmt, i++, tag);
            TryStepStatement(tagsStmt);
            FinalizeSqlStatement(*tagsStmt);

            // Delete exists mappings ContentId <-> TagId
            i = 1;
            auto idsStmt = SetupSqlStatement(R"sql(
                delete from web.TagsMap
                where ContentId in ( )sql" + join(vector<string>(ids.size(), "?"), ",") + R"sql( )
            )sql");
            for (const auto& id: ids) TryBindStatementInt64(idsStmt, i++, id);
            TryStepStatement(idsStmt);
            FinalizeSqlStatement(*idsStmt);

            // Insert new mappings ContentId <-> TagId
            for (const auto& contentTag : contentTags)
            {
                auto stmt = SetupSqlStatement(R"sql(
                    insert or ignore
                    into web.TagsMap (ContentId, TagId) values (
                        ?,
                        (select t.Id from web.Tags t where t.Value = ?)
                    )
                )sql");
                TryBindStatementInt64(stmt, 1, contentTag.ContentId);
                TryBindStatementText(stmt, 2, contentTag.Value);
                TryStepStatement(stmt);
                FinalizeSqlStatement(*stmt);
            }
        });
    }

    vector<Content> WebRepository::GetContent(const string& blockHash)
    {
        vector<Content> result;

        // TODO (brangr): implement
//        string sql = R"sql(
//            select distinct p.Id, json_each.value
//            from Transactions p indexed by Transactions_BlockHash
//            join Payload pp on pp.TxHash = p.Hash
//            join json_each(pp.String4)
//            where p.Type in (200, 201)
//              and p.Last = 1
//              and p.BlockHash = ?
//        )sql";
//
//        TryTransactionStep(__func__, [&]()
//        {
//            auto stmt = SetupSqlStatement(sql);
//            TryBindStatementText(stmt, 1, blockHash);
//
//            while (sqlite3_step(*stmt) == SQLITE_ROW)
//            {
//                auto[okId, id] = TryGetColumnInt64(*stmt, 0);
//                if (!okId) continue;
//
//                auto[okValue, value] = TryGetColumnString(*stmt, 1);
//                if (!okValue) continue;
//
//                result.emplace_back(Content(id, value));
//            }
//
//            FinalizeSqlStatement(*stmt);
//        });

        return result;
    }

    void WebRepository::UpsertContent(const vector<Content>& contents)
    {

    }

//    void WebRepository::InsertTags(int height)
//    {
//        string deleteSql = R"sql(
//            delete from web.Tags indexed by Tags_Id
//            where web.Tags.ContentId in (
//                select p.ContentId
//                from Transactions p
//                where p.Height >= ?
//            )
//        )sql";
//
//        string insertSql = R"sql(
//            insert into web.Tags (Type, ContentId, Value)
//            select distinct p.Type, p.ContentId, json_each.value
//            from Transactions p indexed by Transactions_Type_Last_Height_String3
//            join Payload pp  on pp.TxHash = p.Hash
//            join json_each(pp.String4)
//            where p.Type in (200, 201)
//              and p.Last = 1
//              and p.Height >= ?
//        )sql";
//
//        TryTransactionStep(__func__, [&]()
//        {
//            auto deleteStmt = SetupSqlStatement(deleteSql);
//            TryBindStatementInt(deleteStmt, 1, height);
//    TryStepStatement(stmt);
//            FinalizeSqlStatement(*deleteStmt);
//
//            auto insertStmt = SetupSqlStatement(insertSql);
//            TryBindStatementInt(insertStmt, 1, height);
//    TryStepStatement(stmt);
//            FinalizeSqlStatement(*insertStmt);
//        });
//    }
//
//    void WebRepository::ProcessSearchContent(int height)
//    {
//
//    }
}