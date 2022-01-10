// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef SRC_POCKETEXPLORERRPC_H
#define SRC_POCKETEXPLORERRPC_H

#include "rpc/server.h"
#include "rpc/blockchain.h"
#include "validation.h"

namespace PocketWeb::PocketWebRpc
{
    using namespace PocketDb;
    using namespace std;

    RPCHelpMan GetStatisticByHours();
    RPCHelpMan GetStatisticByDays();
    RPCHelpMan GetStatisticContent();
    RPCHelpMan GetLastBlocks();
    RPCHelpMan GetCompactBlock();
    RPCHelpMan GetAddressInfo();
    RPCHelpMan SearchByHash();
    RPCHelpMan GetAddressTransactions();
    RPCHelpMan GetBlockTransactions();
    RPCHelpMan GetTransactions();
}


#endif //SRC_POCKETEXPLORERRPC_H
