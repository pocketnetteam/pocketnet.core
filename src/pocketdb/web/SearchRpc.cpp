// Copyright (c) 2018-2022 The Pocketnet developers
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
            searchRequest.OrderByRank = true;
            searchRequest.FieldTypes = {
                ContentFieldType_AccountUserName,
                // ContentFieldType_AccountUserAbout,
                // ContentFieldType_AccountUserUrl
            };

            // Search
            auto ids = request.DbConnection()->SearchRepoInst->SearchUsers(searchRequest);
            
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
        return RPCHelpMan{"SearchUsers",
                "\nSearch users in DB.\n",
                {
                    // TODO (losty-fur): update argumants probably?
                    {"keyword", RPCArg::Type::STR, RPCArg::Optional::NO, "String for search"},
                    {"fieldtype", RPCArg::Type::STR, RPCArg::Optional::OMITTED_NAMED_ARG, ""},
                    {"orderbyrank", RPCArg::Type::NUM, RPCArg::Optional::OMITTED_NAMED_ARG, ""},
                },
                {
                    // TODO (losty-fur): provide return description
                },
                RPCExamples{
                    HelpExampleCli("SearchUsers", "\"keyword\", \"fieldtype\", orderbyrank") +
                    HelpExampleRpc("SearchUsers", "\"keyword\", \"fieldtype\", orderbyrank")
                },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
    {
        RPCTypeCheck(request.params, {UniValue::VSTR});

        SearchRequest searchRequest;

        searchRequest.Keyword = HtmlUtils::UrlDecode(request.params[0].get_str());
        searchRequest.FieldTypes = { ContentFieldType::ContentFieldType_AccountUserName };
        // ContentFieldType::ContentFieldType_AccountUserAbout, ContentFieldType::ContentFieldType_AccountUserUrl
        searchRequest.OrderByRank = true;

        auto ids = request.DbConnection()->SearchRepoInst->SearchUsers(searchRequest);
        auto usersProfiles = request.DbConnection()->WebRpcRepoInst->GetAccountProfiles(ids);

        UniValue result(UniValue::VARR);
        for (auto &profile : usersProfiles)
        {
            // profile.second.pushKV("searchResult", users[profile.first]);
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
                    {"contenttypes", RPCArg::Type::ARR, RPCArg::Optional::OMITTED_NAMED_ARG, "type(s) of content posts/video",
                        {
                            {"contenttype", RPCArg::Type::STR, RPCArg::Optional::NO, ""}   
                        }
                    },
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

    RPCHelpMan SearchContents()
    {
        return RPCHelpMan{"SearchContents",
                "\n\n", // TODO (team): provide description
                {   
                    // TODO (team): args
                },
                {
                    // TODO (losty-fur): provide return description
                },
                RPCExamples{
                    // TODO (team): examples
                    HelpExampleCli("SearchContents", "") +
                    HelpExampleRpc("SearchContents", "")
                },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
    {
        if (request.fHelp)
            throw runtime_error(
                "SearchContents"
            );

        RPCTypeCheck(request.params, {UniValue::VSTR});
        string keyword = HtmlUtils::UrlDecode(request.params[0].get_str());

        vector<int> contentTypes;
        ParseRequestContentTypes(request.params[1], contentTypes);

        vector<int> fieldTypes = { ContentFieldType::ContentFieldType_ContentPostCaption,
                                   ContentFieldType::ContentFieldType_ContentPostMessage,
                                   ContentFieldType::ContentFieldType_ContentPostUrl,
                                   ContentFieldType::ContentFieldType_ContentVideoCaption,
                                   ContentFieldType::ContentFieldType_ContentVideoMessage,
                                   ContentFieldType::ContentFieldType_ContentVideoUrl};

        // auto contents = request.DbConnection()->SearchRepoInst->SearchContents(keyword, contentTypes, fieldTypes, false);
        //
        // vector<int64_t> contentsIds;
        // for (const auto &content : contents)
        //     contentsIds.emplace_back(content.first);

        //auto usersProfiles = request.DbConnection()->WebRpcRepoInst->GetAccountProfiles(usersIds);

        UniValue result(UniValue::VARR);
        // for (auto &profile : usersProfiles)
        // {
        //     profile.second.pushKV("searchResult",users[profile.first]);
        //     result.push_back(profile.second);
        // }

        return result;
    },
        };
    }

    RPCHelpMan GetRecomendedAccountsBySubscriptions()
    {
        return RPCHelpMan{"GetRecomendedAccountsBySubscriptions",
                "\n\n", // TODO (team): provide description
                {   
                    // TODO (team): args
                },
                {
                    // TODO (losty-fur): provide return description
                },
                RPCExamples{
                    // TODO (team): examples
                    HelpExampleCli("GetRecomendedAccountsBySubscriptions", "") +
                    HelpExampleRpc("GetRecomendedAccountsBySubscriptions", "")
                },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
    {
        RPCTypeCheckArgument(request.params[0], UniValue::VSTR);
        string address = request.params[0].get_str();
        CTxDestination dest = DecodeDestination(address);

        if (!IsValidDestination(dest))
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, string("Invalid Pocketcoin address: ") + address);

        int cntOut = 10;
        if (request.params.size() > 1 && request.params[1].isNum())
            cntOut = request.params[1].get_int();

        return request.DbConnection()->SearchRepoInst->GetRecomendedAccountsBySubscriptions(address, cntOut);
    },
        };
    }

    RPCHelpMan GetRecomendedAccountsByScoresOnSimilarAccounts()
    {
        return RPCHelpMan{"getrecomendedaccountsbyscoresonsimilaraccounts",
                "\n\n", // TODO (team): provide description
                {   
                    // TODO (team): args
                },
                {
                    // TODO (losty-fur): provide return description
                },
                RPCExamples{
                    // TODO (team): examples
                    HelpExampleCli("getrecomendedaccountsbyscoresonsimilaraccounts", "") +
                    HelpExampleRpc("getrecomendedaccountsbyscoresonsimilaraccounts", "")
                },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
    {
        RPCTypeCheckArgument(request.params[0], UniValue::VSTR);
        string address = request.params[0].get_str();
        CTxDestination dest = DecodeDestination(address);

        if (!IsValidDestination(dest))
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, string("Invalid Pocketcoin address: ") + address);

        vector<int> contentTypes;
        ParseRequestContentTypes(request.params[1], contentTypes);

        int nHeight = ChainActive().Height();
        int depth = 1000;
        int cntOut = 10;

        if (request.params.size() > 2 && request.params[2].isNum() && request.params[2].get_int() > 0)
            nHeight = request.params[2].get_int();

        if (request.params.size() > 3 && request.params[3].isNum())
            depth = request.params[3].get_int();

        if (request.params.size() > 4 && request.params[4].isNum())
            cntOut = request.params[4].get_int();

        return request.DbConnection()->SearchRepoInst->GetRecomendedAccountsByScoresOnSimilarAccounts(address, contentTypes, nHeight, depth, cntOut);
    },
        };
    }

    RPCHelpMan GetRecomendedAccountsByScoresFromAddress()
    {
        return RPCHelpMan{"GetRecomendedAccountsByScoresFromAddress",
                "\n\n", // TODO (team): provide description
                {   
                    // TODO (team): args
                },
                {
                    // TODO (losty-fur): provide return description
                },
                RPCExamples{
                    // TODO (team): examples
                    HelpExampleCli("GetRecomendedAccountsByScoresFromAddress", "") +
                    HelpExampleRpc("GetRecomendedAccountsByScoresFromAddress", "")
                },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
    {
        RPCTypeCheckArgument(request.params[0], UniValue::VSTR);
        string address = request.params[0].get_str();
        CTxDestination dest = DecodeDestination(address);

        if (!IsValidDestination(dest))
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, string("Invalid Pocketcoin address: ") + address);

        vector<int> contentTypes;
        ParseRequestContentTypes(request.params[1], contentTypes);

        int nHeight = ChainActive().Height();
        int depth = 1000;
        int cntOut = 10;

        if (request.params.size() > 2 && request.params[2].isNum() && request.params[2].get_int() > 0)
            nHeight = request.params[2].get_int();

        if (request.params.size() > 3 && request.params[3].isNum())
            depth = request.params[3].get_int();

        if (request.params.size() > 4 && request.params[4].isNum())
            cntOut = request.params[4].get_int();

        return request.DbConnection()->SearchRepoInst->GetRecomendedAccountsByScoresFromAddress(address, contentTypes, nHeight, depth, cntOut);
    },
        };
    }

    RPCHelpMan GetRecomendedContentsByScoresOnSimilarContents()
    {
        return RPCHelpMan{"GetRecomendedContentsByScoresOnSimilarContents",
                "\n\n", // TODO (team): provide description
                {   
                    // TODO (team): args
                },
                {
                    // TODO (losty-fur): provide return description
                },
                RPCExamples{
                    // TODO (team): examples
                    HelpExampleCli("GetRecomendedContentsByScoresOnSimilarContents", "") +
                    HelpExampleRpc("GetRecomendedContentsByScoresOnSimilarContents", "")
                },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
    {
        return request.DbConnection()->SearchRepoInst->GetRecomendedContentsByScoresOnSimilarContents();
    },
        };
    }

    RPCHelpMan GetRecomendedContentsByScoresFromAddress()
    {
        return RPCHelpMan{"GetRecomendedContentsByScoresFromAddress",
                "\n\n", // TODO (team): provide description
                {   
                    // TODO (team): args
                },
                {
                    // TODO (losty-fur): provide return description
                },
                RPCExamples{
                    // TODO (team): examples
                    HelpExampleCli("GetRecomendedContentsByScoresFromAddress", "") +
                    HelpExampleRpc("GetRecomendedContentsByScoresFromAddress", "")
                },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
    {
        return request.DbConnection()->SearchRepoInst->GetRecomendedContentsByScoresFromAddress();
    },
        };
    }
}
