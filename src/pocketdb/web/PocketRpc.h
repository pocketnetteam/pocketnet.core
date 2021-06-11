// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 Bitcoin developers
// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef SRC_POCKETRPC_H
#define SRC_POCKETRPC_H

#include "rpc/server.h"
#include "rpc/rawtransaction.h"

namespace PocketWeb
{
    class PocketRpc
    {
    public:
        UniValue GetLastComments(const JSONRPCRequest& request);
    };
} // namespace PocketWeb


#endif //SRC_POCKETRPC_H
