// Copyright (c) 2018-2022 The Pocketnet developers
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

    RPCHelpMan GetStatisticTransactions();
    RPCHelpMan GetStatisticByHours();
    RPCHelpMan GetStatisticByDays();
    RPCHelpMan GetStatisticContentByHours();
    RPCHelpMan GetStatisticContentByDays();
    RPCHelpMan GetLastBlocks();
    RPCHelpMan GetCompactBlock();
    RPCHelpMan GetAddressInfo();
    RPCHelpMan GetBalanceHistory();
    RPCHelpMan SearchByHash();
    RPCHelpMan GetAddressTransactions();
    RPCHelpMan GetBlockTransactions();
    RPCHelpMan GetTransaction();
    RPCHelpMan GetTransactions();

    UniValue _constructTransaction(const PTransactionRef& ptx);
}


#endif //SRC_POCKETEXPLORERRPC_H
