// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include "pocketdb/repositories/web/WebRepository.h"

namespace PocketDb
{
    void WebRepository::Init() {}

    void WebRepository::Destroy() {}

    vector<WebTag> WebRepository::GetContentTags(const string& blockHash)
    {
        vector<WebTag> result;

        string sql = R"sql(
            select distinct p.Id, pp.String1, json_each.value
            from Transactions p indexed by Transactions_BlockHash
            join Payload pp on pp.TxHash = p.Hash
            join json_each(pp.String4)
            where p.Type in (200, 201, 202)
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

                auto[okLang, lang] = TryGetColumnString(*stmt, 1);
                if (!okLang) continue;

                auto[okValue, value] = TryGetColumnString(*stmt, 2);
                if (!okValue) continue;

                result.emplace_back(WebTag(id, lang, value));
            }

            FinalizeSqlStatement(*stmt);
        });

        return result;
    }

    void WebRepository::UpsertContentTags(const vector<WebTag>& contentTags)
    {
        // build distinct lists
        vector<int> ids;
        for (auto& contentTag : contentTags)
        {
            if (find(ids.begin(), ids.end(), contentTag.ContentId) == ids.end())
                ids.emplace_back(contentTag.ContentId);
        }

        // Next work in transaction
        TryTransactionStep(__func__, [&]()
        {
            // Insert new tags and ignore exists with unique index Lang+Value
            int i = 1;
            auto tagsStmt = SetupSqlStatement(R"sql(
                insert or ignore
                into web.Tags (Lang, Value)
                values )sql" + join(vector<string>(contentTags.size(), "(?,?)"), ",") + R"sql(
            )sql");
            for (const auto& tag: contentTags)
            {
                TryBindStatementText(tagsStmt, i++, tag.Lang);
                TryBindStatementText(tagsStmt, i++, tag.Value);
            }
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
                        (select t.Id from web.Tags t where t.Value = ? and t.Lang = ?)
                    )
                )sql");
                TryBindStatementInt64(stmt, 1, contentTag.ContentId);
                TryBindStatementText(stmt, 2, contentTag.Value);
                TryBindStatementText(stmt, 3, contentTag.Lang);
                TryStepStatement(stmt);
            }
        });
    }

    vector<WebContent> WebRepository::GetContent(const string& blockHash)
    {
        vector<WebContent> result;

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
              and t.Type in (100, 200, 201, 202, 204, 205)
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
                        result.emplace_back(WebContent(id, ContentFieldType_AccountUserName, string2));

                    if (auto[ok, string4] = TryGetColumnString(*stmt, 5); ok)    
                        result.emplace_back(WebContent(id, ContentFieldType_AccountUserAbout, string4));

                    // if (auto[ok, string5] = TryGetColumnString(*stmt, 6); ok)
                    //     result.emplace_back(WebContent(id, ContentFieldType_AccountUserUrl, string5));

                    break;
                case CONTENT_POST:

                    if (auto[ok, string2] = TryGetColumnString(*stmt, 3); ok)
                        result.emplace_back(WebContent(id, ContentFieldType_ContentPostCaption, string2));
                    
                    if (auto[ok, string3] = TryGetColumnString(*stmt, 4); ok)
                        result.emplace_back(WebContent(id, ContentFieldType_ContentPostMessage, string3));

                    // if (auto[ok, string7] = TryGetColumnString(*stmt, 8); ok)
                    //     result.emplace_back(WebContent(id, ContentFieldType_ContentPostUrl, string7));

                    break;
                case CONTENT_VIDEO:

                    if (auto[ok, string2] = TryGetColumnString(*stmt, 3); ok)
                        result.emplace_back(WebContent(id, ContentFieldType_ContentVideoCaption, string2));

                    if (auto[ok, string3] = TryGetColumnString(*stmt, 4); ok)
                        result.emplace_back(WebContent(id, ContentFieldType_ContentVideoMessage, string3));

                    // if (auto[ok, string7] = TryGetColumnString(*stmt, 8); ok)
                    //     result.emplace_back(WebContent(id, ContentFieldType_ContentVideoUrl, string7));

                    break;
                
                // TODO (brangr): parse JSON for indexing
                // case CONTENT_ARTICLE:

                // case CONTENT_COMMENT:
                // case CONTENT_COMMENT_EDIT:

                    // TODO (brangr): implement extract message from JSON
                    // if (auto[ok, string1] = TryGetColumnString(*stmt, 2); ok)
                    //     result.emplace_back(WebContent(id, ContentFieldType_CommentMessage, string1));

                    // break;
                default:
                    break;
                }
           }

           FinalizeSqlStatement(*stmt);
       });

        return result;
    }

    void WebRepository::UpsertContent(const vector<WebContent>& contentList)
    {
        auto func = __func__;

        vector<int64_t> ids;
        for (auto& contentItm : contentList)
        {
            if (find(ids.begin(), ids.end(), contentItm.ContentId) == ids.end())
                ids.emplace_back(contentItm.ContentId);
        }

        // ---------------------------------------------------------

        TryTransactionStep(__func__, [&]()
        {
            // ---------------------------------------------------------
            int64_t nTime1 = GetTimeMicros();

            auto delContentStmt = SetupSqlStatement(R"sql(
                delete from web.Content
                where ROWID in (
                    select cm.ROWID from ContentMap cm where cm.ContentId in (
                        )sql" + join(vector<string>(ids.size(), "?"), ",") + R"sql(
                    )
                )
            )sql");

            int i = 1;
            for (const auto& id: ids) TryBindStatementInt64(delContentStmt, i++, id);
            TryStepStatement(delContentStmt);

            // ---------------------------------------------------------
            int64_t nTime2 = GetTimeMicros();

            auto delContentMapStmt = SetupSqlStatement(R"sql(
                delete from web.ContentMap
                where ContentId in (
                    )sql" + join(vector<string>(ids.size(), "?"), ",") + R"sql(
                )
            )sql");

            i = 1;
            for (const auto& id: ids) TryBindStatementInt64(delContentMapStmt, i++, id);
            TryStepStatement(delContentMapStmt);

            // ---------------------------------------------------------
            int64_t nTime3 = GetTimeMicros();

            for (const auto& contentItm : contentList)
            {
                SetLastInsertRowId(0);

                auto stmtMap = SetupSqlStatement(R"sql(
                    insert or ignore into ContentMap (ContentId, FieldType) values (?,?)
                )sql");
                TryBindStatementInt64(stmtMap, 1, contentItm.ContentId);
                TryBindStatementInt(stmtMap, 2, (int)contentItm.FieldType);
                TryStepStatement(stmtMap);

                // ---------------------------------------------------------

                auto lastRowId = GetLastInsertRowId();
                if (lastRowId > 0)
                {
                    auto stmtContent = SetupSqlStatement(R"sql(
                        replace into web.Content (ROWID, Value) values (?,?)
                    )sql");
                    TryBindStatementInt64(stmtContent, 1, lastRowId);
                    TryBindStatementText(stmtContent, 2, contentItm.Value);
                    TryStepStatement(stmtContent);
                }
                else
                {
                    LogPrintf("Warning: content (%d) field (%d) not indexed in search db\n",
                        contentItm.ContentId, (int)contentItm.FieldType);
                }
            }

            // ---------------------------------------------------------
            int64_t nTime4 = GetTimeMicros();

            LogPrint(BCLog::BENCH, "        - TryTransactionStep (%s): %.2fms + %.2fms + %.2fms = %.2fms\n",
                func,
                0.001 * (nTime2 - nTime1),
                0.001 * (nTime3 - nTime2),
                0.001 * (nTime4 - nTime3),
                0.001 * (nTime4 - nTime1)
            );
        });
    }

    void WebRepository::CalculateSharkAccounts(BadgeSharkConditions& cond)
    {
        TryTransactionStep(__func__, [&]()
        {
            // Clear badges table before insert new values
            auto stmtClear = SetupSqlStatement(R"sql(
                delete from web.Badges
            )sql");
            TryStepStatement(stmtClear);

            // Clear old Last record
            auto stmtInsert = SetupSqlStatement(R"sql(
                insert into web.Badges (AccountId, Badge)
                select u.Id, 1
                from Transactions u indexed by Transactions_Type_Last_Height_Id
                where u.Type in (100)
                  and u.Last = 1
                  and u.Height is not null
                  and ifnull((select sum(r.Value) from Ratings r where r.Type in (111,112,113) and r.Last = 1 and r.Id = u.Id),0) >= ?
                  and ifnull((select r.Value from Ratings r where r.Type = 111 and r.Last = 1 and r.Id = u.Id),0) >= ?
                  and ifnull((select r.Value from Ratings r where r.Type = 112 and r.Last = 1 and r.Id = u.Id),0) >= ?
                  and ifnull((select r.Value from Ratings r where r.Type = 113 and r.Last = 1 and r.Id = u.Id),0) >= ?
                  and ? - (select min(reg1.Height) from Transactions reg1 indexed by Transactions_Id where reg1.Id = u.Id) > ?
            )sql");
            int i = 1;
            TryBindStatementInt(stmtInsert, i++, cond.LikersAll);
            TryBindStatementInt64(stmtInsert, i++, cond.LikersContent);
            TryBindStatementInt64(stmtInsert, i++, cond.LikersComment);
            TryBindStatementInt64(stmtInsert, i++, cond.LikersAnswer);
            TryBindStatementInt64(stmtInsert, i++, cond.Height);
            TryBindStatementInt64(stmtInsert, i++, cond.RegistrationDepth);
            TryStepStatement(stmtInsert);
        });
    }

    void WebRepository::CalculateValidAuthors(int blockHeight)
    {
        TryTransactionStep(__func__, [&]()
        {
            // Clear badges table before insert new values
            auto stmtClear = SetupSqlStatement(R"sql(
                delete from web.Authors
            )sql");
            TryStepStatement(stmtClear);

            // Clear old Last record
            auto stmtInsert = SetupSqlStatement(R"sql(
                insert into web.Authors (AccountId, SharkCommented)
          
                select
          
                  pu.Id
                  ,count( distinct u.Id )
          
                from Transactions p indexed by Transactions_Type_Last_String1_Height_Id
          
                cross join Transactions c indexed by Transactions_Type_Last_String3_Height
                  on  c.Type in (204, 205)
                  and c.Last = 1
                  and c.String3 = p.String2
                  and c.Height is not null
          
                cross join Transactions u indexed by Transactions_Type_Last_String1_Height_Id
                  on u.Type in (100)
                  and u.Last = 1
                  and u.String1 = c.String1
                  and u.Height is not null
          
                cross join web.Badges b on b.AccountId = u.Id and b.Badge = 1
          
                cross join Transactions pu indexed by Transactions_Type_Last_String1_Height_Id
                  on  pu.Type in (100)
                  and pu.Last = 1
                  and pu.String1 = p.String1
                  and pu.Height is not null
          
                where p.Type in (200,201,202)
                  and p.Last = 1
                  and p.Height is not null
          
                group by pu.Id
            )sql");
            TryStepStatement(stmtInsert);
        });
    }
}