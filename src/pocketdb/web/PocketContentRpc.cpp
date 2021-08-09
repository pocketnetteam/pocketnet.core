// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include "PocketContentRpc.h"

namespace PocketWeb::PocketContentRpc {
    namespace {
        typedef PocketDb::WebRepository::param param;

        std::map<std::string, param> ParseParams(UniValue params)
        {
            std::map<std::string, param> conditions;

            if (params.size() > 0) {
                if (params[0].isNum()) {
                    if (params[0].get_int() > 0) {
                        conditions.emplace("height", param(params[0].get_int()));
                    }
                }
            }

            if (params.size() > 1) {
                conditions.emplace("txidstart",param(boost::trim_copy(params[1].get_str())));
            }

            if (params.size() > 2) {
                if (params[2].isNum()) {
                    conditions.emplace("count",param(params[2].get_int()));
                }
            }

            if (params.size() > 3) {
                conditions.emplace("lang",param(boost::trim_copy(params[3].get_str())));
            }

            if (params.size() > 4) {
                std::vector<std::string> tags;
                if (params[4].isStr()) {
                    std::string tag = boost::trim_copy(params[4].get_str());
                    if (!tag.empty()) {
                        tags.emplace_back(tag);
                    }
                } else if (params[4].isArray()) {
                    UniValue tgs = params[4].get_array();
                    if(tgs.size() > 0) {
                        for (unsigned int idx = 0; idx < tgs.size(); idx++) {
                            std::string tag = boost::trim_copy(tgs[idx].get_str());
                            if (!tag.empty()) {
                                tags.emplace_back(tag);
                            }
                        }
                    }
                }

                if (!tags.empty()) {
                    conditions.emplace("tags", param(tags));
                }
            }

            if (params.size() > 5) {
                std::vector<int> contentTypes;
                if (params[5].isNum()) {
                    contentTypes.push_back(params[5].get_int());
                } else if (params[5].isStr()) {
//                    if (getcontenttype(params[5].get_str()) >= 0) {
//                        contentTypes.push_back(getcontenttype(params[5].get_str()));
//                    }
                } else if (params[5].isArray()) {
                    UniValue cntntTps = params[5].get_array();
                    if(cntntTps.size() > 0) {
                        for (unsigned int idx = 0; idx < cntntTps.size(); idx++) {
                            if (cntntTps[idx].isNum()) {
                                contentTypes.emplace_back(cntntTps[idx].get_int());
                            } else if (cntntTps[idx].isStr()) {
//                                if (getcontenttype(cntntTps[idx].get_str()) >= 0) {
//                                    contentTypes.push_back(getcontenttype(cntntTps[idx].get_str()));
//                                }
                            }
                        }
                    }
                }

                if (!contentTypes.empty()) {
                    conditions.emplace("types", param(contentTypes));
                }
            }

//            std::vector<string> txidsExcluded;
//            if (request.params.size() > 6) {
//                if (request.params[6].isStr()) {
//                    txidsExcluded.push_back(request.params[6].get_str());
//                } else if (request.params[6].isArray()) {
//                    UniValue txids = request.params[6].get_array();
//                    //if (txids.size() > 1000) {
//                    //    throw JSONRPCError(RPC_INVALID_PARAMS, "Too large array txids");
//                    //}
//                    if(txids.size() > 0) {
//                        for (unsigned int idx = 0; idx < txids.size(); idx++) {
//                            std::string txidEx = boost::trim_copy(txids[idx].get_str());
//                            if (!txidEx.empty()) {
//                                txidsExcluded.push_back(txidEx);
//                            }
//                        }
//                    }
//                }
//            }
//
//            std::vector<string> adrsExcluded;
//            if (request.params.size() > 7) {
//                if (request.params[7].isStr()) {
//                    adrsExcluded.push_back(request.params[7].get_str());
//                } else if (request.params[7].isArray()) {
//                    UniValue adrs = request.params[7].get_array();
//                    if (adrs.size() > 1000) {
//                        throw JSONRPCError(RPC_INVALID_PARAMS, "Too large array addresses");
//                    }
//                    if(adrs.size() > 0) {
//                        for (unsigned int idx = 0; idx < adrs.size(); idx++) {
//                            std::string adrEx = boost::trim_copy(adrs[idx].get_str());
//                            if (!adrEx.empty()) {
//                                adrsExcluded.push_back(adrEx);
//                            }
//                        }
//                    }
//                }
//            }
//
//            std::vector<string> tagsExcluded;
//            if (request.params.size() > 8) {
//                if (request.params[8].isStr()) {
//                    tagsExcluded.push_back(request.params[8].get_str());
//                } else if (request.params[8].isArray()) {
//                    UniValue tagsEx = request.params[8].get_array();
//                    if (tagsEx.size() > 1000) {
//                        throw JSONRPCError(RPC_INVALID_PARAMS, "Too large array tags");
//                    }
//                    if(tagsEx.size() > 0) {
//                        for (unsigned int idx = 0; idx < tagsEx.size(); idx++) {
//                            std::string tgsEx = boost::trim_copy(tagsEx[idx].get_str());
//                            if (!tgsEx.empty()) {
//                                tagsExcluded.push_back(tgsEx);
//                            }
//                        }
//                    }
//                }
//            }

            return conditions;
        }

        std::map<std::string, UniValue> GetContentsData(std::vector<std::string> txids)
        {
            auto result = PocketDb::WebRepoInst.GetContentsData(txids);

            return result;
        }
    }
}

UniValue PocketWeb::PocketContentRpc::GetContentsData(const JSONRPCRequest& request)
{
    if (request.fHelp)
        throw std::runtime_error(
            "GetContentsData\n"
            "\n.\n");

    std::vector<std::string> txids;
    if (request.params.size() > 0) {
        if (request.params[0].isStr()) {
            txids.emplace_back(request.params[0].get_str());
        }
        else if (request.params[0].isArray()) {
            UniValue ids = request.params[0].get_array();
            for (unsigned int idx = 0; idx < ids.size(); idx++){
                if(ids[idx].isStr()) {
                    txids.emplace_back(ids[idx].get_str());
                }
            }
        }
    }

    std::map<std::string, UniValue> contentsdata = GetContentsData(txids);

    UniValue aResult(UniValue::VARR);
    for (auto& cd : contentsdata) {
        aResult.push_back(cd.second);
    }
    return aResult;
}

UniValue PocketWeb::PocketContentRpc::GetHistoricalStrip(const JSONRPCRequest& request)
{
    if (request.fHelp)
        throw std::runtime_error(
            "GetHistoricalStrip\n"
            "\n.\n");

    UniValue oResult(UniValue::VOBJ);
    UniValue aContents(UniValue::VARR);

    int height = chainActive.Height(); // TODO (only1question): read from input params
    if (request.params.size()>0) {
        std::map<std::string, param> conditions = ParseParams(request.params);
        map<string, UniValue> contents = PocketDb::WebRepoInst.GetContents(conditions);

        for (auto& c : contents) {
            aContents.push_back(c.second);
        }
    }

    oResult.pushKV("height", height);
    oResult.pushKV("contents", aContents);
    oResult.pushKV("contentsTotal", "0");
    return oResult;
}

UniValue PocketWeb::PocketContentRpc::GetHierarchicalStrip(const JSONRPCRequest& request)
{
    if (request.fHelp)
        throw std::runtime_error(
            "GetHierarchicalStrip\n"
            "\n.\n");

    return GetHistoricalStrip(request);
    UniValue oResult(UniValue::VOBJ);
    return oResult;
}