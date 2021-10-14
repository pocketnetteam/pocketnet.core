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
                "search \"keyword\", \"type\", topBlock, pageStart, pageSize, \"address\"\n"
                "\nSearch data in DB.\n"
                "\nArguments:\n"
                "1. \"keyword\"     (string) String for search\n"
                "2. \"type\"        (string, optional) posts, videolink, tags, users\n"
                "3. \"topBlock\"  (int, optional) Top block for search.\n"
                "4. \"pageStart\" (int, optional) Pagination start. Default 0\n"
                "5. \"pageSize\" (int, optional) Pagination count. Default 10\n"
                "5. \"address\"     (string, optional) Filter by address\n"
            );

        RPCTypeCheck(request.params, {UniValue::VSTR, UniValue::VSTR});

        SearchRequest searchRequest;

        // General params
        searchRequest.Keyword = HtmlUtils::UrlDecode(request.params[0].get_str());
        string type = lower(request.params[1].get_str());

        // Optional parameters
        // TopBlock
        if (request.params.size() > 2)
        {
            try
            {
                ParseInt32(request.params[2].get_str(), &searchRequest.TopBlock);
            }
            catch (...) { }
        }

        // PageStart
        if (request.params.size() > 3)
        {
            try
            {
                ParseInt32(request.params[3].get_str(), &searchRequest.PageStart);
            }
            catch (...) { }
        }

        // PageSize
        if (request.params.size() > 4)
        {
            try
            {
                ParseInt32(request.params[4].get_str(), &searchRequest.PageSize);
            }
            catch (...) { }
        }

        // Address
        if (request.params.size() > 5) {
            RPCTypeCheckArgument(request.params[5], UniValue::VSTR);
            CTxDestination dest = DecodeDestination(request.params[5].get_str());
            if (!IsValidDestination(dest)) {
                throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, string("Invalid Pocketcoin address: ") + request.params[5].get_str());
            }
            searchRequest.Address = request.params[5].get_str();
        }

        // -----------------------------------------------------------
        UniValue result(UniValue::VOBJ);
        UniValue data(UniValue::VARR);

        // Search simple tags without join content data
        if (type == "tags")
            data = request.DbConnection()->SearchRepoInst->SearchTags(searchRequest);

        // Search posts in caption, message and urls
        if (type == "posts")
            data = request.DbConnection()->SearchRepoInst->SearchPosts(searchRequest);
        
        // Get all videos with equal link
        if (type == "videolink")
            data = request.DbConnection()->SearchRepoInst->SearchVideoLink(searchRequest);

        // Search users
        if (type == "users")
            data = request.DbConnection()->SearchRepoInst->SearchAccounts(searchRequest);
        
        // Send result
        result.pushKV("data", data);
        return result;
    }
}
