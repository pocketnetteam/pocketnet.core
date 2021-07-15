// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 Bitcoin developers
// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef SRC_POCKETUSERRPC_H
#define SRC_POCKETUSERRPC_H

#include "rpc/server.h"
#include "rpc/rawtransaction.h"

namespace PocketWeb
{
    using std::string;
    using std::vector;
    using std::map;

    class PocketUserRpc
    {
    public:
        UniValue GetReputations(const JSONRPCRequest& request);
    private:
        map<string, UniValue> GetUsersProfiles(std::vector<std::string> addresses, bool shortForm = true, int option = 0);
    };
}


#endif //SRC_POCKETUSERRPC_H
