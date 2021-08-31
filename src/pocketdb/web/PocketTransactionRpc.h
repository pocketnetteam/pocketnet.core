// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef SRC_POCKETTRANSACTIONRPC_H
#define SRC_POCKETTRANSACTIONRPC_H

#include "rpc/server.h"
#include "init.h"
#include "validation.h"
#include "consensus/validation.h"
#include "validationinterface.h"
#include "txmempool.h"
#include "pocketdb/services/TransactionSerializer.h"
#include "pocketdb/consensus/Base.h"
#include "pocketdb/consensus/Helper.h"

namespace PocketWeb::PocketWebRpc
{
    UniValue _accept_transaction(const CTransactionRef& tx, const PTransactionRef& ptx);
    UniValue AddTransaction(const JSONRPCRequest& request);
    UniValue GetTransaction(const JSONRPCRequest& request);
}

#endif //SRC_POCKETTRANSACTIONRPC_H
