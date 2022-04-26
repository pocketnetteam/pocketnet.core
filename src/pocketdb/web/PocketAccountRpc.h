// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef SRC_POCKETACCOUNTRPC_H
#define SRC_POCKETACCOUNTRPC_H

#include "rpc/server.h"
#include "rpc/rawtransaction.h"
#include "pocketdb/web/WebRpcUtils.h"
#include "pocketdb/consensus/Reputation.h"

namespace PocketWeb::PocketWebRpc
{
    using namespace std;
    using namespace PocketConsensus;

    UniValue GetAccountId(const JSONRPCRequest& request);
    UniValue GetAccountProfiles(const JSONRPCRequest& request);
    UniValue GetAccountSetting(const JSONRPCRequest& request);
    UniValue GetAccountAddress(const JSONRPCRequest& request);
    UniValue GetAccountRegistration(const JSONRPCRequest& request);
    UniValue GetAccountState(const JSONRPCRequest& request);
    UniValue GetAccountUnspents(const JSONRPCRequest& request);
    UniValue GetAccountStatistic(const JSONRPCRequest& request);
    UniValue GetAccountSubscribes(const JSONRPCRequest& request);
    UniValue GetAccountSubscribers(const JSONRPCRequest& request);
    UniValue GetAccountBlockings(const JSONRPCRequest& request);
    UniValue GetTopAccounts(const JSONRPCRequest& request);
}


#endif //SRC_POCKETACCOUNTRPC_H
