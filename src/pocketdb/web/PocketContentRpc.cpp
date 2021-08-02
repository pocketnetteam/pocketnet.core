// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 Bitcoin developers
// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include "PocketContentRpc.h"

std::map<std::string, UniValue> PocketWeb::PocketContentRpc::GetContentsData(std::vector<std::string> txids)
{
    auto result = PocketDb::WebRepoInst.GetContentsData(txids);

    return result;
}

UniValue PocketWeb::PocketContentRpc::GetContentsData(const JSONRPCRequest& request)
{
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

    std::map<std::string, UniValue> contents = GetContentsData(txids);

    UniValue aResult(UniValue::VARR);
    for (auto& c : contents) {
        aResult.push_back(c.second);
    }
    return aResult;
}