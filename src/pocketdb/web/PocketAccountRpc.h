// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef SRC_POCKETACCOUNTRPC_H
#define SRC_POCKETACCOUNTRPC_H

#include "rpc/server.h"
// #include "rpc/rawtransaction.h"
#include "pocketdb/consensus/Reputation.h"

namespace PocketWeb::PocketWebRpc
{
    using namespace std;
    using namespace PocketConsensus;

    RPCHelpMan GetAccountId();
    RPCHelpMan GetAccountProfiles();
    RPCHelpMan GetAccountSetting();
    RPCHelpMan GetAccountAddress();
    RPCHelpMan GetAccountRegistration();
    RPCHelpMan GetAccountState();
    RPCHelpMan GetAccountUnspents();
    RPCHelpMan GetAccountStatistic();
    RPCHelpMan GetAccountSubscribes();
    RPCHelpMan GetAccountSubscribers();
    RPCHelpMan GetAccountBlockings();
}


#endif //SRC_POCKETACCOUNTRPC_H
