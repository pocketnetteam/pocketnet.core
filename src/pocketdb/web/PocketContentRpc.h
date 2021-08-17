// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef SRC_POCKETDEBUG_H
#define SRC_POCKETDEBUG_H

#include "rpc/server.h"
#include "rpc/rawtransaction.h"

namespace PocketWeb::PocketWebRpc
    {
        UniValue GetContentsData(const JSONRPCRequest& request);
        UniValue GetHistoricalStrip(const JSONRPCRequest& request);
        UniValue GetHierarchicalStrip(const JSONRPCRequest& request);
    }

#endif //SRC_POCKETDEBUG_H
