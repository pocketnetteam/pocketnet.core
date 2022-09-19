// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include <pocketdb/consensus/Base.h>
#include "pocketdb/web/PocketContentRpc.h"
#include "pocketdb/helpers/ShortFormHelper.h"

namespace PocketWeb::PocketWebRpc
{
    void ParseFeedRequest(const JSONRPCRequest& request, int& topHeight, string& topContentHash, int& countOut, string& lang, vector<string>& tags,
        vector<int>& contentTypes, vector<string>& txIdsExcluded, vector<string>& adrsExcluded, vector<string>& tagsExcluded, string& address)
    {
        topHeight = chainActive.Height();
        if (request.params.size() > 0 && request.params[0].isNum() && request.params[0].get_int() > 0)
            topHeight = request.params[0].get_int();

        if (request.params.size() > 1 && request.params[1].isStr())
            topContentHash = request.params[1].get_str();

        countOut = 10;
        if (request.params.size() > 2 && request.params[2].isNum())
        {
            countOut = request.params[2].get_int();
            countOut = std::min(countOut, 20);
        }

        lang = "en";
        if (request.params.size() > 3 && request.params[3].isStr())
            lang = request.params[3].get_str();

        // tags
        if (request.params.size() > 4)
            ParseRequestTags(request.params[4], tags);

        // content types
        ParseRequestContentTypes(request.params[5], contentTypes);

        // exclude ids
        if (request.params.size() > 6)
        {
            if (request.params[6].isStr())
            {
                txIdsExcluded.push_back(request.params[6].get_str());
            }
            else if (request.params[6].isArray())
            {
                UniValue txids = request.params[6].get_array();
                for (unsigned int idx = 0; idx < txids.size(); idx++)
                {
                    string txidEx = boost::trim_copy(txids[idx].get_str());
                    if (!txidEx.empty())
                        txIdsExcluded.push_back(txidEx);

                    if (txIdsExcluded.size() > 100)
                        break;
                }
            }
        }

        // exclude addresses
        if (request.params.size() > 7)
        {
            if (request.params[7].isStr())
            {
                adrsExcluded.push_back(request.params[7].get_str());
            }
            else if (request.params[7].isArray())
            {
                UniValue adrs = request.params[7].get_array();
                for (unsigned int idx = 0; idx < adrs.size(); idx++)
                {
                    string adrEx = boost::trim_copy(adrs[idx].get_str());
                    if (!adrEx.empty())
                        adrsExcluded.push_back(adrEx);

                    if (adrsExcluded.size() > 100)
                        break;
                }
            }
        }

        // exclude tags
        if (request.params.size() > 8)
            ParseRequestTags(request.params[8], tagsExcluded);

        // address for person output
        if (request.params.size() > 9)
        {
            RPCTypeCheckArgument(request.params[9], UniValue::VSTR);
            address = request.params[9].get_str();
            if (!address.empty())
            {
                CTxDestination dest = DecodeDestination(address);

                if (!IsValidDestination(dest))
                    throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, string("Invalid Pocketcoin address: ") + address);
            }
        }
    }

    void ParseFeedRequest(const JSONRPCRequest& request, int& topHeight, string& topContentHash, int& countOut, string& lang, vector<string>& tags,
        vector<int>& contentTypes, vector<string>& txIdsExcluded, vector<string>& adrsExcluded, vector<string>& tagsExcluded, string& address, string& address_feed)
    {
        ParseFeedRequest(request, topHeight, topContentHash, countOut, lang, tags, contentTypes, txIdsExcluded, adrsExcluded, tagsExcluded, address);

        if (request.params.size() > 10)
        {
            // feed's address
            if (request.params[10].isStr())
            {
                address_feed = request.params[10].get_str();
                if (!address_feed.empty())
                {
                    CTxDestination dest = DecodeDestination(address_feed);

                    if (!IsValidDestination(dest))
                        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, string("Invalid Pocketcoin address: ") + address_feed);
                }
            }
        }
    }

    void ParseFeedRequest(const JSONRPCRequest& request, int& topHeight, string& topContentHash, int& countOut, string& lang, vector<string>& tags,
        vector<int>& contentTypes, vector<string>& txIdsExcluded, vector<string>& adrsExcluded, vector<string>& tagsExcluded, string& address, string& address_feed, vector<string>& addresses_extended)
    {
        ParseFeedRequest(request, topHeight, topContentHash, countOut, lang, tags, contentTypes, txIdsExcluded, adrsExcluded, tagsExcluded, address, address_feed);

        if (request.params.size() > 11) {
            if (request.params[11].isStr()) {
                addresses_extended.push_back(request.params[11].get_str());
            } else if (request.params[11].isArray()) {
                UniValue adrs = request.params[11].get_array();
                for (unsigned int idx = 0; idx < adrs.size(); idx++) {
                    string adrEx = boost::trim_copy(adrs[idx].get_str());
                    if (!adrEx.empty())
                        addresses_extended.push_back(adrEx);

                    if (addresses_extended.size() > 100)
                        break;
                }
            }
        }
    }

    void ParseFeedRequest(const JSONRPCRequest& request, int& topHeight, string& topContentHash, int& countOut, string& lang, vector<string>& tags,
        vector<int>& contentTypes, vector<string>& txIdsExcluded, vector<string>& adrsExcluded, vector<string>& tagsExcluded, string& address, string& address_feed, string& keyword, string& orderby, string& ascdesc)
    {
        ParseFeedRequest(request, topHeight, topContentHash, countOut, lang, tags, contentTypes, txIdsExcluded, adrsExcluded, tagsExcluded, address, address_feed);

        if (request.params.size() > 11 && request.params[11].isStr())
        {
            keyword = HtmlUtils::UrlDecode(request.params[11].get_str());
        }

        orderby = "id";
        if (request.params.size() > 12 && request.params[12].isStr())
        {
            if(request.params[12].get_str() == "comment") orderby = "comment";
            else if(request.params[12].get_str() == "score") orderby = "score";
        }

        ascdesc = "desc";
        if (request.params.size() > 13 && request.params[13].isStr())
        {
            ascdesc = request.params[13].get_str() == "asc" ? "asc" : "desc";
        }
    }

    UniValue GetContent(const JSONRPCRequest& request)
    {
        if (request.fHelp)
        {
            throw runtime_error(
                "getcontent ids[]\n"
                "\nReturns contents for list of ids");
        }

        vector<string> hashes;
        if (request.params.size() > 0)
        {
            if (request.params[0].isStr())
            {
                hashes.push_back(request.params[0].get_str());
            }
            else if (request.params[0].isArray())
            {
                UniValue txids = request.params[0].get_array();
                for (unsigned int idx = 0; idx < txids.size(); idx++)
                {
                    string txidEx = boost::trim_copy(txids[idx].get_str());
                    if (!txidEx.empty())
                        hashes.push_back(txidEx);

                    if (hashes.size() > 100)
                        break;
                }
            }
        }

        string address;
        if (request.params.size() > 1 && request.params[1].isStr())
            address = request.params[1].get_str();

        auto ids = request.DbConnection()->WebRpcRepoInst->GetContentIds(hashes);
        auto content = request.DbConnection()->WebRpcRepoInst->GetContentsData(ids, address);

        UniValue result(UniValue::VARR);
        result.push_backV(content);
        return result;
    }

    UniValue GetContents(const JSONRPCRequest& request)
    {
        if (request.fHelp)
        {
            throw runtime_error(
                "getcontents address\n"
                "\nReturns contents for address.\n"
                "\nArguments:\n"
                "1. address            (string) A pocketcoin addresses to filter\n"
                "\nResult\n"
                "[                     (array of contents)\n"
                "  ...\n"
                "]");
        }

        string address;
        if (request.params[0].isStr())
            address = request.params[0].get_str();

        // TODO (brangr, team): add pagination

        return request.DbConnection()->WebRpcRepoInst->GetContentsForAddress(address);
    }

    UniValue GetProfileFeed(const JSONRPCRequest& request)
    {
        if (request.fHelp)
            throw runtime_error(
                "GetProfileFeed\n"
                "topHeight           (int) - ???\n"
                "topContentHash      (string, optional) - ???\n"
                "countOut            (int, optional) - ???\n"
                "lang                (string, optional) - ???\n"
                "tags                (vector<string>, optional) - ???\n"
                "contentTypes        (vector<int>, optional) - ???\n"
                "txIdsExcluded       (vector<string>, optional) - ???\n"
                "adrsExcluded        (vector<string>, optional) - ???\n"
                "tagsExcluded        (vector<string>, optional) - ???\n"
                "address             (string, optional) - ???\n"
                "address_feed        (string) - ???\n"
                "keyword             (string) - ???\n"
                "orderby             (string) - ???\n"
                "ascdesc             (string) - ???\n"
            );

        int topHeight;
        string topContentHash;
        int countOut;
        string lang;
        vector<string> tags;
        vector<int> contentTypes;
        vector<string> txIdsExcluded;
        vector<string> adrsExcluded;
        vector<string> tagsExcluded;
        string address;
        string address_feed;
        string keyword;
        string orderby;
        string ascdesc;
        ParseFeedRequest(request, topHeight, topContentHash, countOut, lang, tags, contentTypes, txIdsExcluded,
            adrsExcluded, tagsExcluded, address, address_feed, keyword, orderby, ascdesc);

        if (address_feed.empty())
            throw JSONRPCError(RPC_INVALID_REQUEST, string("No profile address"));

        int64_t topContentId = 0;
        int pageNumber = 0;
        if (!topContentHash.empty())
        {
            auto ids = request.DbConnection()->WebRpcRepoInst->GetContentIds({topContentHash});
            if (!ids.empty())
                topContentId = ids[0];
        } else if (topContentHash.empty() && request.params.size() > 1 && request.params[1].isNum())
        {
            pageNumber = request.params[1].get_int();
        }

        UniValue result(UniValue::VOBJ);
        UniValue content = request.DbConnection()->WebRpcRepoInst->GetProfileFeed(
            address_feed, countOut, pageNumber, topContentId, topHeight, lang, tags, contentTypes,
            txIdsExcluded, adrsExcluded, tagsExcluded, address, keyword, orderby, ascdesc);

        result.pushKV("height", topHeight);
        result.pushKV("contents", content);
        return result;
    }

    UniValue GetHotPosts(const JSONRPCRequest& request)
    {
        if (request.fHelp)
            throw runtime_error(
                "GetHotPosts\n");

        int count = 30;
        if (request.params.size() > 0)
        {
            if (request.params[0].isNum())
                count = request.params[0].get_int();
            else if (request.params[0].isStr())
                ParseInt32(request.params[0].get_str(), &count);
        }

        // Depth in blocks (default about 3 days)
        int dayInBlocks = 24 * 60;
        int depthBlocks = 3 * dayInBlocks;
        if (request.params.size() > 1)
        {
            if (request.params[1].isNum())
                depthBlocks = request.params[1].get_int();
            else if (request.params[1].isStr())
                ParseInt32(request.params[1].get_str(), &depthBlocks);

            // for old version electron
            if (depthBlocks == 259200)
                depthBlocks = 3 * dayInBlocks;

            // Max 3 months
            depthBlocks = min(depthBlocks, 90 * dayInBlocks);
        }

        int nHeightOffset = chainActive.Height();
        int nOffset = 0;
        if (request.params.size() > 2)
        {
            if (request.params[2].isNum())
            {
                if (request.params[2].get_int() > 0)
                    nOffset = request.params[2].get_int();
            }
            else if (request.params[2].isStr())
            {
                ParseInt32(request.params[2].get_str(), &nOffset);
            }
            nHeightOffset -= nOffset;
        }

        string lang = "";
        if (request.params.size() > 3)
            lang = request.params[3].get_str();

        vector<int> contentTypes;
        ParseRequestContentTypes(request.params[4], contentTypes);

        string address = "";
        if (request.params.size() > 5)
            address = request.params[5].get_str();

        auto reputationConsensus = ReputationConsensusFactoryInst.Instance(chainActive.Height());
        auto badReputationLimit = reputationConsensus->GetConsensusLimit(ConsensusLimit_bad_reputation);

        return request.DbConnection()->WebRpcRepoInst->GetHotPosts(count, depthBlocks, nHeightOffset, lang,
            contentTypes, address, badReputationLimit);
    }

    UniValue GetHistoricalFeed(const JSONRPCRequest& request)
    {
        if (request.fHelp)
            throw runtime_error(
                "GetHistoricalFeed\n"
                "topHeight           (int) - ???\n"
                "topContentHash      (string, optional) - ???\n"
                "countOut            (int, optional) - ???\n"
                "lang                (string, optional) - ???\n"
                "tags                (vector<string>, optional) - ???\n"
                "contentTypes        (vector<int>, optional) - ???\n"
                "txIdsExcluded       (vector<string>, optional) - ???\n"
                "adrsExcluded        (vector<string>, optional) - ???\n"
                "tagsExcluded        (vector<string>, optional) - ???\n"
                "address             (string, optional) - ???\n"
            );

        int topHeight;
        string topContentHash;
        int countOut;
        string lang;
        vector<string> tags;
        vector<int> contentTypes;
        vector<string> txIdsExcluded;
        vector<string> adrsExcluded;
        vector<string> tagsExcluded;
        string address;
        ParseFeedRequest(request, topHeight, topContentHash, countOut, lang, tags, contentTypes, txIdsExcluded,
            adrsExcluded, tagsExcluded, address);

        int64_t topContentId = 0;
        if (!topContentHash.empty())
        {
            auto ids = request.DbConnection()->WebRpcRepoInst->GetContentIds({topContentHash});
            if (!ids.empty())
                topContentId = ids[0];
        }

        auto reputationConsensus = ReputationConsensusFactoryInst.Instance(chainActive.Height());
        auto badReputationLimit = reputationConsensus->GetConsensusLimit(ConsensusLimit_bad_reputation);

        UniValue result(UniValue::VOBJ);
        UniValue content = request.DbConnection()->WebRpcRepoInst->GetHistoricalFeed(
            countOut, topContentId, topHeight, lang, tags, contentTypes,
            txIdsExcluded, adrsExcluded, tagsExcluded,
            address, badReputationLimit);

        result.pushKV("height", topHeight);
        result.pushKV("contents", content);
        return result;
    }

    UniValue GetHierarchicalFeed(const JSONRPCRequest& request)
    {
        if (request.fHelp)
            throw runtime_error(
                "GetHierarchicalFeed\n"
                "topHeight           (int) - ???\n"
                "topContentHash      (string, optional) - ???\n"
                "countOut            (int, optional) - ???\n"
                "lang                (string, optional) - ???\n"
                "tags                (vector<string>, optional) - ???\n"
                "contentTypes        (vector<int>, optional) - ???\n"
                "txIdsExcluded       (vector<string>, optional) - ???\n"
                "adrsExcluded        (vector<string>, optional) - ???\n"
                "tagsExcluded        (vector<string>, optional) - ???\n"
                "address             (string, optional) - ???\n"
            );

        int topHeight;
        string topContentHash;
        int countOut;
        string lang;
        vector<string> tags;
        vector<int> contentTypes;
        vector<string> txIdsExcluded;
        vector<string> adrsExcluded;
        vector<string> tagsExcluded;
        string address;
        ParseFeedRequest(request, topHeight, topContentHash, countOut, lang, tags, contentTypes, txIdsExcluded,
            adrsExcluded, tagsExcluded, address);

        int64_t topContentId = 0;
        if (!topContentHash.empty())
        {
            auto ids = request.DbConnection()->WebRpcRepoInst->GetContentIds({topContentHash});
            if (!ids.empty())
                topContentId = ids[0];
        }

        auto reputationConsensus = ReputationConsensusFactoryInst.Instance(chainActive.Height());
        auto badReputationLimit = reputationConsensus->GetConsensusLimit(ConsensusLimit_bad_reputation);

        UniValue result(UniValue::VOBJ);
        UniValue content = request.DbConnection()->WebRpcRepoInst->GetHierarchicalFeed(
            countOut, topContentId, topHeight, lang, tags, contentTypes,
            txIdsExcluded, adrsExcluded, tagsExcluded,
            address, badReputationLimit);

        result.pushKV("height", topHeight);
        result.pushKV("contents", content);
        return result;
    }

    UniValue GetBoostFeed(const JSONRPCRequest& request)
    {
        if (request.fHelp)
            throw runtime_error(
                "GetHierarchicalFeed\n"
                "topHeight           (int) - ???\n"
                "topContentHash      (string, not supported) - ???\n"
                "countOut            (int, not supported) - ???\n"
                "lang                (string, optional) - ???\n"
                "tags                (vector<string>, optional) - ???\n"
                "contentTypes        (vector<int>, optional) - ???\n"
                "txIdsExcluded       (vector<string>, optional) - ???\n"
                "adrsExcluded        (vector<string>, optional) - ???\n"
                "tagsExcluded        (vector<string>, optional) - ???\n"
            );

        int topHeight;
        string lang;
        vector<string> tags;
        vector<int> contentTypes;
        vector<string> txIdsExcluded;
        vector<string> adrsExcluded;
        vector<string> tagsExcluded;

        string skipString = "";
        int skipInt =  0;
        ParseFeedRequest(request, topHeight, skipString, skipInt, lang, tags, contentTypes, txIdsExcluded,
            adrsExcluded, tagsExcluded, skipString);

        auto reputationConsensus = ReputationConsensusFactoryInst.Instance(chainActive.Height());
        auto badReputationLimit = reputationConsensus->GetConsensusLimit(ConsensusLimit_bad_reputation);

        UniValue result(UniValue::VOBJ);
        UniValue boosts = request.DbConnection()->WebRpcRepoInst->GetBoostFeed(
            topHeight, lang, tags, contentTypes,
            txIdsExcluded, adrsExcluded, tagsExcluded,
            badReputationLimit);

        result.pushKV("height", topHeight);
        result.pushKV("boosts", boosts);
        return result;
    }

    UniValue GetTopFeed(const JSONRPCRequest& request)
    {
        if (request.fHelp)
            throw runtime_error(
                "GetTopFeed\n"
                "topHeight           (int) - ???\n"
                "topContentHash      (string, optional) - ???\n"
                "countOut            (int, optional) - ???\n"
                "lang                (string, optional) - ???\n"
                "tags                (vector<string>, optional) - ???\n"
                "contentTypes        (vector<int>, optional) - ???\n"
                "txIdsExcluded       (vector<string>, optional) - ???\n"
                "adrsExcluded        (vector<string>, optional) - ???\n"
                "tagsExcluded        (vector<string>, optional) - ???\n"
                "address             (string, optional) - ???\n"
                "depth               (int, optional) - ???\n"
            );

        int topHeight;
        string topContentHash;
        int countOut;
        string lang;
        vector<string> tags;
        vector<int> contentTypes;
        vector<string> txIdsExcluded;
        vector<string> adrsExcluded;
        vector<string> tagsExcluded;
        string address;
        int depth = 60 * 24 * 30 * 12; // about 1 year
        ParseFeedRequest(request, topHeight, topContentHash, countOut, lang, tags, contentTypes, txIdsExcluded,
            adrsExcluded, tagsExcluded, address);
        // depth
        if (request.params.size() > 10)
        {
            RPCTypeCheckArgument(request.params[10], UniValue::VNUM);
            depth = std::min(depth, request.params[10].get_int());
        }

        int64_t topContentId = 0;
        if (!topContentHash.empty())
        {
            auto ids = request.DbConnection()->WebRpcRepoInst->GetContentIds({topContentHash});
            if (!ids.empty())
                topContentId = ids[0];
        }

        auto reputationConsensus = ReputationConsensusFactoryInst.Instance(chainActive.Height());
        auto badReputationLimit = reputationConsensus->GetConsensusLimit(ConsensusLimit_bad_reputation);

        UniValue result(UniValue::VOBJ);
        UniValue content = request.DbConnection()->WebRpcRepoInst->GetTopFeed(
            countOut, topContentId, topHeight, lang, tags, contentTypes,
            txIdsExcluded, adrsExcluded, tagsExcluded,
            address, depth, badReputationLimit);

        result.pushKV("height", topHeight);
        result.pushKV("contents", content);
        return result;
    }

    UniValue GetMostCommentedFeed(const JSONRPCRequest& request)
    {
        if (request.fHelp)
            throw runtime_error(
                "GetMostCommentedFeed\n"
                "topHeight           (int) - ???\n"
                "topContentHash      (string, optional) - ???\n"
                "countOut            (int, optional) - ???\n"
                "lang                (string, optional) - ???\n"
                "tags                (vector<string>, optional) - ???\n"
                "contentTypes        (vector<int>, optional) - ???\n"
                "txIdsExcluded       (vector<string>, optional) - ???\n"
                "adrsExcluded        (vector<string>, optional) - ???\n"
                "tagsExcluded        (vector<string>, optional) - ???\n"
                "address             (string, optional) - ???\n"
                "depth               (int, optional) - ???\n"
            );

        int topHeight;
        string topContentHash;
        int countOut;
        string lang;
        vector<string> tags;
        vector<int> contentTypes;
        vector<string> txIdsExcluded;
        vector<string> adrsExcluded;
        vector<string> tagsExcluded;
        string address;
        int depth = 60 * 24 * 30 * 1; // about 1 month
        ParseFeedRequest(request, topHeight, topContentHash, countOut, lang, tags, contentTypes, txIdsExcluded,
            adrsExcluded, tagsExcluded, address);
        // depth
        if (request.params.size() > 10)
        {
            RPCTypeCheckArgument(request.params[10], UniValue::VNUM);
            depth = std::min(depth, request.params[10].get_int());
        }

        int64_t topContentId = 0;
        // if (!topContentHash.empty())
        // {
        //     auto ids = request.DbConnection()->WebRpcRepoInst->GetContentIds({topContentHash});
        //     if (!ids.empty())
        //         topContentId = ids[0];
        // }

        auto reputationConsensus = ReputationConsensusFactoryInst.Instance(chainActive.Height());
        auto badReputationLimit = reputationConsensus->GetConsensusLimit(ConsensusLimit_bad_reputation);

        UniValue result(UniValue::VOBJ);
        UniValue content = request.DbConnection()->WebRpcRepoInst->GetMostCommentedFeed(
            countOut, topContentId, topHeight, lang, tags, contentTypes,
            txIdsExcluded, adrsExcluded, tagsExcluded,
            address, depth, badReputationLimit);

        result.pushKV("height", topHeight);
        result.pushKV("contents", content);
        return result;
    }

    UniValue GetSubscribesFeed(const JSONRPCRequest& request)
    {
        if (request.fHelp)
            throw runtime_error(
                "GetSubscribesFeed\n"
                "topHeight           (int) - ???\n"
                "topContentHash      (string, optional) - ???\n"
                "countOut            (int, optional) - ???\n"
                "lang                (string, optional) - ???\n"
                "tags                (vector<string>, optional) - ???\n"
                "contentTypes        (vector<int>, optional) - ???\n"
                "txIdsExcluded       (vector<string>, optional) - ???\n"
                "adrsExcluded        (vector<string>, optional) - ???\n"
                "tagsExcluded        (vector<string>, optional) - ???\n"
                "address             (string, optional) - ???\n"
                "address_feed        (string) - ???\n"
                "addresses_extended  (vector<string>, optional) - ???\n"
            );

        int topHeight;
        string topContentHash;
        int countOut;
        string lang;
        vector<string> tags;
        vector<int> contentTypes;
        vector<string> txIdsExcluded;
        vector<string> adrsExcluded;
        vector<string> tagsExcluded;
        string address;
        string address_feed;
        vector<string> addresses_extended;
        ParseFeedRequest(request, topHeight, topContentHash, countOut, lang, tags, contentTypes, txIdsExcluded,
            adrsExcluded, tagsExcluded, address, address_feed, addresses_extended);

        if (address_feed.empty() && addresses_extended.empty())
            throw JSONRPCError(RPC_INVALID_REQUEST, string("No profile or addresses_extended addresses"));

        int64_t topContentId = 0;
        if (!topContentHash.empty())
        {
            auto ids = request.DbConnection()->WebRpcRepoInst->GetContentIds({topContentHash});
            if (!ids.empty())
                topContentId = ids[0];
        }

        UniValue result(UniValue::VOBJ);
        UniValue content = request.DbConnection()->WebRpcRepoInst->GetSubscribesFeed(
            address_feed, countOut, topContentId, topHeight, lang, tags, contentTypes,
            txIdsExcluded, adrsExcluded, tagsExcluded, address, addresses_extended);

        result.pushKV("height", topHeight);
        result.pushKV("contents", content);
        return result;
    }

    // TODO (o1q): Remove this method when the client gui switches to new methods
    UniValue FeedSelector(const JSONRPCRequest& request)
    {
        if (request.fHelp)
        {
            throw runtime_error(
                "feedselector\n"
                "\nOld method. Will be removed in future");
        }

        string addressFrom;
        if (request.params.size() > 0 && request.params[0].isStr())
            addressFrom = request.params[0].get_str();

        string addressTo = "";
        if (request.params.size() > 1 && request.params[1].isStr())
            addressTo = request.params[1].get_str();

        string topContentHash;
        if (request.params.size() > 2 && request.params[2].isStr())
            topContentHash = request.params[2].get_str();

        int count = 10;
        if (request.params.size() > 3 && request.params[3].isNum())
        {
            count = request.params[3].get_int();
            if (count > 10)
                count = 10;
        }

        string lang = "";
        if (request.params.size() > 4 && request.params[4].isStr())
            lang = request.params[4].get_str();

        vector<string> tags;
        if (request.params.size() > 5)
            ParseRequestTags(request.params[5], tags);

        // content types
        vector<int> contentTypes;
        ParseRequestContentTypes(request.params[6], contentTypes);

        int64_t topContentId = 0;
        if (!topContentHash.empty())
        {
            auto ids = request.DbConnection()->WebRpcRepoInst->GetContentIds({ topContentHash });
            if (!ids.empty())
                topContentId = ids[0];
        }

        if (addressTo == "1")
            //return GetSubscribesFeed(request);
            return request.DbConnection()->WebRpcRepoInst->GetSubscribesFeedOld(addressFrom, topContentId, count, lang, tags, contentTypes);

        //return GetProfileFeed(request);
        return request.DbConnection()->WebRpcRepoInst->GetProfileFeedOld(addressFrom, addressTo, topContentId, count, lang, tags, contentTypes);
    }

    UniValue GetContentsStatistic(const JSONRPCRequest& request)
    {
        if (request.fHelp)
            throw runtime_error(
                "getcontentsstatistic \"address\", \"contenttypes\", height, depth\n"
                "\nGet contents statistic.\n"
                "\nArguments:\n"
                "1. \"address\" (string) Address - сontent author\n"
                "2. \"contenttypes\" (string or array of strings, optional) type(s) of content posts/video\n"
                "3. \"height\"  (int, optional) Maximum content height. Default is current chain height\n"
                "4. \"depth\" (int, optional) Depth of content history for statistics. Default is all history\n"
            );

        string address;
        vector<string> addresses;
        if (request.params.size() > 0) {
            if (request.params[0].isStr()) {
                address = request.params[0].get_str();
                CTxDestination dest = DecodeDestination(address);
                if (!IsValidDestination(dest)) {
                    throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, string("Invalid Pocketnet address: ") + address);
                }
                addresses.emplace_back(address);
            } else if (request.params[0].isArray()) {
                UniValue addrs = request.params[0].get_array();
                if (addrs.size() > 10) {
                    throw JSONRPCError(RPC_INVALID_PARAMS, "Too large array");
                }
                if(addrs.size() > 0) {
                    for (unsigned int idx = 0; idx < addrs.size(); idx++) {
                        address = addrs[idx].get_str();
                        CTxDestination dest = DecodeDestination(address);
                        if (!IsValidDestination(dest)) {
                            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, string("Invalid Pocketnet address: ") + address);
                        }
                        addresses.emplace_back(address);
                    }
                }
            }
        }

        vector<int> contentTypes;
        ParseRequestContentTypes(request.params[1], contentTypes);

        return request.DbConnection()->WebRpcRepoInst->GetContentsStatistic(addresses, contentTypes);
    }

    UniValue GetRandomContents(const JSONRPCRequest& request)
    {
        if (request.fHelp)
        {
            UniValue help(UniValue::VOBJ);
            help.pushKV("Method", "GetRandomPost");

            UniValue args(UniValue::VARR);

            UniValue argLang(UniValue::VOBJ);
            argLang.pushKV("Name", "lang");
            argLang.pushKV("Type", "String");
            argLang.pushKV("Default", "en");
            args.push_back(argLang);

            help.pushKV("Arguments", args);

            UniValue examples(UniValue::VARR);
            help.pushKV("Examples", examples);
        }

        string lang = "en";
        if (request.params[0].isStr())
            lang = request.params[0].get_str();

        const int count = 1;
        const int height = chainActive.Height() - 150000;

        auto ids = request.DbConnection()->WebRpcRepoInst->GetRandomContentIds(lang, count, height);
        auto content = request.DbConnection()->WebRpcRepoInst->GetContentsData(ids, "");

        UniValue result(UniValue::VARR);
        result.push_backV(content);

        return result;
    }

    UniValue GetContentActions(const JSONRPCRequest& request)
    {
        if (request.fHelp)
            throw std::runtime_error(
                "getcontentactions\n"
                "\nGet profiles that performed actions(score/boos/donate) on content.\n");

        RPCTypeCheck(request.params, {UniValue::VSTR});

        auto contentHash = request.params[0].get_str();
        return request.DbConnection()->WebRpcRepoInst->GetContentActions(contentHash);
    }

    UniValue GetNotifications(const JSONRPCRequest& request)
    {
        if (request.fHelp)
            throw std::runtime_error(
                "getnotifications\n"
                "\nGet all possible notifications for all addresses for concrete block height.\n"
                "\nArguments:\n"
                "1. \"height\" (int) height of block to search in\n"
                "2. \"filters\" (array of strings, optional) type(s) of notifications. If empty or null - search for all types\n"
                );

        RPCTypeCheck(request.params, {UniValue::VNUM});

        auto height = request.params[0].get_int64();

        if (height > chainActive.Height()) throw JSONRPCError(RPC_INVALID_PARAMETER, "Spefified height is greater than current chain height");

        std::set<ShortTxType> filters;
        if (request.params.size() > 1 && request.params[1].isArray()) {
            const auto& rawFilters  = request.params[1].get_array();
            for (int i = 0; i < rawFilters.size(); i++) {
                if (rawFilters[i].isStr()) {
                    const auto& rawFilter = rawFilters[i].get_str();
                    auto filter = ShortTxTypeConvertor::strToType(rawFilter);
                    if (!ShortTxFilterValidator::Notifications::IsFilterAllowed(filter)) {
                        throw JSONRPCError(RPC_INVALID_PARAMETER, "Unexpected filter: " + rawFilter);
                    }
                    filters.insert(filter);
                }
            }
        }

        return request.DbConnection()->WebRpcRepoInst->GetNotifications(height, filters);
    }
}
