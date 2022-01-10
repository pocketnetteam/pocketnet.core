// Copyright (c) 2018-2022 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef SRC_WEB_RPC_UTILS_H
#define SRC_WEB_RPC_UTILS_H

#include "rpc/server.h"
#include "utils/html.h"
#include "pocketdb/models/base/PocketTypes.h"
#include "pocketdb/helpers/TransactionHelper.h"

namespace PocketWeb::PocketWebRpc
{
    using namespace std;
    using namespace PocketTx;
    using namespace PocketHelpers;

    void ParseRequestContentTypes(const UniValue& value, vector<int>& types);
    void ParseRequestTags(const UniValue& value, vector<string>& tags);
}

#endif //SRC_WEB_RPC_UTILS_H