// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef SRC_POCKETDEBUG_H
#define SRC_POCKETDEBUG_H

#include "rpc/server.h"
#include "rpc/rawtransaction.h"
#include "pocketdb/helpers/TransactionHelper.h"

namespace PocketWeb::PocketWebRpc
{
    using namespace PocketHelpers;

    void ParseRequestContentType(const UniValue& value, vector<int>& types);

    UniValue GetContent(const JSONRPCRequest& request);
    UniValue GetHotPosts(const JSONRPCRequest& request);
    UniValue GetHistoricalFeed(const JSONRPCRequest& request);
    UniValue GetHierarchicalFeed(const JSONRPCRequest& request);
    UniValue GetProfileFeed(const JSONRPCRequest& request);
    UniValue GetSubscribesFeed(const JSONRPCRequest& request);
    UniValue FeedSelector(const JSONRPCRequest& request);
    UniValue GetContentsStatistic(const JSONRPCRequest& request);
    
}

#endif //SRC_POCKETDEBUG_H
