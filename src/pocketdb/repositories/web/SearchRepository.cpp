// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include "pocketdb/repositories/web/SearchRepository.h"

namespace PocketDb
{
    void SearchRepository::Init() {}

    void SearchRepository::Destroy() {}

    UniValue SearchRepository::SearchTags(const SearchRequest& request)
    {
        UniValue result(UniValue::VARR);

        string sql = R"sql(
            select Value
            from Tags t indexed by Tags_Value
            where t.Value like '%?%'
            limit ?
            offset ?
        )sql";

        TryTransactionStep(__func__, [&]()
        {
            auto stmt = SetupSqlStatement(sql);
            TryBindStatementText(stmt, 1, request.Keyword);
            TryBindStatementInt(stmt, 2, request.PageSize);
            TryBindStatementInt(stmt, 3, request.PageStart);

            while (sqlite3_step(*stmt) == SQLITE_ROW)
            {
                if (auto[ok, value] = TryGetColumnString(*stmt, 0); ok)
                    result.push_back(value);
            }

            FinalizeSqlStatement(*stmt);
        });

        return result;
    }

    UniValue SearchRepository::SearchPosts(const SearchRequest& searchRequest)
    {
        UniValue result(UniValue::VARR);

        string sql = R"sql(
            
        )sql";

        TryTransactionStep(__func__, [&]()
        {
            auto stmt = SetupSqlStatement(sql);
            TryBindStatementText(stmt, 1, request.Keyword);
            TryBindStatementInt(stmt, 2, request.PageSize);
            TryBindStatementInt(stmt, 3, request.PageStart);

            while (sqlite3_step(*stmt) == SQLITE_ROW)
            {
                if (auto[ok, value] = TryGetColumnString(*stmt, 0); ok)
                    result.push_back(value);
            }

            FinalizeSqlStatement(*stmt);
        });

        return result;

        {
            //LogPrintf("--- Search: %s\n", fulltext_search_string);
            reindexer::QueryResults resPostsBySearchString;
            if (g_pocketdb->Select(
                    reindexer::Query("Posts", resultStart, resulCount)
                    .Where("block", blockNumber ? CondLe : CondGe, blockNumber)
                    .Where(search_string.at(0) == '#' ? "tags" : "caption+message", CondEq, search_string.at(0) == '#' ? search_string.substr(1) : "\"" + search_string + "\"")
                    .Where("address", address == "" ? CondGt : CondEq, address)
                    .Not().Where("type", CondEq, (int)ContentType::ContentDelete)
                    .Sort("time", true)
                    .ReqTotal(),
                    resPostsBySearchString)
                    .ok()) {
                UniValue aPosts(UniValue::VARR);

                for (auto& it : resPostsBySearchString) {
                    Item _itm = it.GetItem();
                    string _txid = _itm["txid"].As<string>();
                    string _caption = _itm["caption_"].As<string>();
                    string _message = _itm["message_"].As<string>();

                    if (fs) getFastSearchString(search_string, _caption, mFastSearch);
                    if (fs) getFastSearchString(search_string, _message, mFastSearch);

                    if (all || type == "posts") aPosts.push_back(getPostData(_itm, ""));
                }

                if (all || type == "posts") {
                    UniValue oPosts(UniValue::VOBJ);
                    oPosts.pushKV("count", resPostsBySearchString.totalCount);
                    oPosts.pushKV("data", aPosts);
                    result.pushKV("posts", oPosts);
                }
            }
        }
    }

    UniValue SearchRepository::SearchVideoLink(const SearchRequest& searchRequest)
    {
        {
            reindexer::QueryResults resVideoLinksBySearchString;
            if (g_pocketdb->Select(
                    reindexer::Query("Posts", resultStart, resulCount)
                    .Where("block", blockNumber ? CondLe : CondGe, blockNumber)
                    .Where("type", CondEq, (int)ContentType::ContentVideo)
                    .Where("url", CondSet, search_vector)
                    .Where("address", address == "" ? CondGt : CondEq, address)
                    .Sort("time", true)
                    .ReqTotal(),
                    resVideoLinksBySearchString)
                    .ok()) {
                UniValue aPosts(UniValue::VARR);

                for (auto& it : resVideoLinksBySearchString) {
                    Item _itm = it.GetItem();
                    string _txid = _itm["txid"].As<string>();

                    aPosts.push_back(getPostData(_itm, ""));
                }

                UniValue oPosts(UniValue::VOBJ);
                oPosts.pushKV("count", resVideoLinksBySearchString.totalCount);
                oPosts.pushKV("data", aPosts);
                result.pushKV("videos", oPosts);
            }
        }
    }

    UniValue SearchRepository::SearchAccounts(const SearchRequest& searchRequest)
    {
        {
            reindexer::QueryResults resUsersBySearchString;
            if (g_pocketdb->Select(
                    reindexer::Query("UsersView", resultStart, resulCount)
                    .Where("block", blockNumber ? CondLe : CondGe, blockNumber)
                    .Where("name_text", CondEq, "*" + UrlEncode(search_string) + "*")
                    //.Sort("time", false)  // Do not sort or think about full-text match first
                    .ReqTotal(),
                    resUsersBySearchString)
                    .ok()) {
                vector<string> vUserAdresses;

                for (auto& it : resUsersBySearchString) {
                    Item _itm = it.GetItem();
                    string _address = _itm["address"].As<string>();
                    vUserAdresses.push_back(_address);
                }

                auto mUsers = getUsersProfiles(vUserAdresses, true, 1);

                UniValue aUsers(UniValue::VARR);
                for (const auto& item : vUserAdresses) {
                    aUsers.push_back(mUsers[item]);
                }
                //for (auto& u : mUsers) {
                //    aUsers.push_back(u.second);
                //}

                UniValue oUsers(UniValue::VOBJ);
                oUsers.pushKV("count", resUsersBySearchString.totalCount);
                oUsers.pushKV("data", aUsers);

                result.pushKV("users", oUsers);
            }

            fsresultCount = resUsersBySearchString.Count() < fsresultCount ? fsresultCount - resUsersBySearchString.Count() : 0;
        }
    }
}