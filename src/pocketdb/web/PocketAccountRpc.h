// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef SRC_POCKETACCOUNTRPC_H
#define SRC_POCKETACCOUNTRPC_H

#include "rpc/server.h"
#include "rpc/rawtransaction.h"

namespace PocketWeb::PocketWebRpc
{
    using namespace std;

    UniValue GetReputations(const JSONRPCRequest& request);
    UniValue GetUserProfile(const JSONRPCRequest& request);
    UniValue GetUserAddress(const JSONRPCRequest& request);
    UniValue GetAddressRegistration(const JSONRPCRequest& request);
    UniValue GetUserState(const JSONRPCRequest& request);
    map<string, UniValue> GetUsersProfiles(const DbConnectionRef& dbCon, std::vector<std::string> addresses,
        bool shortForm = true, int option = 0);
}


#endif //SRC_POCKETACCOUNTRPC_H
