// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include "pocketdb/repositories/web/WebRepository.h"

namespace PocketDb
{
    vector<WebTag> WebRepository::GetContentTags(const string& blockHash)
    {
        vector<WebTag> result;

        string sql = R"sql(
            select distinct p.Id, pp.String1, json_each.value
            from Transactions p indexed by Transactions_BlockHash
            join Payload pp on pp.TxHash = p.Hash
            join json_each(pp.String4)
            where p.Type in (200, 201, 202, 209, 210)
              and p.Last = 1
              and p.BlockHash = ?
        )sql";

        SqlTransaction(__func__, [&]()
        {
            auto& stmt = Sql(sql);
            stmt.Bind(blockHash);

            while (stmt.Step() == SQLITE_ROW)
            {
                auto[okId, id] = stmt.TryGetColumnInt64(0);
                if (!okId) continue;

                auto[okLang, lang] = stmt.TryGetColumnString(1);
                if (!okLang) continue;

                auto[okValue, value] = stmt.TryGetColumnString(2);
                if (!okValue) continue;

                result.emplace_back(WebTag(id, lang, value));
            }
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
        SqlTransaction(__func__, [&]()
        {
            // Insert new tags and ignore exists with unique index Lang+Value
            auto& stmt = Sql(R"sql(
                insert or ignore
                into web.Tags (Lang, Value)
                values )sql" + join(vector<string>(contentTags.size(), "(?,?)"), ",") + R"sql(
            )sql");
            for (const auto& tag: contentTags)
                stmt.Bind(tag.Lang, tag.Value);
            stmt.Step();

            // Delete exists mappings ContentId <-> TagId
            Sql(R"sql(
                delete from web.TagsMap
                where ContentId in ( )sql" + join(vector<string>(ids.size(), "?"), ",") + R"sql( )
            )sql").Bind(ids).Step();

            // Insert new mappings ContentId <-> TagId
            for (const auto& contentTag : contentTags)
            {
                Sql(R"sql(
                    insert or ignore
                    into web.TagsMap (ContentId, TagId) values (
                        ?,
                        (select t.Id from web.Tags t where t.Value = ? and t.Lang = ?)
                    )
                )sql")
                .Bind(contentTag.ContentId, contentTag.Value, contentTag.Lang)
                .Step();
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
              and t.Type in (100, 200, 201, 202, 209, 210, 204, 205)
       )sql";
       
       SqlTransaction(__func__, [&]()
       {
           auto& stmt = Sql(sql).Bind(blockHash);

           while (stmt.Step() == SQLITE_ROW)
           {
                auto[okType, type] = stmt.TryGetColumnInt(0);
                auto[okId, id] = stmt.TryGetColumnInt64(1);
                if (!okType || !okId)
                    continue;

                switch ((TxType)type)
                {
                case ACCOUNT_USER:

                    if (auto[ok, string2] = stmt.TryGetColumnString(3); ok)
                        result.emplace_back(WebContent(id, ContentFieldType_AccountUserName, string2));

                    if (auto[ok, string4] = stmt.TryGetColumnString(5); ok)    
                        result.emplace_back(WebContent(id, ContentFieldType_AccountUserAbout, string4));

                    // if (auto[ok, string5] = stmt.TryGetColumnString(6); ok)
                    //     result.emplace_back(WebContent(id, ContentFieldType_AccountUserUrl, string5));

                    break;
                case CONTENT_POST:

                    if (auto[ok, string2] = stmt.TryGetColumnString(3); ok)
                        result.emplace_back(WebContent(id, ContentFieldType_ContentPostCaption, string2));
                    
                    if (auto[ok, string3] = stmt.TryGetColumnString(4); ok)
                        result.emplace_back(WebContent(id, ContentFieldType_ContentPostMessage, string3));

                    // if (auto[ok, string7] = stmt.TryGetColumnString(8); ok)
                    //     result.emplace_back(WebContent(id, ContentFieldType_ContentPostUrl, string7));

                    break;
                case CONTENT_VIDEO:

                    if (auto[ok, string2] = stmt.TryGetColumnString(3); ok)
                        result.emplace_back(WebContent(id, ContentFieldType_ContentVideoCaption, string2));

                    if (auto[ok, string3] = stmt.TryGetColumnString(4); ok)
                        result.emplace_back(WebContent(id, ContentFieldType_ContentVideoMessage, string3));

                    // if (auto[ok, string7] = stmt.TryGetColumnString(8); ok)
                    //     result.emplace_back(WebContent(id, ContentFieldType_ContentVideoUrl, string7));

                    break;
                
                // TODO (aok): parse JSON for indexing
                // case CONTENT_ARTICLE:

                // case CONTENT_COMMENT:
                // case CONTENT_COMMENT_EDIT:

                    // TODO (aok): implement extract message from JSON
                    // if (auto[ok, string1] = stmt.TryGetColumnString(2); ok)
                    //     result.emplace_back(WebContent(id, ContentFieldType_CommentMessage, string1));

                    // break;
                default:
                    break;
                }
           }
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

        SqlTransaction(__func__, [&]()
        {
            // ---------------------------------------------------------
            int64_t nTime1 = GetTimeMicros();

            Sql(R"sql(
                delete from web.Content
                where ROWID in (
                    select cm.ROWID from ContentMap cm where cm.ContentId in (
                        )sql" + join(vector<string>(ids.size(), "?"), ",") + R"sql(
                    )
                )
            )sql")
            .Bind(ids)
            .Step();

            // ---------------------------------------------------------
            int64_t nTime2 = GetTimeMicros();

            Sql(R"sql(
                delete from web.ContentMap
                where ContentId in (
                    )sql" + join(vector<string>(ids.size(), "?"), ",") + R"sql(
                )
            )sql")
            .Bind(ids)
            .Step();

            // ---------------------------------------------------------
            int64_t nTime3 = GetTimeMicros();

            for (const auto& contentItm : contentList)
            {
                SetLastInsertRowId(0);

                Sql(R"sql(
                    insert or ignore into ContentMap (ContentId, FieldType) values (?,?)
                )sql")
                .Bind(contentItm.ContentId, (int)contentItm.FieldType)
                .Step();

                // ---------------------------------------------------------

                auto lastRowId = GetLastInsertRowId();
                if (lastRowId > 0)
                {
                    Sql(R"sql(
                        replace into web.Content (ROWID, Value) values (?,?)
                    )sql")
                    .Bind(lastRowId, contentItm.Value)
                    .Step();
                }
                else
                {
                    LogPrintf("Warning: content (%d) field (%d) not indexed in search db\n",
                        contentItm.ContentId, (int)contentItm.FieldType);
                }
            }

            // ---------------------------------------------------------
            int64_t nTime4 = GetTimeMicros();

            LogPrint(BCLog::BENCH, "        - SqlTransaction (%s): %.2fms + %.2fms + %.2fms = %.2fms\n",
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
        SqlTransaction(__func__, [&]()
        {
            // Clear badges table before insert new values
            Sql(R"sql(
                delete from web.Badges
            )sql").Step();

            // Clear old Last record
            Sql(R"sql(
                insert into web.Badges (AccountId, Badge)
                select uc.Uid, 1
                from Transactions u
                join Chain uc
                    on uc.TxId = u.RowId
                    and uc.BlockId > 0 -- tx is in block (analog Height>0)
                join Last ul
                    on ul.TxId = u.RowId
                where u.Type in (100)
                  and ifnull((select sum(r.Value) from Ratings r where r.Type in (111,112,113) and r.Last = 1 and r.Uid = uc.Uid),0) >= ?
                  and ifnull((select r.Value from Ratings r where r.Type = 111 and r.Last = 1 and r.Uid = uc.Uid),0) >= ?
                  and ifnull((select r.Value from Ratings r where r.Type = 112 and r.Last = 1 and r.Uid = uc.Uid),0) >= ?
                  and ifnull((select r.Value from Ratings r where r.Type = 113 and r.Last = 1 and r.Uid = uc.Uid),0) >= ?
                  and ? - (select min(reg1.Height) from Chain reg1 where reg1.Uid = uc.Uid) > ?
            )sql")
            .Bind(cond.LikersAll, cond.LikersContent, cond.LikersComment, cond.LikersAnswer, cond.Height, cond.RegistrationDepth)
            .Step();
        });
    }

    void WebRepository::CalculateValidAuthors(int blockHeight)
    {
        SqlTransaction(__func__, [&]()
        {
            // Clear badges table before insert new values
            Sql(R"sql(
                delete from web.Authors
            )sql").Step();

            // Clear old Last record
            Sql(R"sql(
                insert into web.Authors (AccountId, SharkCommented)
          
                select
          
                  pu.Id,
                  count( distinct u.Id )
          
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
          
                where p.Type in (200,201,202,209,210)
                  and p.Last = 1
                  and p.Height is not null
          
                group by pu.Id
            )sql").Step();
        });
    }
}