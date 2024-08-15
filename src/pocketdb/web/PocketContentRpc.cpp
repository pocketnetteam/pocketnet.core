// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include <pocketdb/consensus/Base.h>
#include "pocketdb/web/PocketContentRpc.h"
#include "rpc/util.h"
#include "validation.h"
#include "pocketdb/helpers/ShortFormModelsHelper.h"

namespace PocketWeb::PocketWebRpc
{
    void ParseFeedRequest(const JSONRPCRequest& request, int& topHeight, string& topContentHash, int& countOut, string& lang, vector<string>& tags,
        vector<int>& contentTypes, vector<string>& txIdsExcluded, vector<string>& adrsExcluded, vector<string>& tagsExcluded, string& address)
    {
        topHeight = ChainActiveSafeHeight();
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

        lang = "";
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

    RPCHelpMan GetContent()
    {
        return RPCHelpMan{"getcontent",
                "\nReturns contents for list of hash\n",
                {
                    {"hashes", RPCArg::Type::ARR, RPCArg::Optional::NO, "",
                        {
                            {"hash", RPCArg::Type::STR, RPCArg::Optional::NO, ""}   
                        }
                    },
                    {"address", RPCArg::Type::STR, RPCArg::Optional::NO, "Address (Default: \"\")"},
                    {"last", RPCArg::Type::BOOL, RPCArg::Optional::OMITTED_NAMED_ARG, "Get last version of content (Default: True)"},
                },
                {
                    // TODO (rpc): provide return description
                },
                RPCExamples{
                    HelpExampleCli("getcontent", "hashes[]") +
                    HelpExampleRpc("getcontent", "hashes[]")
                },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
    {
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

        bool last = true;
        if (request.params.size() > 2 && request.params[2].isNum())
            last = (request.params[2].get_int() == 1);

        vector<UniValue> content;

        if (last)
        {
            auto ids = request.DbConnection()->WebRpcRepoInst->GetContentIds(hashes);
            content = request.DbConnection()->WebRpcRepoInst->GetContentsData({}, ids, address);
        }
        else
        {
            content = request.DbConnection()->WebRpcRepoInst->GetContentsData(hashes, {}, address);
        }

        UniValue result(UniValue::VARR);
        result.push_backV(content);
        return result;
    },
        };
    }

    RPCHelpMan GetContents()
    {
        return RPCHelpMan{"getcontents",
                "\nReturns contents for address\n",
                {
                    {"address", RPCArg::Type::STR, RPCArg::Optional::NO, ""},
                },
                {
                    // TODO (rpc): provide return description
                },
                RPCExamples{
                    HelpExampleCli("getcontents", "address") +
                    HelpExampleRpc("getcontents", "address")
                },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
    {
        string address;
        if (request.params[0].isStr())
            address = request.params[0].get_str();

        return request.DbConnection()->WebRpcRepoInst->GetContentsForAddress(address);
    },
        };
    }

    RPCHelpMan GetProfileFeed()
    {
        return RPCHelpMan{"GetProfileFeed",
                "\n\n", // TODO (rpc)
                {
                    {"topHeight", RPCArg::Type::NUM, RPCArg::Optional::NO, "" /* TODO (rpc): arg description*/},
                    {"topContentHash", RPCArg::Type::STR, RPCArg::Optional::OMITTED_NAMED_ARG, "" /* TODO (rpc): arg description*/},
                    {"countOut", RPCArg::Type::NUM, RPCArg::Optional::OMITTED_NAMED_ARG, "" /* TODO (rpc): arg description*/},
                    {"lang", RPCArg::Type::STR, RPCArg::Optional::OMITTED_NAMED_ARG, "" /* TODO (rpc): arg description*/},
                    {"tags", RPCArg::Type::ARR, RPCArg::Optional::OMITTED_NAMED_ARG, "",
                        {
                            {"tag", RPCArg::Type::STR, RPCArg::Optional::NO, "" /* TODO (rpc): arg description*/}   
                        }
                    },
                    {"contentTypes", RPCArg::Type::ARR, RPCArg::Optional::OMITTED_NAMED_ARG, "",
                        {
                            {"contentType", RPCArg::Type::NUM, RPCArg::Optional::NO, "" /* TODO (rpc): arg description*/}   
                        }
                    },
                    {"txIdsExcluded", RPCArg::Type::ARR, RPCArg::Optional::OMITTED_NAMED_ARG, "",
                        {
                            {"txIdExcluded", RPCArg::Type::STR, RPCArg::Optional::NO, "" /* TODO (rpc): arg description*/}   
                        }
                    },
                    {"adrsExcluded", RPCArg::Type::ARR, RPCArg::Optional::OMITTED_NAMED_ARG, "",
                        {
                            {"adrExcluded", RPCArg::Type::STR, RPCArg::Optional::NO, "" /* TODO (rpc): arg description*/}   
                        }
                    },
                    {"tagsExcluded", RPCArg::Type::ARR, RPCArg::Optional::OMITTED_NAMED_ARG, "",
                        {
                            {"tagExcluded", RPCArg::Type::STR, RPCArg::Optional::NO, "" /* TODO (rpc): arg description*/}   
                        }
                    },
                    {"address", RPCArg::Type::STR, RPCArg::Optional::OMITTED_NAMED_ARG, "" /* TODO (rpc): arg description*/},
                    {"address_feed", RPCArg::Type::STR, RPCArg::Optional::NO, "" /* TODO (rpc): arg description*/},
                    {"keyword", RPCArg::Type::STR, RPCArg::Optional::NO, "" /* TODO (rpc): arg description*/},
                    {"orderby", RPCArg::Type::STR, RPCArg::Optional::NO, "" /* TODO (rpc): arg description*/},
                    {"ascdesc", RPCArg::Type::STR, RPCArg::Optional::NO, "" /* TODO (rpc): arg description*/},
                },
                {
                    // TODO (rpc): provide return description
                },
                RPCExamples{
                    // TODO (rpc): better examples
                    HelpExampleCli("getprofilefeed", "...") +
                    HelpExampleRpc("getprofilefeed", "...")
                },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
    {
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
        }
        else if (topContentHash.empty() && request.params.size() > 1 && request.params[1].isNum())
        {
            pageNumber = request.params[1].get_int();
        }

        UniValue result(UniValue::VOBJ);
        UniValue content = request.DbConnection()->WebRpcRepoInst->GetProfileFeed(
            address_feed, countOut, pageNumber, topContentId, topHeight, lang, tags, contentTypes,
            txIdsExcluded, adrsExcluded, tagsExcluded, address, orderby, ascdesc);

        result.pushKV("height", topHeight);
        result.pushKV("contents", content);
        return result;
    },
        };
    }

