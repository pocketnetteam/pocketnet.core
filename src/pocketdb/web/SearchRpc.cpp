// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include "pocketdb/web/SearchRpc.h"

namespace PocketWeb::PocketWebRpc
{
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
        string type = request.params[1].get_str();
        HtmlUtils::StringToLower(type);

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
    }

    UniValue SearchUsers(const JSONRPCRequest& request)
    {
        if (request.fHelp)
            throw runtime_error(
                "SearchUsers"
            );

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
    }

    UniValue SearchLinks(const JSONRPCRequest& request)
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

        int nHeight = chainActive.Height();
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
    }

    UniValue SearchContents(const JSONRPCRequest& request)
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
    }

    UniValue GetRecommendedContentByAddress(const JSONRPCRequest& request)
    {
        if (request.fHelp)
            throw runtime_error(
                "getrecommendedcontentbyaddress \"address\", \"addressExclude\", \"contenttypes\", \"lang\", count\n"
                "\nContents recommendations by content address.\n"
                "\nArguments:\n"
                "1. \"address\" (string) Address for recommendations\n"
                "2. \"addressExclude\" (string, optional) Address for exclude from recommendations\n"
                "3. \"contenttypes\" (string or array of strings, optional) type(s) of content posts/videos/articles\n"
                "3. \"lang\" (string, optional) Language for recommendations\n"
                "4. \"countRec\" (int, optional) Number of recommendations records. Default 15\n"
                "5. \"countOthers\" (int, optional) Number of other contents from address. Default equal countRec\n"
                "6. \"countSubs\" (int, optional) Number of contents from address subscriptions. Default equal countRec\n"
            );

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

        int nHeight = chainActive.Height();
        // if (request.params.size() > 5 && request.params[5].isNum() && request.params[5].get_int() > 0)
        //     nHeight = request.params[5].get_int();

        int depth = (60 * 24 * 30 * 3); //about 3 month as default
        if (request.params.size() > 7 && request.params[7].isNum())
            depth = request.params[7].get_int();
        depth = std::min(depth, (60 * 24 * 30 * 3)); // not greater than about 3 month

        UniValue resultContent(UniValue::VARR);
        auto ids = request.DbConnection()->SearchRepoInst->GetRecommendedContentByAddressSubscriptions(address, addressExclude, contentTypes, lang, cntRec, nHeight, depth);
        if (!ids.empty())
        {
            auto contents = request.DbConnection()->WebRpcRepoInst->GetContentsData(ids, "");
            resultContent.push_backV(contents);
        }

        if (cntOthers > 0) {
            ids = request.DbConnection()->SearchRepoInst->GetRandomContentByAddress(address, contentTypes, lang, cntOthers);
            if (!ids.empty()) {
                auto contents = request.DbConnection()->WebRpcRepoInst->GetContentsData(ids, "");
                resultContent.push_backV(contents);
            }
        }

        if (cntSubs > 0) {
            ids = request.DbConnection()->SearchRepoInst->GetContentFromAddressSubscriptions(address, contentTypes, lang, cntSubs, false);
            if (!ids.empty()) {
                auto contents = request.DbConnection()->WebRpcRepoInst->GetContentsData(ids, "");
                resultContent.push_backV(contents);
            }

            if (ids.size() < cntSubs) {
                int restCntSubs = cntSubs - ids.size();
                ids = request.DbConnection()->SearchRepoInst->GetContentFromAddressSubscriptions(address, contentTypes, lang, cntSubs, true);
                if (!ids.empty()) {
                    restCntSubs = std::min(restCntSubs, static_cast<int>(ids.size()));
                    ids = {ids.begin(), ids.begin() + restCntSubs};
                    auto contents = request.DbConnection()->WebRpcRepoInst->GetContentsData(ids, "");
                    resultContent.push_backV(contents);
                }
            }
        }

        UniValue result(UniValue::VOBJ);
        result.pushKV("contents", resultContent);
        return result;
    }

    UniValue GetRecommendedAccountByAddress(const JSONRPCRequest& request)
    {
        if (request.fHelp)
            throw runtime_error(
                "getrecommendedaccountbyaddress \"address\", \"addressExclude\", \"contenttypes\", \"lang\", count\n"
                "\nAccounts recommendations by address.\n"
                "\nArguments:\n"
                "1. \"address\" (string) Address for recommendations\n"
                "2. \"addressExclude\" (string, optional) Address for exclude from recommendations\n"
                "3. \"contenttypes\" (string or array of strings, optional) type(s) of content posts/videos/articles\n"
                "3. \"lang\" (string, optional) Language for recommendations\n"
                "4. \"count\" (int, optional) Number of recommendations records and number of other contents from addres. Default 15\n"
            );

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

        int nHeight = chainActive.Height();
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
    }
}
