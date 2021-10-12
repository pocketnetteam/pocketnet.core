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

            // Delete exists mappings ContentId <-> TagId
            i = 1;
            auto idsStmt = SetupSqlStatement(R"sql(
                delete from web.TagsMap
                where ContentId in ( )sql" + join(vector<string>(ids.size(), "?"), ",") + R"sql( )
            )sql");
            for (const auto& id: ids) TryBindStatementInt64(idsStmt, i++, id);
            TryStepStatement(idsStmt);

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
            }
        });
    }

    vector<Content> WebRepository::GetContent(const string& blockHash)
    {
        vector<Content> result;

        string sql = R"sql(
            select
                t.Type,
                t.Id,
                p.String1,
                p.String2,
                p.String3,
                p.String4,
                p.String5,
                p.String6,
                p.String7
            from Transactions t indexed by Transactions_BlockHash
            join Payload p on p.TxHash = t.Hash
            where t.BlockHash = ?
              and t.Type in (100, 101, 102, 200, 201, 204, 205)
       )sql";
       
       TryTransactionStep(__func__, [&]()
       {
           auto stmt = SetupSqlStatement(sql);
           TryBindStatementText(stmt, 1, blockHash);

           while (sqlite3_step(*stmt) == SQLITE_ROW)
           {
                auto[okType, type] = TryGetColumnInt(*stmt, 0);
                auto[okId, id] = TryGetColumnInt64(*stmt, 1);
                if (!okType || !okId)
                    continue;

                switch ((TxType)type)
                {
                case ACCOUNT_USER:

                    if (auto[ok, string2] = TryGetColumnString(*stmt, 3); ok)
                        result.emplace_back(Content(id, ContentFieldType_AccountUserName, string2));

                    if (auto[ok, string4] = TryGetColumnString(*stmt, 5); ok)    
                        result.emplace_back(Content(id, ContentFieldType_AccountUserAbout, string4));

                    if (auto[ok, string5] = TryGetColumnString(*stmt, 6); ok)
                        result.emplace_back(Content(id, ContentFieldType_AccountUserUrl, string5));

                    break;
                case CONTENT_POST:

                    if (auto[ok, string2] = TryGetColumnString(*stmt, 3); ok)
                        result.emplace_back(Content(id, ContentFieldType_ContentPostCaption, string2));
                    
                    if (auto[ok, string3] = TryGetColumnString(*stmt, 4); ok)
                        result.emplace_back(Content(id, ContentFieldType_ContentPostMessage, string3));

                    if (auto[ok, string7] = TryGetColumnString(*stmt, 8); ok)
                        result.emplace_back(Content(id, ContentFieldType_ContentPostUrl, string7));

                    break;
                case CONTENT_VIDEO:

                    if (auto[ok, string2] = TryGetColumnString(*stmt, 3); ok)
                        result.emplace_back(Content(id, ContentFieldType_ContentVideoCaption, string2));

                    if (auto[ok, string3] = TryGetColumnString(*stmt, 4); ok)
                        result.emplace_back(Content(id, ContentFieldType_ContentVideoMessage, string3));

                    if (auto[ok, string7] = TryGetColumnString(*stmt, 8); ok)
                        result.emplace_back(Content(id, ContentFieldType_ContentVideoUrl, string7));

                    break;
                case CONTENT_COMMENT:
                case CONTENT_COMMENT_EDIT:

                    if (auto[ok, string1] = TryGetColumnString(*stmt, 2); ok)
                        result.emplace_back(Content(id, ContentFieldType_CommentMessage, string1));

                    break;
                default:
                    break;
                }
           }

           FinalizeSqlStatement(*stmt);
       });

        return result;
    }

    void WebRepository::UpsertContent(const vector<Content>& contentList)
    {
        TryTransactionStep(__func__, [&]()
        {
            for (const auto& contentItm : contentList)
            {
                auto stmt = SetupSqlStatement(R"sql(
                    insert or ignore
                    into web.Content (ContentId, FieldType, Value)
                    values (?,?,?)
                )sql");
                TryBindStatementInt64(stmt, 1, contentItm.ContentId);
                TryBindStatementInt(stmt, 2, (int)contentItm.FieldType);
                TryBindStatementText(stmt, 3, contentItm.Value);
                TryStepStatement(stmt);
            }
        });
    }
}