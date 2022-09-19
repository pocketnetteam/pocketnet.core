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
                    // TODO (rpc): provide return description
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
                if (!ParseInt32(request.params[2].get_str(), &searchRequest.TopBlock))
                    throw JSONRPCError(RPC_INVALID_PARAMETER, "Failed to parse int from string");
            }
            catch (...) { }
        }

        // PageStart
        if (request.params.size() > 3)
        {
            try
            {
                if (!ParseInt32(request.params[3].get_str(), &searchRequest.PageStart))
                    throw JSONRPCError(RPC_INVALID_PARAMETER, "Failed to parse int from string");
            }
            catch (...) { }
        }

        // PageSize
        if (request.params.size() > 4)
        {
            try
            {
                if (!ParseInt32(request.params[4].get_str(), &searchRequest.PageSize))
                    throw JSONRPCError(RPC_INVALID_PARAMETER, "Failed to parse int from string");
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
            // TODO (brangr): realize search indexing
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
            auto ids = request.DbConnection()->SearchRepoInst->SearchUsersOld(searchRequest);
            
            // Get accounts data
            auto accounts = request.DbConnection()->WebRpcRepoInst->GetAccountProfiles(ids);

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
                    // TODO (rpc): update argumants probably?
                    {"keyword", RPCArg::Type::STR, RPCArg::Optional::NO, "String for search"},
                    {"fieldtype", RPCArg::Type::STR, RPCArg::Optional::OMITTED_NAMED_ARG, ""},
                    {"orderbyrank", RPCArg::Type::NUM, RPCArg::Optional::OMITTED_NAMED_ARG, ""},
                },
                {
                    // TODO (rpc): provide return description
                },
                RPCExamples{
                    HelpExampleCli("SearchUsers", "\"keyword\", \"fieldtype\", orderbyrank") +
                    HelpExampleRpc("SearchUsers", "\"keyword\", \"fieldtype\", orderbyrank")
                },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
    {
        RPCTypeCheck(request.params, {UniValue::VSTR});

        UniValue result(UniValue::VARR);

        string keyword = HtmlUtils::UrlDecode(request.params[0].get_str());
        if (keyword.size() <= 1)
            return result;

        auto ids = request.DbConnection()->SearchRepoInst->SearchUsers(keyword);
        auto usersProfiles = request.DbConnection()->WebRpcRepoInst->GetAccountProfiles(ids);
        
        for (auto& id : ids)
            result.push_back(usersProfiles[id]);

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
                    // TODO (rpc): provide return description
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
                    // TODO (rpc): provide return description
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

    RPCHelpMan GetRecommendedContentByAddress()
    {
        return RPCHelpMan{"GetRecommendedContentByAddress",
                          "\n\n", // TODO (rpc): provide description
                          {
                                  // TODO (rpc): args
                          },
                          {
                                  // TODO (rpc): provide return description
                          },
                          RPCExamples{
                                  // TODO (team): examples
                                  HelpExampleCli("GetRecommendedContentByAddress", "") +
                                  HelpExampleRpc("GetRecommendedContentByAddress", "")
                          },
    [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
    {
        // if (request.fHelp)
        //     throw runtime_error(
        //         "getrecommendedcontentbyaddress \"address\", \"addressExclude\", \"contenttypes\", \"lang\", count\n"
        //         "\nContents recommendations by content address.\n"
        //         "\nArguments:\n"
        //         "1. \"address\" (string) Address for recommendations\n"
        //         "2. \"addressExclude\" (string, optional) Address for exclude from recommendations\n"
        //         "3. \"contenttypes\" (string or array of strings, optional) type(s) of content posts/videos/articles\n"
        //         "3. \"lang\" (string, optional) Language for recommendations\n"
        //         "4. \"countRec\" (int, optional) Number of recommendations records. Default 15\n"
        //         "5. \"countOthers\" (int, optional) Number of other contents from address. Default equal countRec\n"
        //         "6. \"countSubs\" (int, optional) Number of contents from address subscriptions. Default equal countRec\n"
        //     );

        RPCTypeCheckArgument(request.params[0], UniValue::VSTR);
        string address = "";
        if (request.params.size() > 0 && request.params[0].isStr()) {
            address = request.params[0].get_str();

            if(!address.empty()) {
                CTxDestination dest = DecodeDestination(address);
                if (!IsValidDestination(dest))
                    throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, string("Invalid address: ") + address);
            }
        }

        string addressExclude = "";
        if (request.params.size() > 1 && request.params[1].isStr()) {
            addressExclude = request.params[1].get_str();

            if(!addressExclude.empty()) {
                CTxDestination dest = DecodeDestination(addressExclude);
                if (!IsValidDestination(dest))
                    throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, string("Invalid address: ") + addressExclude);
            }
        }

        vector<int> contentTypes;
        if(request.params.size()>2)
            ParseRequestContentTypes(request.params[2], contentTypes);

        string lang = "";
        if (request.params.size() > 3 && request.params[3].isStr())
            lang = request.params[3].get_str();

        int cntRec = 15;
        if (request.params.size() > 4 && request.params[4].isNum())
            cntRec = request.params[4].get_int();
        cntRec = std::min(cntRec, 30);

        int cntOthers = cntRec;
        if (request.params.size() > 5 && request.params[5].isNum())
            cntOthers = request.params[5].get_int();
        cntOthers = std::min(cntOthers, 30);

        int cntSubs = cntRec;
        if (request.params.size() > 6 && request.params[6].isNum())
            cntSubs = request.params[6].get_int();
        cntSubs = std::min(cntSubs, 30);

        int nHeight = ChainActive().Height();
        // if (request.params.size() > 5 && request.params[5].isNum() && request.params[5].get_int() > 0)
        //     nHeight = request.params[5].get_int();

        int depth = (60 * 24 * 30 * 3); //about 3 month as default
        // if (request.params.size() > 6 && request.params[6].isNum())
        // {
        //     depth = std::max(request.params[6].get_int(), (60 * 24 * 30 * 6)); // not greater than about 6 month
        // }

        UniValue resultContent(UniValue::VARR);
        auto ids = request.DbConnection()->SearchRepoInst->GetRecommendedContentByAddressSubscriptions(address, addressExclude, contentTypes, lang, cntRec, nHeight, depth);
        if (!ids.empty())
        {
            auto contents = request.DbConnection()->WebRpcRepoInst->GetContentsData(ids, "");
            resultContent.push_backV(contents);
        }

        ids = request.DbConnection()->SearchRepoInst->GetRandomContentByAddress(address, contentTypes, lang, cntOthers);
        if (!ids.empty())
        {
            auto contents = request.DbConnection()->WebRpcRepoInst->GetContentsData(ids, "");
            resultContent.push_backV(contents);
        }

        ids = request.DbConnection()->SearchRepoInst->GetContentFromAddressSubscriptions(address, contentTypes, lang, cntSubs, false);
        if (!ids.empty())
        {
            auto contents = request.DbConnection()->WebRpcRepoInst->GetContentsData(ids, "");
            resultContent.push_backV(contents);
        }

        if (ids.size() < cntSubs)
        {
            int restCntSubs = cntSubs - ids.size();
            ids = request.DbConnection()->SearchRepoInst->GetContentFromAddressSubscriptions(address, contentTypes, lang, cntSubs, true);
            if (!ids.empty())
            {
                restCntSubs = std::min(restCntSubs, static_cast<int>(ids.size()));
                ids = {ids.begin(), ids.begin() + restCntSubs};
                auto contents = request.DbConnection()->WebRpcRepoInst->GetContentsData(ids, "");
                resultContent.push_backV(contents);
            }
        }

        UniValue result(UniValue::VOBJ);
        result.pushKV("contents", resultContent);
        return result;
    },
        };
    }

    RPCHelpMan GetRecommendedAccountByAddress()
    {
        return RPCHelpMan{"GetRecommendedContentByAddress",
                          "\n\n", // TODO (rpc): provide description
                          {
                                  // TODO (rpc): args
                          },
                          {
                                  // TODO (rpc): provide return description
                          },
                          RPCExamples{
                                  // TODO (team): examples
                                  HelpExampleCli("GetRecommendedContentByAddress", "") +
                                  HelpExampleRpc("GetRecommendedContentByAddress", "")
                          },
    [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
    {
        //   if (request.fHelp)
        //       throw runtime_error(
        //               "getrecommendedaccountbyaddress \"address\", \"addressExclude\", \"contenttypes\", \"lang\", count\n"
        //               "\nAccounts recommendations by address.\n"
        //               "\nArguments:\n"
        //               "1. \"address\" (string) Address for recommendations\n"
        //               "2. \"addressExclude\" (string, optional) Address for exclude from recommendations\n"
        //               "3. \"contenttypes\" (string or array of strings, optional) type(s) of content posts/videos/articles\n"
        //               "3. \"lang\" (string, optional) Language for recommendations\n"
        //               "4. \"count\" (int, optional) Number of recommendations records and number of other contents from addres. Default 15\n"
        //       );
        RPCTypeCheckArgument(request.params[0], UniValue::VSTR);
        string address = "";
        if (request.params.size() > 0 && request.params[0].isStr()) {
            address = request.params[0].get_str();

            if(!address.empty()) {
                CTxDestination dest = DecodeDestination(address);
                if (!IsValidDestination(dest))
                    throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, string("Invalid address: ") + address);
            }
        }

        string addressExclude = "";
        if (request.params.size() > 1 && request.params[1].isStr()) {
            addressExclude = request.params[1].get_str();

            if(!addressExclude.empty()) {
                CTxDestination dest = DecodeDestination(addressExclude);
                if (!IsValidDestination(dest))
                    throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, string("Invalid address: ") + addressExclude);
            }
        }

        vector<int> contentTypes;
        if(request.params.size()>2)
            ParseRequestContentTypes(request.params[2], contentTypes);

        string lang = "";
        if (request.params.size() > 3 && request.params[3].isStr())
            lang = request.params[3].get_str();

        int cntOut = 15;
        if (request.params.size() > 4 && request.params[4].isNum())
            cntOut = request.params[4].get_int();

        int nHeight = ChainActive().Height();
        if (request.params.size() > 5 && request.params[5].isNum() && request.params[5].get_int() > 0)
            nHeight = request.params[5].get_int();

        int depth = (60 * 24 * 30 * 3); //about 3 month as default
        if (request.params.size() > 6 && request.params[6].isNum())
        {
            depth = std::max(request.params[6].get_int(), (60 * 24 * 30 * 6)); // not greater than about 6 month
        }

        UniValue result(UniValue::VARR);
        auto ids = request.DbConnection()->SearchRepoInst->GetRecommendedAccountByAddressSubscriptions(address, addressExclude, contentTypes, lang, cntOut, nHeight, depth);
        if (!ids.empty())
        {
            auto profiles = request.DbConnection()->WebRpcRepoInst->GetAccountProfiles(ids);
            for (const auto[id, record] : profiles)
                result.push_back(record);
        }

        return result;
    },
        };
    }
}
