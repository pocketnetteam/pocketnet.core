// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef SRC_POCKETTRANSACTIONRPC_H
#define SRC_POCKETTRANSACTIONRPC_H

#include "rpc/server.h"
#include "pocketdb/services/TransactionSerializer.h"

namespace PocketWeb::PocketWebRpc
{
    UniValue AddTransaction(const JSONRPCRequest& request);
}

#endif //SRC_POCKETTRANSACTIONRPC_H
