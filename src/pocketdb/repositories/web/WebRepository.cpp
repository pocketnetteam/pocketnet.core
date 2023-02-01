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
            select distinct
                c.Uid,
                pp.String1,
                json_each.value

            from Transactions p

            join Payload pp on
                pp.TxId = p.RowId

            join json_each(pp.String4)

            join Chain c on
                c.TxId = p.RowId

            join Last l on
                l.TxId = p.RowId

            join Registry r on
                r.RowId = c.BlockId and
                r.String = ?

            where
                p.Type in (200, 201, 202, 209, 210)
        )sql";

        SqlTransaction(__func__, [&]()
        {
            Sql(sql)
            .Bind(blockHash)
            .Select([&](Cursor& cursor) {
                while (cursor.Step())
                {
                    auto[okId, id] = cursor.TryGetColumnInt64(0);
                    if (!okId) continue;

                    auto[okLang, lang] = cursor.TryGetColumnString(1);
                    if (!okLang) continue;

                    auto[okValue, value] = cursor.TryGetColumnString(2);
                    if (!okValue) continue;

                    result.emplace_back(WebTag(id, lang, value));
                }
            });
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
            stmt.Run();

            // Delete exists mappings ContentId <-> TagId
            Sql(R"sql(
                delete from web.TagsMap
                where ContentId in ( )sql" + join(vector<string>(ids.size(), "?"), ",") + R"sql( )
            )sql").Bind(ids).Run();

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
                .Run();
            }
        });
    }

    vector<WebContent> WebRepository::GetContent(const string& blockHash)
    {
        vector<WebContent> result;

        string sql = R"sql(
            select
                t.Type,
                c.Uid,
                p.String1,
                p.String2,
                p.String3,
                p.String4,
                p.String5,
                p.String6,
                p.String7

            from Transactions t

            join Payload p on
                p.TxId = t.RowId

            join Chain c on
                c.TxId = p.RowId

            join Registry r on
                r.RowId = c.BlockId and
                r.String = ?
            where
                t.Type in (100, 200, 201, 202, 209, 210, 204, 205)
       )sql";
       
       SqlTransaction(__func__, [&]()
       {
           Sql(sql)
           .Bind(blockHash)
           .Select([&](Cursor& cursor) {
                while (cursor.Step())
                {
                    auto[okType, type] = cursor.TryGetColumnInt(0);
                    auto[okId, id] = cursor.TryGetColumnInt64(1);
                    if (!okType || !okId)
                        continue;

                    switch ((TxType)type)
                    {
                    case ACCOUNT_USER:

                        if (auto[ok, string2] = cursor.TryGetColumnString(3); ok)
                            result.emplace_back(WebContent(id, ContentFieldType_AccountUserName, string2));

                        if (auto[ok, string4] = cursor.TryGetColumnString(5); ok)    
                            result.emplace_back(WebContent(id, ContentFieldType_AccountUserAbout, string4));

                        // if (auto[ok, string5] = cursor.TryGetColumnString(6); ok)
                        //     result.emplace_back(WebContent(id, ContentFieldType_AccountUserUrl, string5));

                        break;
                    case CONTENT_POST:

                        if (auto[ok, string2] = cursor.TryGetColumnString(3); ok)
                            result.emplace_back(WebContent(id, ContentFieldType_ContentPostCaption, string2));
                        
                        if (auto[ok, string3] = cursor.TryGetColumnString(4); ok)
                            result.emplace_back(WebContent(id, ContentFieldType_ContentPostMessage, string3));

                        // if (auto[ok, string7] = cursor.TryGetColumnString(8); ok)
                        //     result.emplace_back(WebContent(id, ContentFieldType_ContentPostUrl, string7));

                        break;
                    case CONTENT_VIDEO:

                        if (auto[ok, string2] = cursor.TryGetColumnString(3); ok)
                            result.emplace_back(WebContent(id, ContentFieldType_ContentVideoCaption, string2));

                        if (auto[ok, string3] = cursor.TryGetColumnString(4); ok)
                            result.emplace_back(WebContent(id, ContentFieldType_ContentVideoMessage, string3));

                        // if (auto[ok, string7] = cursor.TryGetColumnString(8); ok)
                        //     result.emplace_back(WebContent(id, ContentFieldType_ContentVideoUrl, string7));

                        break;
                    
                    // TODO (aok): parse JSON for indexing
                    // case CONTENT_ARTICLE:

                    // case CONTENT_COMMENT:
                    // case CONTENT_COMMENT_EDIT:

                        // TODO (aok): implement extract message from JSON
                        // if (auto[ok, string1] = cursor.TryGetColumnString(2); ok)
                        //     result.emplace_back(WebContent(id, ContentFieldType_CommentMessage, string1));

                        // break;
                    default:
                        break;
                    }
                }
           });
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
            .Run();

            // ---------------------------------------------------------
            int64_t nTime2 = GetTimeMicros();

            Sql(R"sql(
                delete from web.ContentMap
                where ContentId in (
                    )sql" + join(vector<string>(ids.size(), "?"), ",") + R"sql(
                )
            )sql")
            .Bind(ids)
            .Run();

            // ---------------------------------------------------------
            int64_t nTime3 = GetTimeMicros();

            for (const auto& contentItm : contentList)
            {
                SetLastInsertRowId(0);

                Sql(R"sql(
                    insert or ignore into ContentMap (ContentId, FieldType) values (?,?)
                )sql")
                .Bind(contentItm.ContentId, (int)contentItm.FieldType)
                .Run();

                // ---------------------------------------------------------

                auto lastRowId = GetLastInsertRowId();
                if (lastRowId > 0)
                {
                    Sql(R"sql(
                        replace into web.Content (ROWID, Value) values (?,?)
                    )sql")
                    .Bind(lastRowId, contentItm.Value)
                    .Run();
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
            )sql").Run();

            // Clear old Last record
            Sql(R"sql(
                insert into web.Badges (AccountId, Badge)

                select
                    cu.Uid, 1

                from Transactions u indexed by Transactions_Type_Last_Height_Id

                join Chain cu on
                    cu.TxId = u.RowId

                join Last l on
                    l.TxId = u.RowId

                where
                    u.Type in (100) and
                    ifnull((select sum(r.Value) from Ratings r where r.Type in (111,112,113) and r.Last = 1 and r.Uid = cu.Uid),0) >= ? and
                    ifnull((select r.Value from Ratings r where r.Type = 111 and r.Last = 1 and r.Uid = cu.Uid),0) >= ? and
                    ifnull((select r.Value from Ratings r where r.Type = 112 and r.Last = 1 and r.Uid = cu.Uid),0) >= ? and
                    ifnull((select r.Value from Ratings r where r.Type = 113 and r.Last = 1 and r.Uid = cu.Uid),0) >= ? and
                    ? - (select min(reg1.Height) from Chain reg1 where reg1.Uid = cu.Uid) > ?
            )sql")
            .Bind(cond.LikersAll, cond.LikersContent, cond.LikersComment, cond.LikersAnswer, cond.Height, cond.RegistrationDepth)
            .Run();
        });
    }

    void WebRepository::CalculateValidAuthors(int blockHeight)
    {
        SqlTransaction(__func__, [&]()
        {
            // Clear badges table before insert new values
            Sql(R"sql(
                delete from web.Authors
            )sql").Run();

            // Clear old Last record
            Sql(R"sql(
                insert into web.Authors (AccountId, SharkCommented)

                select

                  cpu.Uid,
                  count( distinct cu.Uid )

                from Transactions p

                join Last l on
                    l.TxId = p.RowId

                cross join Transactions c on
                    c.Type in (204, 205) and
                    c.RegId3 = p.RegId2 and
                    exists (select 1 from Last lc where lc.TxId = c.RowId)

                cross join Transactions u on
                    u.Type in (100) and
                    u.RegId1 = c.RegId1 and
                    exists (select 1 from Last lu where lu.TxId = u.RowId)

                join Chain cu on
                    cu.TxId = u.RowId

                cross join web.Badges b on b.AccountId = cu.Uid and b.Badge = 1

                cross join Transactions pu on
                    pu.Type in (100) and
                    pu.RegId1 = p.RegId1 and
                    exists (select 1 from Last lpu where lpu.TxId = pu.RowId)

                join Chain cpu on
                    cpu.TxId = pu.RowId

                where
                    p.Type in (200,201,202,209,210)

                group by
                    cpu.Uid
            )sql").Run();
        });
    }
}