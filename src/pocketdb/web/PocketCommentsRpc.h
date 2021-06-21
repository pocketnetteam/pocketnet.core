// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 Bitcoin developers
// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef SRC_POCKETCOMMENTSRPC_H
#define SRC_POCKETCOMMENTSRPC_H

#include "rpc/server.h"
#include "rpc/rawtransaction.h"

namespace PocketWeb
{
    class PocketCommentsRpc
    {
    public:
        UniValue GetComments(const JSONRPCRequest& request);
        UniValue GetCommentsByPost(const JSONRPCRequest& request);
        UniValue GetCommentsByIds(const JSONRPCRequest& request);
        UniValue GetLastComments(const JSONRPCRequest& request);
    };
} // namespace PocketWeb


#endif //SRC_POCKETCOMMENTSRPC_H
