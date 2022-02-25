// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef SRC_WEBSOCKET_RPC_H
#define SRC_WEBSOCKET_RPC_H

#include "rpc/server.h"
#include "logging.h"
#include "validation.h"
#include "pocketdb/web/PocketExplorerRpc.h"

namespace PocketWeb::PocketWebRpc
{
    UniValue GetMissedInfo(const JSONRPCRequest& request);
} // namespace PocketWeb


#endif //SRC_WEBSOCKET_RPC_H