    RPCHelpMan GetProfileCollections()
    {
        return RPCHelpMan{"GetProfileCollections",
                          "\n\n", // TODO (rpc)
                          {
                                  {"topHeight", RPCArg::Type::NUM, RPCArg::Optional::NO, "" /* TODO (rpc): arg description*/},
                                  {"topContentHash", RPCArg::Type::STR, RPCArg::Optional::OMITTED_NAMED_ARG, "" /* TODO (rpc): arg description*/},
                                  {"countOut", RPCArg::Type::NUM, RPCArg::Optional::OMITTED_NAMED_ARG, "" /* TODO (rpc): arg description*/},
                                  {"lang", RPCArg::Type::STR, RPCArg::Optional::OMITTED_NAMED_ARG, "" /* TODO (rpc): arg description*/},
                                  {"tags", RPCArg::Type::ARR, RPCArg::Optional::OMITTED_NAMED_ARG, "",
                                   {
                                           {"tag", RPCArg::Type::STR, RPCArg::Optional::NO, "" /* TODO (rpc): arg description*/}
                                   }
                                  },
                                  {"contentTypes", RPCArg::Type::ARR, RPCArg::Optional::OMITTED_NAMED_ARG, "",
                                   {
                                           {"contentType", RPCArg::Type::NUM, RPCArg::Optional::NO, "" /* TODO (rpc): arg description*/}
                                   }
                                  },
                                  {"txIdsExcluded", RPCArg::Type::ARR, RPCArg::Optional::OMITTED_NAMED_ARG, "",
                                   {
                                           {"txIdExcluded", RPCArg::Type::STR, RPCArg::Optional::NO, "" /* TODO (rpc): arg description*/}
                                   }
                                  },
                                  {"adrsExcluded", RPCArg::Type::ARR, RPCArg::Optional::OMITTED_NAMED_ARG, "",
                                   {
                                           {"adrExcluded", RPCArg::Type::STR, RPCArg::Optional::NO, "" /* TODO (rpc): arg description*/}
                                   }
                                  },
                                  {"tagsExcluded", RPCArg::Type::ARR, RPCArg::Optional::OMITTED_NAMED_ARG, "",
                                   {
                                           {"tagExcluded", RPCArg::Type::STR, RPCArg::Optional::NO, "" /* TODO (rpc): arg description*/}
                                   }
                                  },
                                  {"address", RPCArg::Type::STR, RPCArg::Optional::OMITTED_NAMED_ARG, "" /* TODO (rpc): arg description*/},
                                  {"address_feed", RPCArg::Type::STR, RPCArg::Optional::NO, "" /* TODO (rpc): arg description*/},
                                  {"keyword", RPCArg::Type::STR, RPCArg::Optional::NO, "" /* TODO (rpc): arg description*/},
                                  {"orderby", RPCArg::Type::STR, RPCArg::Optional::NO, "" /* TODO (rpc): arg description*/},
                                  {"ascdesc", RPCArg::Type::STR, RPCArg::Optional::NO, "" /* TODO (rpc): arg description*/},
                          },
                          {
                                  // TODO (rpc): provide return description
                          },
                          RPCExamples{
                                  // TODO (rpc): better examples
                                  HelpExampleCli("getprofilecollections", "...") +
                                  HelpExampleRpc("getprofilecollections", "...")
                          },
                          [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
                          {
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
                              UniValue content = request.DbConnection()->WebRpcRepoInst->GetProfileCollections(
                                      address_feed, countOut, pageNumber, topContentId, topHeight, lang, tags, contentTypes,
                                      txIdsExcluded, adrsExcluded, tagsExcluded, address, keyword, orderby, ascdesc);

                              result.pushKV("height", topHeight);
                              result.pushKV("contents", content);
                              return result;
                          },
        };
    }

    RPCHelpMan GetHotPosts()
    {
        return RPCHelpMan{"GetHotPosts",
                "\n\n", // TODO (rpc)
                {
                    // TODO (rpc): args description
                },
                {
                    // TODO (rpc): provide return description
                },
                RPCExamples{
                    "" // TODO (rpc): usage examples
                },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
    {
        // if (request.fHelp)
        //     throw runtime_error(
        //         "GetHotPosts\n");

        int count = 30;
        if (request.params.size() > 0)
        {
            if (request.params[0].isNum())
                count = request.params[0].get_int();
            else if (request.params[0].isStr() && !request.params[0].empty())
                if (!ParseInt32(request.params[0].get_str(), &count))
                    throw JSONRPCError(RPC_INVALID_PARAMETER, "Failed to parse int from string");
        }

        // Depth in blocks (default about 3 days)
        int dayInBlocks = 24 * 60;
        int depthBlocks = 3 * dayInBlocks;
        if (request.params.size() > 1)
        {
            if (request.params[1].isNum())
                depthBlocks = request.params[1].get_int();
            else if (request.params[1].isStr() && !request.params[1].empty())
                if (!ParseInt32(request.params[1].get_str(), &depthBlocks))
                    throw JSONRPCError(RPC_INVALID_PARAMETER, "Failed to parse int from string");

            // for old version electron
            if (depthBlocks == 259200)
                depthBlocks = 3 * dayInBlocks;

            // Max 3 months
            depthBlocks = min(depthBlocks, 90 * dayInBlocks);
        }

        int nHeightOffset = ChainActiveSafeHeight();
        int nOffset = 0;
        if (request.params.size() > 2)
        {
            if (request.params[2].isNum())
            {
                if (request.params[2].get_int() > 0)
                    nOffset = request.params[2].get_int();
            }
            else if (request.params[2].isStr() && !request.params[2].empty())
            {
                if (!ParseInt32(request.params[2].get_str(), &nOffset))
                    throw JSONRPCError(RPC_INVALID_PARAMETER, "Failed to parse int from string");
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

        auto reputationConsensus = ConsensusFactoryInst_Reputation.Instance(ChainActiveSafeHeight());
        auto badReputationLimit = reputationConsensus->GetConsensusLimit(ConsensusLimit_bad_reputation);

        return request.DbConnection()->WebRpcRepoInst->GetHotPosts(count, depthBlocks, nHeightOffset, lang,
            contentTypes, address, badReputationLimit);
    },
        };
    }

    RPCHelpMan GetHistoricalFeed()
    {
        return RPCHelpMan{"GetHistoricalFeed",
                "\n\n", // TODO (rpc)
                {
                    {"topHeight", RPCArg::Type::NUM, RPCArg::Optional::NO, "" /* TODO (rpc): arg description*/},
                    {"topContentHash", RPCArg::Type::STR, RPCArg::Optional::OMITTED_NAMED_ARG, "" /* TODO (rpc): arg description*/},
                    {"countOut", RPCArg::Type::NUM, RPCArg::Optional::OMITTED_NAMED_ARG, "" /* TODO (rpc): arg description*/},
                    {"lang", RPCArg::Type::STR, RPCArg::Optional::OMITTED_NAMED_ARG, "" /* TODO (rpc): arg description*/},
                    {"tags", RPCArg::Type::ARR, RPCArg::Optional::OMITTED_NAMED_ARG, "",
                        {
                            {"tag", RPCArg::Type::STR, RPCArg::Optional::NO, "" /* TODO (rpc): arg description*/}   
                        }
                    },
                    {"contentTypes", RPCArg::Type::ARR, RPCArg::Optional::OMITTED_NAMED_ARG, "",
                        {
                            {"contentType", RPCArg::Type::NUM, RPCArg::Optional::NO, "" /* TODO (rpc): arg description*/}   
                        }
                    },
                    {"txIdsExcluded", RPCArg::Type::ARR, RPCArg::Optional::OMITTED_NAMED_ARG, "",
                        {
                            {"txIdExcluded", RPCArg::Type::STR, RPCArg::Optional::NO, "" /* TODO (rpc): arg description*/}   
                        }
                    },
                    {"adrsExcluded", RPCArg::Type::ARR, RPCArg::Optional::OMITTED_NAMED_ARG, "",
                        {
                            {"adrExcluded", RPCArg::Type::STR, RPCArg::Optional::NO, "" /* TODO (rpc): arg description*/}   
                        }
                    },
                    {"tagsExcluded", RPCArg::Type::ARR, RPCArg::Optional::OMITTED_NAMED_ARG, "",
                        {
                            {"tagExcluded", RPCArg::Type::STR, RPCArg::Optional::NO, "" /* TODO (rpc): arg description*/}   
                        }
                    },
                    {"address", RPCArg::Type::STR, RPCArg::Optional::OMITTED_NAMED_ARG, "" /* TODO (rpc): arg description*/},
                },
                {
                    // TODO (rpc): provide return description
                },
                RPCExamples{
                    // TODO (rpc): better examples
                    HelpExampleCli("gethistoricalfeed", "...") +
                    HelpExampleRpc("gethistoricalfeed", "...")
                },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
    {
        // if (request.fHelp)
        //     throw runtime_error(
        //         "GetHistoricalFeed\n"
        //         "topHeight           (int) - ???\n"
        //         "topContentHash      (string, optional) - ???\n"
        //         "countOut            (int, optional) - ???\n"
        //         "lang                (string, optional) - ???\n"
        //         "tags                (vector<string>, optional) - ???\n"
        //         "contentTypes        (vector<int>, optional) - ???\n"
        //         "txIdsExcluded       (vector<string>, optional) - ???\n"
        //         "adrsExcluded        (vector<string>, optional) - ???\n"
        //         "tagsExcluded        (vector<string>, optional) - ???\n"
        //         "address             (string, optional) - ???\n"
        //     );

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

        auto reputationConsensus = ConsensusFactoryInst_Reputation.Instance(ChainActiveSafeHeight());
        auto badReputationLimit = reputationConsensus->GetConsensusLimit(ConsensusLimit_bad_reputation);

        UniValue result(UniValue::VOBJ);
        UniValue content = request.DbConnection()->WebRpcRepoInst->GetHistoricalFeed(
            countOut, topContentId, topHeight, lang, tags, contentTypes,
            txIdsExcluded, adrsExcluded, tagsExcluded,
            address, badReputationLimit);

        result.pushKV("height", topHeight);
        result.pushKV("contents", content);
        return result;
    },
        };
    }

    RPCHelpMan GetHierarchicalFeed()
    {
        return RPCHelpMan{"GetHierarchicalFeed",
                "\n\n", // TODO (rpc)
                {
                    {"topHeight", RPCArg::Type::NUM, RPCArg::Optional::NO, "" /* TODO (rpc): arg description*/},
                    {"topContentHash", RPCArg::Type::STR, RPCArg::Optional::OMITTED_NAMED_ARG, "" /* TODO (rpc): arg description*/},
                    {"countOut", RPCArg::Type::NUM, RPCArg::Optional::OMITTED_NAMED_ARG, "" /* TODO (rpc): arg description*/},
                    {"lang", RPCArg::Type::STR, RPCArg::Optional::OMITTED_NAMED_ARG, "" /* TODO (rpc): arg description*/},
                    {"tags", RPCArg::Type::ARR, RPCArg::Optional::OMITTED_NAMED_ARG, "",
                        {
                            {"tag", RPCArg::Type::STR, RPCArg::Optional::NO, "" /* TODO (rpc): arg description*/}   
                        }
                    },
                    {"contentTypes", RPCArg::Type::ARR, RPCArg::Optional::OMITTED_NAMED_ARG, "",
                        {
                            {"contentType", RPCArg::Type::NUM, RPCArg::Optional::NO, "" /* TODO (rpc): arg description*/}   
                        }
                    },
                    {"txIdsExcluded", RPCArg::Type::ARR, RPCArg::Optional::OMITTED_NAMED_ARG, "",
                        {
                            {"txIdExcluded", RPCArg::Type::STR, RPCArg::Optional::NO, "" /* TODO (rpc): arg description*/}   
                        }
                    },
                    {"adrsExcluded", RPCArg::Type::ARR, RPCArg::Optional::OMITTED_NAMED_ARG, "",
                        {
                            {"adrExcluded", RPCArg::Type::STR, RPCArg::Optional::NO, "" /* TODO (rpc): arg description*/}   
                        }
                    },
                    {"tagsExcluded", RPCArg::Type::ARR, RPCArg::Optional::OMITTED_NAMED_ARG, "",
                        {
                            {"tagExcluded", RPCArg::Type::STR, RPCArg::Optional::NO, "" /* TODO (rpc): arg description*/}   
                        }
                    },
                    {"address", RPCArg::Type::STR, RPCArg::Optional::OMITTED_NAMED_ARG, "" /* TODO (rpc): arg description*/},
                },
                {
                    // TODO (rpc): provide return description
                },
                RPCExamples{
                    // TODO (rpc): better examples
                    HelpExampleCli("gethierarchicalfeed", "...") +
                    HelpExampleRpc("gethierarchicalfeed", "...")
                },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
    {
        // if (request.fHelp)
        //     throw runtime_error(
        //         "GetHierarchicalFeed\n"
        //         "topHeight           (int) - ???\n"
        //         "topContentHash      (string, optional) - ???\n"
        //         "countOut            (int, optional) - ???\n"
        //         "lang                (string, optional) - ???\n"
        //         "tags                (vector<string>, optional) - ???\n"
        //         "contentTypes        (vector<int>, optional) - ???\n"
        //         "txIdsExcluded       (vector<string>, optional) - ???\n"
        //         "adrsExcluded        (vector<string>, optional) - ???\n"
        //         "tagsExcluded        (vector<string>, optional) - ???\n"
        //         "address             (string, optional) - ???\n"
        //     );

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

        auto reputationConsensus = ConsensusFactoryInst_Reputation.Instance(ChainActiveSafeHeight());
        auto badReputationLimit = reputationConsensus->GetConsensusLimit(ConsensusLimit_bad_reputation);

        UniValue result(UniValue::VOBJ);
        UniValue content = request.DbConnection()->WebRpcRepoInst->GetHierarchicalFeed(
            countOut, topContentId, topHeight, lang, tags, contentTypes,
            txIdsExcluded, adrsExcluded, tagsExcluded,
            address, badReputationLimit);

        result.pushKV("height", topHeight);
        result.pushKV("contents", content);
        return result;
    },
        };
    }

    RPCHelpMan GetBoostFeed()
    {
        return RPCHelpMan{"getboostfeed",
                "\n\n",
                {
                    {"topHeight", RPCArg::Type::NUM, RPCArg::Optional::NO, "" /* TODO (rpc): arg description*/},
                    {"topContentHash", RPCArg::Type::STR, RPCArg::Optional::OMITTED_NAMED_ARG, "" /* TODO (rpc): arg description*/},
                    {"countOut", RPCArg::Type::NUM, RPCArg::Optional::OMITTED_NAMED_ARG, "" /* TODO (rpc): arg description*/},
                    {"lang", RPCArg::Type::STR, RPCArg::Optional::OMITTED_NAMED_ARG, "" /* TODO (rpc): arg description*/},
                    {"tags", RPCArg::Type::ARR, RPCArg::Optional::OMITTED_NAMED_ARG, "",
                        {
                            {"tag", RPCArg::Type::STR, RPCArg::Optional::NO, "" /* TODO (rpc): arg description*/}   
                        }
                    },
                    {"contentTypes", RPCArg::Type::ARR, RPCArg::Optional::OMITTED_NAMED_ARG, "",
                        {
                            {"contentType", RPCArg::Type::NUM, RPCArg::Optional::NO, "" /* TODO (rpc): arg description*/}   
                        }
                    },
                    {"txIdsExcluded", RPCArg::Type::ARR, RPCArg::Optional::OMITTED_NAMED_ARG, "",
                        {
                            {"txIdExcluded", RPCArg::Type::STR, RPCArg::Optional::NO, "" /* TODO (rpc): arg description*/}   
                        }
                    },
                    {"adrsExcluded", RPCArg::Type::ARR, RPCArg::Optional::OMITTED_NAMED_ARG, "",
                        {
                            {"adrExcluded", RPCArg::Type::STR, RPCArg::Optional::NO, "" /* TODO (rpc): arg description*/}   
                        }
                    },
                    {"tagsExcluded", RPCArg::Type::ARR, RPCArg::Optional::OMITTED_NAMED_ARG, "",
                        {
                            {"tagExcluded", RPCArg::Type::STR, RPCArg::Optional::NO, "" /* TODO (rpc): arg description*/}   
                        }
                    },
                },
                {
                    // TODO (rpc): provide return description
                },
                RPCExamples{
                    // TODO (rpc): better examples
                    HelpExampleCli("getboostfeed", "...") +
                    HelpExampleRpc("getboostfeed", "...")
                },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
    {
        // if (request.fHelp)
        //     throw runtime_error(
        //         "GetHierarchicalFeed\n"
        //         "topHeight           (int) - ???\n"
        //         "topContentHash      (string, not supported) - ???\n"
        //         "countOut            (int, not supported) - ???\n"
        //         "lang                (string, optional) - ???\n"
        //         "tags                (vector<string>, optional) - ???\n"
        //         "contentTypes        (vector<int>, optional) - ???\n"
        //         "txIdsExcluded       (vector<string>, optional) - ???\n"
        //         "adrsExcluded        (vector<string>, optional) - ???\n"
        //         "tagsExcluded        (vector<string>, optional) - ???\n"
        //     );

        int topHeight;
        string lang;
        vector<string> tags;
        vector<int> contentTypes;
        vector<string> txIdsExcluded;
        vector<string> adrsExcluded;
        vector<string> tagsExcluded;

        string skipString = "";
        int countOut =  300;
        ParseFeedRequest(request, topHeight, skipString, countOut, lang, tags, contentTypes, txIdsExcluded,
            adrsExcluded, tagsExcluded, skipString);

        auto reputationConsensus = ConsensusFactoryInst_Reputation.Instance(ChainActiveSafeHeight());
        auto badReputationLimit = reputationConsensus->GetConsensusLimit(ConsensusLimit_bad_reputation);

        UniValue result(UniValue::VOBJ);
        UniValue boosts = request.DbConnection()->WebRpcRepoInst->GetBoostFeed(
            topHeight, countOut, lang, tags, contentTypes,
            txIdsExcluded, adrsExcluded, tagsExcluded,
            badReputationLimit);

        result.pushKV("height", topHeight);
        result.pushKV("boosts", boosts);
        return result;
    },
        };
    }

    RPCHelpMan GetTopFeed()
    {
        return RPCHelpMan{"GetTopFeed",
                "\n\n", // TODO (rpc)
                {
                    {"topHeight", RPCArg::Type::NUM, RPCArg::Optional::NO, "" /* TODO (rpc): arg description*/},
                    {"topContentHash", RPCArg::Type::STR, RPCArg::Optional::OMITTED_NAMED_ARG, "" /* TODO (rpc): arg description*/},
                    {"countOut", RPCArg::Type::NUM, RPCArg::Optional::OMITTED_NAMED_ARG, "" /* TODO (rpc): arg description*/},
                    {"lang", RPCArg::Type::STR, RPCArg::Optional::OMITTED_NAMED_ARG, "" /* TODO (rpc): arg description*/},
                    {"tags", RPCArg::Type::ARR, RPCArg::Optional::OMITTED_NAMED_ARG, "",
                        {
                            {"tag", RPCArg::Type::STR, RPCArg::Optional::NO, "" /* TODO (rpc): arg description*/}   
                        }
                    },
                    {"contentTypes", RPCArg::Type::ARR, RPCArg::Optional::OMITTED_NAMED_ARG, "",
                        {
                            {"contentType", RPCArg::Type::NUM, RPCArg::Optional::NO, "" /* TODO (rpc): arg description*/}   
                        }
                    },
                    {"txIdsExcluded", RPCArg::Type::ARR, RPCArg::Optional::OMITTED_NAMED_ARG, "",
                        {
                            {"txIdExcluded", RPCArg::Type::STR, RPCArg::Optional::NO, "" /* TODO (rpc): arg description*/}   
                        }
                    },
                    {"adrsExcluded", RPCArg::Type::ARR, RPCArg::Optional::OMITTED_NAMED_ARG, "",
                        {
                            {"adrExcluded", RPCArg::Type::STR, RPCArg::Optional::NO, "" /* TODO (rpc): arg description*/}   
                        }
                    },
                    {"tagsExcluded", RPCArg::Type::ARR, RPCArg::Optional::OMITTED_NAMED_ARG, "",
                        {
                            {"tagExcluded", RPCArg::Type::STR, RPCArg::Optional::NO, "" /* TODO (rpc): arg description*/}   
                        }
                    },
                    {"address", RPCArg::Type::STR, RPCArg::Optional::OMITTED_NAMED_ARG, "" /*TODO (rpc): arg description*/},
                    {"depth", RPCArg::Type::NUM, RPCArg::Optional::OMITTED_NAMED_ARG, "" /*TODO (rpc): arg description*/}
                },
                {
                    // TODO (rpc): provide return description
                },
                RPCExamples{
                    // TODO (rpc): better examples
                    HelpExampleCli("gettopfeed", "...") +
                    HelpExampleRpc("gettopfeed", "...")
                },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
    {
        // if (request.fHelp)
        //     throw runtime_error(
        //         "GetTopFeed\n"
        //         "topHeight           (int) - ???\n"
        //         "topContentHash      (string, optional) - ???\n"
        //         "countOut            (int, optional) - ???\n"
        //         "lang                (string, optional) - ???\n"
        //         "tags                (vector<string>, optional) - ???\n"
        //         "contentTypes        (vector<int>, optional) - ???\n"
        //         "txIdsExcluded       (vector<string>, optional) - ???\n"
        //         "adrsExcluded        (vector<string>, optional) - ???\n"
        //         "tagsExcluded        (vector<string>, optional) - ???\n"
        //         "address             (string, optional) - ???\n"
        //         "depth               (int, optional) - ???\n"
        //     );

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

        auto reputationConsensus = ConsensusFactoryInst_Reputation.Instance(ChainActiveSafeHeight());
        auto badReputationLimit = reputationConsensus->GetConsensusLimit(ConsensusLimit_bad_reputation);

        UniValue result(UniValue::VOBJ);
        UniValue content = request.DbConnection()->WebRpcRepoInst->GetTopFeed(
            countOut, topContentId, topHeight, lang, tags, contentTypes,
            txIdsExcluded, adrsExcluded, tagsExcluded,
            address, depth, badReputationLimit);

        result.pushKV("height", topHeight);
        result.pushKV("contents", content);
        return result;
    },
        };
    }

    RPCHelpMan GetMostCommentedFeed()
    {
        return RPCHelpMan{"GetMostCommentedFeed",
                "\n\n", // TODO (rpc)
                {
                    {"topHeight", RPCArg::Type::NUM, RPCArg::Optional::NO, "" /* TODO (rpc): arg description*/},
                    {"topContentHash", RPCArg::Type::STR, RPCArg::Optional::OMITTED_NAMED_ARG, "" /* TODO (rpc): arg description*/},
                    {"countOut", RPCArg::Type::NUM, RPCArg::Optional::OMITTED_NAMED_ARG, "" /* TODO (rpc): arg description*/},
                    {"lang", RPCArg::Type::STR, RPCArg::Optional::OMITTED_NAMED_ARG, "" /* TODO (rpc): arg description*/},
                    {"tags", RPCArg::Type::ARR, RPCArg::Optional::OMITTED_NAMED_ARG, "",
                        {
                            {"tag", RPCArg::Type::STR, RPCArg::Optional::NO, "" /* TODO (rpc): arg description*/}   
                        }
                    },
                    {"contentTypes", RPCArg::Type::ARR, RPCArg::Optional::OMITTED_NAMED_ARG, "",
                        {
                            {"contentType", RPCArg::Type::NUM, RPCArg::Optional::NO, "" /* TODO (rpc): arg description*/}   
                        }
                    },
                    {"txIdsExcluded", RPCArg::Type::ARR, RPCArg::Optional::OMITTED_NAMED_ARG, "",
                        {
                            {"txIdExcluded", RPCArg::Type::STR, RPCArg::Optional::NO, "" /* TODO (rpc): arg description*/}   
                        }
                    },
                    {"adrsExcluded", RPCArg::Type::ARR, RPCArg::Optional::OMITTED_NAMED_ARG, "",
                        {
                            {"adrExcluded", RPCArg::Type::STR, RPCArg::Optional::NO, "" /* TODO (rpc): arg description*/}   
                        }
                    },
                    {"tagsExcluded", RPCArg::Type::ARR, RPCArg::Optional::OMITTED_NAMED_ARG, "",
                        {
                            {"tagExcluded", RPCArg::Type::STR, RPCArg::Optional::NO, "" /* TODO (rpc): arg description*/}   
                        }
                    },
                    {"address", RPCArg::Type::STR, RPCArg::Optional::OMITTED_NAMED_ARG, "" /*TODO (rpc): arg description*/},
                    {"depth", RPCArg::Type::NUM, RPCArg::Optional::OMITTED_NAMED_ARG, "" /*TODO (rpc): arg description*/}
                },
                {
                    // TODO (rpc): provide return description
                },
                RPCExamples{
                    // TODO (rpc): better examples
                    HelpExampleCli("getmostcommentedfeed", "...") +
                    HelpExampleRpc("getmostcommentedfeed", "...")
                },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
    {
        // if (request.fHelp)
        //     throw runtime_error(
        //         "GetMostCommentedFeed\n"
        //         "topHeight           (int) - ???\n"
        //         "topContentHash      (string, optional) - ???\n"
        //         "countOut            (int, optional) - ???\n"
        //         "lang                (string, optional) - ???\n"
        //         "tags                (vector<string>, optional) - ???\n"
        //         "contentTypes        (vector<int>, optional) - ???\n"
        //         "txIdsExcluded       (vector<string>, optional) - ???\n"
        //         "adrsExcluded        (vector<string>, optional) - ???\n"
        //         "tagsExcluded        (vector<string>, optional) - ???\n"
        //         "address             (string, optional) - ???\n"
        //         "depth               (int, optional) - ???\n"
        //     );

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

        auto reputationConsensus = ConsensusFactoryInst_Reputation.Instance(ChainActiveSafeHeight());
        auto badReputationLimit = reputationConsensus->GetConsensusLimit(ConsensusLimit_bad_reputation);

        UniValue result(UniValue::VOBJ);
        UniValue content = request.DbConnection()->WebRpcRepoInst->GetMostCommentedFeed(
            countOut, topContentId, topHeight, lang, tags, contentTypes,
            txIdsExcluded, adrsExcluded, tagsExcluded,
            address, depth, badReputationLimit);

        result.pushKV("height", topHeight);
        result.pushKV("contents", content);
        return result;
    },
        };
    }

    RPCHelpMan GetSubscribesFeed()
    {
        return RPCHelpMan{"GetSubscribesFeed",
                "\n\n", // TODO (rpc)
                {
                    {"topHeight", RPCArg::Type::NUM, RPCArg::Optional::NO, "" /* TODO (rpc): arg description*/},
                    {"topContentHash", RPCArg::Type::STR, RPCArg::Optional::OMITTED_NAMED_ARG, "" /* TODO (rpc): arg description*/},
                    {"countOut", RPCArg::Type::NUM, RPCArg::Optional::OMITTED_NAMED_ARG, "" /* TODO (rpc): arg description*/},
                    {"lang", RPCArg::Type::STR, RPCArg::Optional::OMITTED_NAMED_ARG, "" /* TODO (rpc): arg description*/},
                    {"tags", RPCArg::Type::ARR, RPCArg::Optional::OMITTED_NAMED_ARG, "",
                        {
                            {"tag", RPCArg::Type::STR, RPCArg::Optional::NO, "" /* TODO (rpc): arg description*/}   
                        }
                    },
                    {"contentTypes", RPCArg::Type::ARR, RPCArg::Optional::OMITTED_NAMED_ARG, "",
                        {
                            {"contentType", RPCArg::Type::NUM, RPCArg::Optional::NO, "" /* TODO (rpc): arg description*/}   
                        }
                    },
                    {"txIdsExcluded", RPCArg::Type::ARR, RPCArg::Optional::OMITTED_NAMED_ARG, "",
                        {
                            {"txIdExcluded", RPCArg::Type::STR, RPCArg::Optional::NO, "" /* TODO (rpc): arg description*/}   
                        }
                    },
                    {"adrsExcluded", RPCArg::Type::ARR, RPCArg::Optional::OMITTED_NAMED_ARG, "",
                        {
                            {"adrExcluded", RPCArg::Type::STR, RPCArg::Optional::NO, "" /* TODO (rpc): arg description*/}   
                        }
                    },
                    {"tagsExcluded", RPCArg::Type::ARR, RPCArg::Optional::OMITTED_NAMED_ARG, "",
                        {
                            {"tagExcluded", RPCArg::Type::STR, RPCArg::Optional::NO, "" /* TODO (rpc): arg description*/}   
                        }
                    },
                    {"address", RPCArg::Type::STR, RPCArg::Optional::OMITTED_NAMED_ARG, "" /*TODO (rpc): arg description*/},
                    {"address_feed", RPCArg::Type::STR, RPCArg::Optional::OMITTED_NAMED_ARG, "" /*TODO (rpc): arg description*/},
                    {"addresses_extended", RPCArg::Type::ARR, RPCArg::Optional::OMITTED_NAMED_ARG, "",
                        {
                            {"address_extended", RPCArg::Type::STR, RPCArg::Optional::NO, "" /* TODO (rpc): arg description*/}   
                        }
                    }
                },
                {
                    // TODO (rpc): provide return description
                },
                RPCExamples{
                    // TODO (rpc): better examples
                    HelpExampleCli("getsubscribesfeed", "...") +
                    HelpExampleRpc("getsubscribesfeed", "...")
                },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
    {
        // if (request.fHelp)
        //     throw runtime_error(
        //         "GetSubscribesFeed\n"
        //         "topHeight           (int) - ???\n"
        //         "topContentHash      (string, optional) - ???\n"
        //         "countOut            (int, optional) - ???\n"
        //         "lang                (string, optional) - ???\n"
        //         "tags                (vector<string>, optional) - ???\n"
        //         "contentTypes        (vector<int>, optional) - ???\n"
        //         "txIdsExcluded       (vector<string>, optional) - ???\n"
        //         "adrsExcluded        (vector<string>, optional) - ???\n"
        //         "tagsExcluded        (vector<string>, optional) - ???\n"
        //         "address             (string, optional) - ???\n"
        //         "address_feed        (string) - ???\n"
        //         "addresses_extended  (vector<string>, optional) - ???\n"
        //     );

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
    },
        };
    }

    RPCHelpMan GetContentsStatistic()
    {
        return RPCHelpMan{"getcontentsstatistic",
                "\nGet contents statistic.\n",
                {
                    {"address", RPCArg::Type::STR, RPCArg::Optional::NO, "сontent author"},
                    {"contentTypes", RPCArg::Type::ARR, RPCArg::Optional::OMITTED_NAMED_ARG, "type(s) of content posts/video",
                        {
                            {"contentType", RPCArg::Type::NUM, RPCArg::Optional::NO, ""}   
                        }
                    },
                    {"height", RPCArg::Type::NUM, RPCArg::Optional::OMITTED_NAMED_ARG, "Maximum content height. Default is current chain height"},
                    {"depth", RPCArg::Type::NUM, RPCArg::Optional::OMITTED_NAMED_ARG, "Depth of content history for statistics. Default is all history"}
                },
                {
                    // TODO (rpc): provide return description
                },
                RPCExamples{
                    // TODO (rpc): better examples
                    HelpExampleCli("getcontentsstatistic", "\"address\", \"contenttypes\", height, depth\n") +
                    HelpExampleRpc("getcontentsstatistic", "\"address\", \"contenttypes\", height, depth\n")
                },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
    {
        // if (request.fHelp)
        //     throw runtime_error(
        //         "getcontentsstatistic \"address\", \"contenttypes\", height, depth\n"
        //         "\nGet contents statistic.\n"
        //         "\nArguments:\n"
        //         "1. \"address\" (string) Address - сontent author\n"
        //         "2. \"contenttypes\" (string or array of strings, optional) type(s) of content posts/video\n"
        //         "3. \"height\"  (int, optional) Maximum content height. Default is current chain height\n"
        //         "4. \"depth\" (int, optional) Depth of content history for statistics. Default is all history\n"
        //     );

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
    },
        };
    }

    RPCHelpMan GetRandomContents()
    {
        return RPCHelpMan{"GetRandomPost",
                "\nGet contents statistic.\n",
                {
                    // TODO (rpc): args description
                },
                {
                    // TODO (rpc): provide return description
                },
                RPCExamples{
                    // TODO (rpc): better examples
                    HelpExampleCli("getrandompost", "") +
                    HelpExampleRpc("getrandompost", "")
                },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
    {
        // if (request.fHelp)
        // {
        //     UniValue help(UniValue::VOBJ);
        //     help.pushKV("Method", "GetRandomPost");

        //     UniValue args(UniValue::VARR);

        //     UniValue argLang(UniValue::VOBJ);
        //     argLang.pushKV("Name", "lang");
        //     argLang.pushKV("Type", "String");
        //     argLang.pushKV("Default", "en");
        //     args.push_back(argLang);

        //     help.pushKV("Arguments", args);

        //     UniValue examples(UniValue::VARR);
        //     help.pushKV("Examples", examples);
        // }

        string lang = "en";
        if (request.params[0].isStr())
            lang = request.params[0].get_str();

        const int count = 1;
        const int height = ChainActiveSafeHeight() - 150000;

        auto ids = request.DbConnection()->WebRpcRepoInst->GetRandomContentIds(lang, count, height);
        auto content = request.DbConnection()->WebRpcRepoInst->GetContentsData({}, ids, "");

        UniValue result(UniValue::VARR);
        result.push_backV(content);

        return result;
    },
        };
    }

    RPCHelpMan GetContentActions()
    {
        return RPCHelpMan{"getcontentactions",
                "\nGet profiles that performed actions(score/boos/donate) on content.\n",
                {
                    // TODO (rpc): args description
                },
                {
                    // TODO (rpc): provide return description
                },
                RPCExamples{
                    // TODO (rpc): better examples
                    HelpExampleCli("getcontentactions", "") +
                    HelpExampleRpc("getcontentactions", "")
                },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
    {
        // if (request.fHelp)
        //     throw std::runtime_error(
        //         "getcontentactions\n"
        //         "\nGet profiles that performed actions(score/boos/donate) on content.\n");

        RPCTypeCheck(request.params, {UniValue::VSTR});

        auto contentHash = request.params[0].get_str();
        return request.DbConnection()->WebRpcRepoInst->GetContentActions(contentHash);
    },
        };
    }

    RPCHelpMan GetEvents()
    {
        return RPCHelpMan{"GetEvents",
                        "\nGet all events assotiated with addresses. Search depth - 3 months\n",
                        {
                            {"address", RPCArg::Type::STR, RPCArg::Optional::NO, "address to get events for"},
                            {"heightMax", RPCArg::Type::NUM, RPCArg::Optional::OMITTED_NAMED_ARG, "max height to start search from, including. Default is current chain height"},
                            {"blockNum", RPCArg::Type::NUM, RPCArg::Optional::OMITTED_NAMED_ARG, "number of transaction in block to start search from for specified heightMax, excluding. Default is 999999"},
                            {"filters", RPCArg::Type::ARR, RPCArg::Optional::OMITTED_NAMED_ARG, "filters to specify event's types to search for. Default: search for all events",
                            {
                                {ShortTxTypeConvertor::toString(ShortTxType::Money), RPCArg::Type::STR, RPCArg::Optional::OMITTED_NAMED_ARG, "incoming money"},
                                {ShortTxTypeConvertor::toString(ShortTxType::Referal), RPCArg::Type::STR, RPCArg::Optional::OMITTED_NAMED_ARG, "registered referals"},
                                {ShortTxTypeConvertor::toString(ShortTxType::Answer), RPCArg::Type::STR, RPCArg::Optional::OMITTED_NAMED_ARG, "answers to acc's comments"},
                                {ShortTxTypeConvertor::toString(ShortTxType::Comment), RPCArg::Type::STR, RPCArg::Optional::OMITTED_NAMED_ARG, "comments to acc's content"},
                                {ShortTxTypeConvertor::toString(ShortTxType::Subscriber), RPCArg::Type::STR, RPCArg::Optional::OMITTED_NAMED_ARG, "subscribers"},
                                {ShortTxTypeConvertor::toString(ShortTxType::CommentScore), RPCArg::Type::STR, RPCArg::Optional::OMITTED_NAMED_ARG, "scores to acc's comments"},
                                {ShortTxTypeConvertor::toString(ShortTxType::ContentScore), RPCArg::Type::STR, RPCArg::Optional::OMITTED_NAMED_ARG, "scores to acc's content"},
                                {ShortTxTypeConvertor::toString(ShortTxType::PrivateContent), RPCArg::Type::STR, RPCArg::Optional::OMITTED_NAMED_ARG, "content from private subscriptions"},
                                {ShortTxTypeConvertor::toString(ShortTxType::Boost), RPCArg::Type::STR, RPCArg::Optional::OMITTED_NAMED_ARG, "boosts of acc's content"},
                                {ShortTxTypeConvertor::toString(ShortTxType::Repost), RPCArg::Type::STR, RPCArg::Optional::OMITTED_NAMED_ARG, "reposts of acc's content"},
                                {ShortTxTypeConvertor::toString(ShortTxType::JuryAssigned), RPCArg::Type::STR, RPCArg::Optional::OMITTED_NAMED_ARG, "Juries assigned to account"},
                                {ShortTxTypeConvertor::toString(ShortTxType::JuryModerate), RPCArg::Type::STR, RPCArg::Optional::OMITTED_NAMED_ARG, "Juries assigned to moderators"},
                                {ShortTxTypeConvertor::toString(ShortTxType::JuryVerdict), RPCArg::Type::STR, RPCArg::Optional::OMITTED_NAMED_ARG, "Juries results"},
                            }
                        }
                          },
                          {
                                  // TODO (rpc): provide return description
                          },
                          RPCExamples{
                                  // TODO (rpc)
                                  HelpExampleCli("getevents", "") +
                                  HelpExampleRpc("getevents", "")
                          },
    [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
    {
        RPCTypeCheck(request.params, {UniValue::VSTR});

        auto address = request.params[0].get_str();

        int64_t heightMax = ChainActiveSafeHeight();
        if (request.params.size() > 1 && request.params[1].isNum()) {
            heightMax = request.params[1].get_int64();
            if (heightMax > ChainActiveSafeHeight())
                heightMax = ChainActiveSafeHeight();
        }

        int64_t blockNum = 9999999; // Default
        if (request.params.size() > 2 && request.params[2].isNum()) {
            blockNum = request.params[2].get_int64();
        }

        static const int64_t depth = 129600; // 3 months
        int64_t heightMin = heightMax - depth;
        if (heightMin < 0) {
            heightMin = 0;
        }

        std::set<ShortTxType> filters;
        if (request.params.size() > 3 && request.params[3].isArray()) {
            const auto& rawFilters  = request.params[3].get_array();
            for (int i = 0; i < rawFilters.size(); i++) {
                if (rawFilters[i].isStr()) {
                    const auto& rawFilter = rawFilters[i].get_str();
                    auto filter = ShortTxTypeConvertor::strToType(rawFilter);
                    if (!PocketHelpers::ShortTxFilterValidator::Events::IsFilterAllowed(filter)) {
                        throw JSONRPCError(RPC_INVALID_PARAMETER, "Unexpected filter: " + rawFilter);
                    }
                    filters.insert(filter);
                }
            }
        }

        auto shortTxs = request.DbConnection()->NotifierRepoInst->GetEventsForAddresses(address, heightMax, heightMin, blockNum, filters);
        UniValue res(UniValue::VARR);
        for (const auto& tx: shortTxs) {
            res.push_back(tx.Serialize());
        }

        return res;
    },
        };
    }

    RPCHelpMan GetNotifications()
    {
       return RPCHelpMan{"getnotifications",
                "\nGet all possible notifications for all addresses for concrete block height.\n",
                {
                    {"height", RPCArg::Type::NUM, RPCArg::Optional::NO, "height of block to search in"},
                    {"filters", RPCArg::Type::ARR, RPCArg::Optional::OMITTED_NAMED_ARG, "type(s) of notifications. If empty or null - search for all types",
                        {
                            {ShortTxTypeConvertor::toString(ShortTxType::Money), RPCArg::Type::STR, RPCArg::Optional::OMITTED_NAMED_ARG, "new incoming coins"},
                            {ShortTxTypeConvertor::toString(ShortTxType::Answer), RPCArg::Type::STR, RPCArg::Optional::OMITTED_NAMED_ARG, "new answers to acc's comments"},
                            {ShortTxTypeConvertor::toString(ShortTxType::Boost), RPCArg::Type::STR, RPCArg::Optional::OMITTED_NAMED_ARG, "new boosts of acc's content"},
                            {ShortTxTypeConvertor::toString(ShortTxType::Referal), RPCArg::Type::STR, RPCArg::Optional::OMITTED_NAMED_ARG, "new registration with my referal link"},
                            {ShortTxTypeConvertor::toString(ShortTxType::Comment), RPCArg::Type::STR, RPCArg::Optional::OMITTED_NAMED_ARG, "new comments under acc's content"},
                            {ShortTxTypeConvertor::toString(ShortTxType::Subscriber), RPCArg::Type::STR, RPCArg::Optional::OMITTED_NAMED_ARG, "new subscribers to acc"},
                            {ShortTxTypeConvertor::toString(ShortTxType::CommentScore), RPCArg::Type::STR, RPCArg::Optional::OMITTED_NAMED_ARG, "new scores to acc's comments"},
                            {ShortTxTypeConvertor::toString(ShortTxType::ContentScore), RPCArg::Type::STR, RPCArg::Optional::OMITTED_NAMED_ARG, "new scores to acc's content"},
                            {ShortTxTypeConvertor::toString(ShortTxType::PrivateContent), RPCArg::Type::STR, RPCArg::Optional::OMITTED_NAMED_ARG, "new content from acc's private subscriptions"},
                            {ShortTxTypeConvertor::toString(ShortTxType::Repost), RPCArg::Type::STR, RPCArg::Optional::OMITTED_NAMED_ARG, "new reposts of acc's content"},
                            {ShortTxTypeConvertor::toString(ShortTxType::JuryAssigned), RPCArg::Type::STR, RPCArg::Optional::OMITTED_NAMED_ARG, "new assigned jury for authors"},
                            {ShortTxTypeConvertor::toString(ShortTxType::JuryModerate), RPCArg::Type::STR, RPCArg::Optional::OMITTED_NAMED_ARG, "new assigned jury for moderators"},
                            {ShortTxTypeConvertor::toString(ShortTxType::JuryVerdict), RPCArg::Type::STR, RPCArg::Optional::OMITTED_NAMED_ARG, "new verdict by assigned jury"},
                        }
                    },
                },
                {
                    // TODO (rpc): return description
                },
                RPCExamples{
                    // TODO (rpc): better examples
                    HelpExampleCli("getcontentactions", "") +
                    HelpExampleRpc("getcontentactions", "")
                },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
    {
        RPCTypeCheck(request.params, {UniValue::VNUM});

        auto height = request.params[0].get_int64();

        if (height > ChainActiveSafeHeight()) throw JSONRPCError(RPC_INVALID_PARAMETER, "Spefified height is greater than current chain height");

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

        return request.DbConnection()->NotifierRepoInst->GetNotifications(height, filters);
    },
       };
    }

    RPCHelpMan GetActivities()
    {
        return RPCHelpMan{"GetActivities",
                        "\nGet all activities created by account. Search depth - 3 months\n",
                        {
                            {"address", RPCArg::Type::STR, RPCArg::Optional::NO, "address to get activities for"},
                            {"heightMax", RPCArg::Type::NUM, RPCArg::Optional::OMITTED_NAMED_ARG, "max height to start search from, including. Default is current chain height"},
                            {"blockNum", RPCArg::Type::NUM, RPCArg::Optional::OMITTED_NAMED_ARG, "number of transaction in block to start search from for specified heightMax, excluding. Default is 999999"},
                            {"filters", RPCArg::Type::ARR, RPCArg::Optional::OMITTED_NAMED_ARG, "filters to specify event's types to search for. Default: search for all activities",
                            {
                                {ShortTxTypeConvertor::toString(ShortTxType::Answer), RPCArg::Type::STR, RPCArg::Optional::OMITTED_NAMED_ARG, "acc's answers to comments"},
                                {ShortTxTypeConvertor::toString(ShortTxType::Comment), RPCArg::Type::STR, RPCArg::Optional::OMITTED_NAMED_ARG, "acc's comments"},
                                {ShortTxTypeConvertor::toString(ShortTxType::Subscriber), RPCArg::Type::STR, RPCArg::Optional::OMITTED_NAMED_ARG, "acc's subscribes"},
                                {ShortTxTypeConvertor::toString(ShortTxType::CommentScore), RPCArg::Type::STR, RPCArg::Optional::OMITTED_NAMED_ARG, "acc's comments scores"},
                                {ShortTxTypeConvertor::toString(ShortTxType::ContentScore), RPCArg::Type::STR, RPCArg::Optional::OMITTED_NAMED_ARG, "acc's content scores"},
                                {ShortTxTypeConvertor::toString(ShortTxType::Boost), RPCArg::Type::STR, RPCArg::Optional::OMITTED_NAMED_ARG, "boosts content done by acc"},
                                {ShortTxTypeConvertor::toString(ShortTxType::Blocking), RPCArg::Type::STR, RPCArg::Optional::OMITTED_NAMED_ARG, "my blockings/unblockings"},
                            }
                        }
                          },
                          {
                                  // TODO (rpc): provide return description
                          },
                          RPCExamples{
                                  // TODO (rpc)
                                  HelpExampleCli("getactivities", "") +
                                  HelpExampleRpc("getactivities", "")
                          },
    [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
    {
        RPCTypeCheck(request.params, {UniValue::VSTR});

        auto address = request.params[0].get_str();

        int64_t heightMax = ChainActiveSafeHeight();
        if (request.params.size() > 1 && request.params[1].isNum()) {
            heightMax = request.params[1].get_int64();
        }

        int64_t blockNum = 9999999; // Default
        if (request.params.size() > 2 && request.params[2].isNum()) {
            blockNum = request.params[2].get_int64();
        }

        static const int64_t depth = 129600; // 3 months
        int64_t heightMin = heightMax - depth;
        if (heightMin < 0) {
            heightMin = 0;
        }

        std::set<ShortTxType> filters;
        if (request.params.size() > 3 && request.params[3].isArray()) {
            const auto& rawFilters  = request.params[3].get_array();
            for (int i = 0; i < rawFilters.size(); i++) {
                if (rawFilters[i].isStr()) {
                    const auto& rawFilter = rawFilters[i].get_str();
                    auto filter = ShortTxTypeConvertor::strToType(rawFilter);
                    if (!PocketHelpers::ShortTxFilterValidator::Activities::IsFilterAllowed(filter)) {
                        throw JSONRPCError(RPC_INVALID_PARAMETER, "Unexpected filter: " + rawFilter);
                    }
                    filters.insert(filter);
                }
            }
        }

        auto shortTxs = request.DbConnection()->NotifierRepoInst->GetActivities(address, heightMax, heightMin, blockNum, filters);
        UniValue res(UniValue::VARR);
        for (const auto& tx: shortTxs) {
            res.push_back(tx.Serialize());
        }

        return res;
    },
        };
    }

    RPCHelpMan GetNotificationsSummary()
    {
        return RPCHelpMan{"getnotificationssummary",
                        "\\n", // TODO (losty)
                        {

                          },
                          {
                                  // TODO (rpc): provide return description
                          },
                          RPCExamples{
                                  // TODO (rpc)
                                  HelpExampleCli("getnotificationssummary", "") +
                                  HelpExampleRpc("getnotificationssummary", "")
                          },
    [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
    {
        RPCTypeCheck(request.params, {UniValue::VARR});


        std::vector<std::string> addresses;
        auto addressesRaw = request.params[0].get_array();
        for (int i = 0; i < addressesRaw.size(); i++) {
            addresses.emplace_back(addressesRaw[i].get_str());
        }

        int64_t heightMax = ChainActiveSafeHeight();
        if (request.params.size() > 1 && request.params[1].isNum()) {
            heightMax = request.params[1].get_int64();
        }


        static const int64_t depth = 480; // 8 hours
        int64_t heightMin = heightMax - depth;
        if (heightMin < 0) {
            heightMin = 0;
        }

        std::set<ShortTxType> filters;
        if (request.params.size() > 3 && request.params[3].isArray()) {
            const auto& rawFilters  = request.params[3].get_array();
            for (int i = 0; i < rawFilters.size(); i++) {
                if (rawFilters[i].isStr()) {
                    const auto& rawFilter = rawFilters[i].get_str();
                    auto filter = ShortTxTypeConvertor::strToType(rawFilter);
                    if (!ShortTxFilterValidator::NotificationsSummary::IsFilterAllowed(filter)) {
                        throw JSONRPCError(RPC_INVALID_PARAMETER, "Unexpected filter: " + rawFilter);
                    }
                    filters.insert(filter);
                }
            }
        }

        auto res = request.DbConnection()->NotifierRepoInst->GetNotificationsSummary(heightMax, heightMin, addresses, filters);
        UniValue response(UniValue::VOBJ);

        for (const auto& addressEntry: res) {
            UniValue addressRelated(UniValue::VOBJ);
            for (const auto& summaryEntry: addressEntry.second) {
                addressRelated.pushKV(PocketHelpers::ShortTxTypeConvertor::toString(summaryEntry.first), summaryEntry.second);
            }
            response.pushKV(addressEntry.first, addressRelated);
        }

        return response;
    },
        };
    }

    RPCHelpMan GetsubsciptionsGroupedByAuthors()
    {
        return RPCHelpMan{"GetsubsciptionsGroupedByAuthors",
                          "\n\n", // TODO (rpc)
                          {
                                  {"address", RPCArg::Type::STR, RPCArg::Optional::NO, "" /* TODO (rpc): arg description*/},
                                  {"addressPagination", RPCArg::Type::STR, RPCArg::Optional::OMITTED_NAMED_ARG, "" /* TODO (rpc): arg description*/},
                                  {"nHeight", RPCArg::Type::NUM, RPCArg::Optional::OMITTED_NAMED_ARG, "" /*TODO (rpc): arg description*/},
                                  {"countOutOfUsers", RPCArg::Type::NUM, RPCArg::Optional::OMITTED_NAMED_ARG, "" /* TODO (rpc): arg description*/},
                                  {"countOutOfcontents", RPCArg::Type::STR, RPCArg::Optional::OMITTED_NAMED_ARG, "" /* TODO (rpc): arg description*/}
                          },
                          {
                                  // TODO (rpc): provide return description
                          },
                          RPCExamples{
                                  // TODO (rpc): better examples
                                  HelpExampleCli("getsubsciptionsgroupedbyauthors", "...") +
                                  HelpExampleRpc("getsubsciptionsgroupedbyauthors", "...")
                          },
                          [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
                          {
                              string address = "";
                              string addressPagination = "";
                              int nHeight = ChainActiveSafeHeight();
                              int countOutOfUsers = 10;
                              int countOutOfcontents = 10;

                              UniValue result(UniValue::VOBJ);

                              if (request.params.empty())
                                  return result;

                              if (request.params[0].isStr())
                                  address = request.params[0].get_str();

                              if (request.params.size() > 1 && request.params[1].isStr())
                                  addressPagination = request.params[1].get_str();

                              if (request.params.size() > 2 && request.params[2].isNum() && request.params[2].get_int() > 0)
                                  nHeight = request.params[2].get_int();

                              if (request.params.size() > 3 && request.params[3].isNum())
                                  countOutOfUsers = std::min(request.params[3].get_int(), 100);

                              if (request.params.size() > 4 && request.params[4].isNum())
                                  countOutOfcontents = std::min(request.params[4].get_int(), 1000);

                              auto reputationConsensus = ConsensusFactoryInst_Reputation.Instance(ChainActiveSafeHeight());
                              auto badReputationLimit = reputationConsensus->GetConsensusLimit(ConsensusLimit_bad_reputation);

                              UniValue content = request.DbConnection()->WebRpcRepoInst->GetsubsciptionsGroupedByAuthors(
                                      address, addressPagination, nHeight, countOutOfUsers, countOutOfcontents, badReputationLimit);

                              result.pushKV("height", nHeight);
                              result.pushKV("contents", content);
                              return result;
                          },
        };
    }
}
