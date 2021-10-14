// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include "pocketdb/web/SearchRpc.h"

namespace PocketWeb::PocketWebRpc
{
    string lower(string s)
    {
        transform(s.begin(), s.end(), s.begin(), [](char c) { return 'A' <= c && c <= 'Z' ? c ^ 32 : c; });
        return s;
    }

    UniValue Search(const JSONRPCRequest& request)
    {
        if (request.fHelp)
            throw runtime_error(
                "search \"keyword\", \"type\", startBlock, pageStart, pageSize, \"address\"\n"
                "\nSearch data in DB.\n"
                "\nArguments:\n"
                "1. \"keyword\"     (string or array) String for search or array of strings\n"
                "2. \"type\"        (string, optional) fs (default), all, posts, videolink, tags, users\n"
                "3. \"startBlock\"  (int, optional) Start block for search. Default 0\n"
                "4. \"pageStart\" (int, optional) Pagination start. Default 0\n"
                "5. \"pageSize\" (int, optional) Pagination count. Default 10\n"
                "5. \"address\"     (string, optional) Filter by address\n"
            );

        SearchRequest searchRequest;

        if (request.params.size() > 0)
        {
            if (request.params[0].isStr())
            {
                searchRequest.Keywords.emplace_back(HtmlUtils::UrlDecode(request.params[0].get_str()));
            }
            else if (request.params[0].isArray())
            {
                UniValue srchs = request.params[0].get_array();
                for (unsigned int idx = 0; idx < srchs.size(); idx++)
                {
                    searchRequest.Keywords.emplace_back(HtmlUtils::UrlDecode(srchs[idx].get_str()));
                    if (searchRequest.Keywords.size() > 100)
                        break;
                }
            }
        }

        string type = "fs";
        if (request.params.size() > 1)
        {
            RPCTypeCheckArgument(request.params[1], UniValue::VSTR);
            type = lower(request.params[1].get_str());
        }

        bool fs = (type == "fs");
        bool all = (type == "all");

        if (request.params.size() > 2)
        {
            try
            {
                ParseInt32(request.params[2].get_str(), &searchRequest.StartBlock);
            }
            catch (...) { }
        }

        if (request.params.size() > 3)
        {
            try
            {
                ParseInt32(request.params[3].get_str(), &searchRequest.PageStart);
            }
            catch (...) { }
        }

        if (request.params.size() > 4)
        {
            try
            {
                ParseInt32(request.params[4].get_str(), &searchRequest.PageSize);
            }
            catch (...) { }
        }

        if (request.params.size() > 5) {
            RPCTypeCheckArgument(request.params[5], UniValue::VSTR);
            CTxDestination dest = DecodeDestination(request.params[5].get_str());
            if (!IsValidDestination(dest)) {
                throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, string("Invalid Pocketcoin address: ") + request.params[5].get_str());
            }
            searchRequest.Address = request.params[5].get_str();
        }

        int fsresultCount = 10;

        // -----------------------------------------------------------

        int totalCount;
        map<string, int> foundTagsRatings;
        vector<string> foundTags;
        map<string, set<string>> mTagsUsers;
        map<string, int> mFastSearch;

        // Search simple tags without join content data
        if (type == "tags")
            return request.DbConnection()->SearchRepoInst->SearchTags(searchRequest);

        // --- Search posts by Search String -----------------------------------------------
        if (fs || all || type == "posts") {
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

        if (type == "videolink") {
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

        // --- Search Users by Search String -----------------------------------------------
        if (all || type == "users") {
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

        // --- Autocomplete for search string
        if (fs) {
            struct IntCmp {
                bool operator()(const pair<string, int>& lhs, const pair<string, int>& rhs)
                {
                    return lhs.second > rhs.second; // Changed  < to > since we need DESC order
                }
            };

            UniValue fastsearch(UniValue::VARR);
            vector<pair<string, int>> vFastSearch;
            for (const auto& f : mFastSearch) {
                vFastSearch.push_back(pair<string, int>(f.first, f.second));
            }

            sort(vFastSearch.begin(), vFastSearch.end(), IntCmp());
            int _count = fsresultCount;
            for (auto& t : vFastSearch) {
                fastsearch.push_back(t.first);
                _count -= 1;
                if (_count <= 0) break;
            }
            result.pushKV("fastsearch", fastsearch);
        }

        return result;
    }
}
