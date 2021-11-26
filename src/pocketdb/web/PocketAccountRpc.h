// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef SRC_POCKETACCOUNTRPC_H
#define SRC_POCKETACCOUNTRPC_H

#include "rpc/server.h"
#include "rpc/rawtransaction.h"
#include "pocketdb/consensus/Reputation.h"

namespace PocketWeb::PocketWebRpc
{
    using namespace std;
    using namespace PocketConsensus;

    UniValue GetAddressId(const JSONRPCRequest& request);
    UniValue GetAccountProfiles(const JSONRPCRequest& request);
    UniValue GetAccountSetting(const JSONRPCRequest& request);
    UniValue GetUserAddress(const JSONRPCRequest& request);
    UniValue GetAddressRegistration(const JSONRPCRequest& request);
    UniValue GetUserState(const JSONRPCRequest& request);
    UniValue GetUnspents(const JSONRPCRequest& request);
    UniValue GetUserStatistic(const JSONRPCRequest& request);
    UniValue GetUserSubscribes(const JSONRPCRequest& request);
    UniValue GetUserSubscribers(const JSONRPCRequest& request);
    UniValue GetUserBlockings(const JSONRPCRequest& request);
}


#endif //SRC_POCKETACCOUNTRPC_H
