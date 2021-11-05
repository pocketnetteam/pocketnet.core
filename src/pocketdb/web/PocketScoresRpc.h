// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef SRC_POCKETSCORESRPC_H
#define SRC_POCKETSCORESRPC_H

#include "rpc/server.h"
#include "rpc/rawtransaction.h"

namespace PocketWeb::PocketWebRpc
{
    using namespace std;

    UniValue GetAddressScores(const JSONRPCRequest& request);
    UniValue GetPostScores(const JSONRPCRequest& request);
    UniValue GetPageScores(const JSONRPCRequest& request);
    UniValue GetCoinInfo(const JSONRPCRequest& request);

}


#endif //SRC_POCKETSCORESRPC_H
