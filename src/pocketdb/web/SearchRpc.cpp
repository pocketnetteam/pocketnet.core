// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include "pocketdb/web/SearchRpc.h"
#include "rpc/util.h"
#include "validation.h"

namespace PocketWeb::PocketWebRpc
{
    RPCHelpMan Search()
    {
        return RPCHelpMan{"search",
                "\nSearch data in DB.\n",
                {
                    {"keyword", RPCArg::Type::STR, RPCArg::Optional::NO, "String for search"},
                    {"type", RPCArg::Type::STR, RPCArg::Optional::OMITTED_NAMED_ARG, "posts, videolink, tags, users"},
                    {"topBlock", RPCArg::Type::NUM, RPCArg::Optional::OMITTED_NAMED_ARG, "Top block for search."},
                    {"pageStart", RPCArg::Type::NUM, RPCArg::Optional::OMITTED_NAMED_ARG, "Pagination start. Default 0"},
                    {"pageSize", RPCArg::Type::NUM, RPCArg::Optional::OMITTED_NAMED_ARG, "Pagination count. Default 10"},
                    {"address", RPCArg::Type::STR, RPCArg::Optional::OMITTED_NAMED_ARG, "Filter by address"},
                },
                {
                    // TODO (losty-fur): provide return description
                },
                RPCExamples{
                    HelpExampleCli("search", "\"keyword\", \"type\", topBlock, pageStart, pageSize, \"address\"") +
                    HelpExampleRpc("search", "\"keyword\", \"type\", topBlock, pageStart, pageSize, \"address\"")
                },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
    {
        RPCTypeCheck(request.params, {UniValue::VSTR, UniValue::VSTR});

        SearchRequest searchRequest;

        // General params
        searchRequest.Keyword = HtmlUtils::UrlDecode(request.params[0].get_str());
        string type = request.params[1].get_str();
        HtmlUtils::StringToLower(type);

        // Optional parameters
        // TopBlock
        if (request.params.size() > 2)
        {
            try
            {
                // TODO (losty-fur): do not ignore result
                bool res = ParseInt32(request.params[2].get_str(), &searchRequest.TopBlock);
            }
            catch (...) { }
        }

        // PageStart
        if (request.params.size() > 3)
        {
            try
            {
                // TODO (losty-fur): do not ignore result
                bool res = ParseInt32(request.params[3].get_str(), &searchRequest.PageStart);
            }
            catch (...) { }
        }

        // PageSize
        if (request.params.size() > 4)
        {
            try
            {
                // TODO (losty-fur): do not ignore result
                bool res = ParseInt32(request.params[4].get_str(), &searchRequest.PageSize);
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
        
        // Search simple tags without join content data
        if (type == "tags")
        {
            UniValue data = request.DbConnection()->SearchRepoInst->SearchTags(searchRequest);
            result.pushKV("tags", UniValue(UniValue::VOBJ));
            result.At("tags").pushKV("data", data);
        }

        // Search posts in caption, message and urls
        if (type == "posts")
        {
            searchRequest.TxTypes = { CONTENT_POST, CONTENT_VIDEO };
            searchRequest.FieldTypes = {
                ContentFieldType_ContentPostCaption,
                ContentFieldType_ContentVideoCaption,
                ContentFieldType_ContentPostMessage,
                ContentFieldType_ContentVideoMessage,
                // ContentFieldType_ContentPostUrl,
                // ContentFieldType_ContentVideoUrl,
            };

            // Search
            auto ids = request.DbConnection()->SearchRepoInst->SearchIds(searchRequest);
            
            // Get content data
            auto contents = request.DbConnection()->WebRpcRepoInst->GetContentsData(ids, searchRequest.Address);
            
            UniValue data(UniValue::VARR);
            data.push_backV(contents);

            result.pushKV("posts", UniValue(UniValue::VOBJ));
            result.At("posts").pushKV("data", data);
        }

        // Get all videos with requested link
        if (type == "videolink")
        {
            searchRequest.TxTypes = { CONTENT_VIDEO };
            searchRequest.FieldTypes = { ContentFieldType_ContentVideoUrl };

            // Search
            auto ids = request.DbConnection()->SearchRepoInst->SearchIds(searchRequest);
            
            // Get content data
            auto contents = request.DbConnection()->WebRpcRepoInst->GetContentsData(ids, searchRequest.Address);

            UniValue data(UniValue::VARR);
            data.push_backV(contents);

            result.pushKV("videos", UniValue(UniValue::VOBJ));
            result.At("videos").pushKV("data", data);
        }

        // Search users
        if (type == "users")
        {
            searchRequest.Address = "";
            searchRequest.TxTypes = { ACCOUNT_USER };
            searchRequest.FieldTypes = {
                ContentFieldType_AccountUserName,
                ContentFieldType_AccountUserAbout,
                ContentFieldType_AccountUserUrl
            };

            // Search
            auto ids = request.DbConnection()->SearchRepoInst->SearchIds(searchRequest);
            
            // Get accounts data
            auto accounts = request.DbConnection()->WebRpcRepoInst->GetAccountProfiles(ids, true);

            UniValue data(UniValue::VARR);
            for (const auto& account : accounts)
                data.push_back(account.second);

            result.pushKV("users", UniValue(UniValue::VOBJ));
            result.At("users").pushKV("data", data);
        }
        
        // Send result
        
        return result;
    },
        };
    }

    RPCHelpMan SearchUsers()
    {
        return RPCHelpMan{"searchusers",
                "\nSearch users in DB.\n",
                {
                    {"keyword", RPCArg::Type::STR, RPCArg::Optional::NO, "String for search"},
                    {"fieldtype", RPCArg::Type::STR, RPCArg::Optional::OMITTED_NAMED_ARG, ""},
                    {"orderbyrank", RPCArg::Type::NUM, RPCArg::Optional::OMITTED_NAMED_ARG, ""},
                },
                {
                    // TODO (losty-fur): provide return description
                },
                RPCExamples{
                    HelpExampleCli("searchusers", "\"keyword\", \"fieldtype\", orderbyrank") +
                    HelpExampleRpc("searchusers", "\"keyword\", \"fieldtype\", orderbyrank")
                },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
    {
        RPCTypeCheck(request.params, {UniValue::VSTR});
        string keyword = HtmlUtils::UrlDecode(request.params[0].get_str());

        vector<int> fieldTypes = { ContentFieldType::ContentFieldType_AccountUserName };
        // ContentFieldType::ContentFieldType_AccountUserAbout, ContentFieldType::ContentFieldType_AccountUserUrl
        auto users = request.DbConnection()->SearchRepoInst->SearchUsers(keyword, fieldTypes, false);

        vector<int64_t> usersIds;
        for (const auto &user : users)
            usersIds.emplace_back(user.first);

        auto usersProfiles = request.DbConnection()->WebRpcRepoInst->GetAccountProfiles(usersIds);

        UniValue result(UniValue::VARR);
        for (auto &profile : usersProfiles)
        {
            profile.second.pushKV("searchResult",users[profile.first]);
            result.push_back(profile.second);
        }

        return result;
    },
        };
    }

    RPCHelpMan SearchLinks()
    {
        return RPCHelpMan{"searchlinks",
                "\nSearch links in DB.\n",
                {
                    {"links", RPCArg::Type::STR, RPCArg::Optional::NO, "String for search"},
                    {"contenttypes", RPCArg::Type::ARR, RPCArg::Optional::OMITTED_NAMED_ARG, "type(s) of content posts/video"},
                    {"height", RPCArg::Type::NUM, RPCArg::Optional::OMITTED_NAMED_ARG, "Maximum search height. Default is current chain height"},
                    {"count", RPCArg::Type::NUM, RPCArg::Optional::OMITTED_NAMED_ARG, "Number of resulting records. Default 10"},
                },
                {
                    // TODO (losty-fur): provide return description
                },
                RPCExamples{
                    HelpExampleCli("searchlinks", "[\"links\", ...], \"contenttypes\", height, count") +
                    HelpExampleRpc("searchlinks", "[\"links\", ...], \"contenttypes\", height, count")
                },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
    {
        if (request.fHelp)
            throw runtime_error(
                "searchlinks [\"links\", ...], \"contenttypes\", height, count\n"
                "\nSearch links in DB.\n"
                "\nArguments:\n"
                "1. \"links\" (Array of strings) String for search\n"
                "2. \"contenttypes\" (string or array of strings, optional) type(s) of content posts/video\n"
                "3. \"height\"  (int, optional) Maximum search height. Default is current chain height\n"
                "4. \"count\" (int, optional) Number of resulting records. Default 10\n"
            );

        RPCTypeCheck(request.params, {UniValue::VARR});

        std::vector<std::string> vLinks;
        UniValue lnks = request.params[0].get_array();
        if (lnks.size() > 100)
            throw JSONRPCError(RPC_INVALID_PARAMS, "The array is too large");
        else if (lnks.size() > 0)
            for (unsigned int idx = 0; idx < lnks.size(); idx++)
                vLinks.emplace_back(lnks[idx].get_str());

        vector<int> contentTypes;
        ParseRequestContentTypes(request.params[1], contentTypes);

        int nHeight = ::ChainActive().Height();
        if (request.params.size() > 2)
        {
            RPCTypeCheckArgument(request.params[2], UniValue::VNUM);
            if (request.params[2].get_int() > 0)
                nHeight = request.params[2].get_int();
        }

        int countOut = 10;
        if (request.params.size() > 3)
        {
            RPCTypeCheckArgument(request.params[3], UniValue::VNUM);
            if (request.params[3].get_int() > 0)
                countOut = request.params[3].get_int();
        }

        return request.DbConnection()->WebRpcRepoInst->SearchLinks(vLinks, contentTypes, nHeight, countOut);
    },
        };
    }

}
