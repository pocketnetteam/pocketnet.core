// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef SRC_POCKETSYSTEMRPC_H
#define SRC_POCKETSYSTEMRPC_H

#include "rpc/server.h"
#include "validation.h"
#include "clientversion.h"
#include "net_processing.h"
#include "pos.h"

namespace PocketWeb::PocketWebRpc
{
    RPCHelpMan GetTime();
    RPCHelpMan GetPeerInfo();
    RPCHelpMan GetNodeInfo();
}

#endif //SRC_POCKETSYSTEMRPC_H
