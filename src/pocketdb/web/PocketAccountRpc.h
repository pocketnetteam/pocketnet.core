// Copyright (c) 2018-2021 Pocketnet developers
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

    RPCHelpMan GetAddressId();
    RPCHelpMan GetAccountProfiles();
    RPCHelpMan GetAccountSetting();
    RPCHelpMan GetUserAddress();
    RPCHelpMan GetAddressRegistration();
    RPCHelpMan GetUserState();
    map<string, UniValue> GetUsersProfiles(const DbConnectionRef& dbCon, std::vector<std::string> addresses, bool shortForm = true);
    RPCHelpMan GetUnspents();
    RPCHelpMan GetUserStatistic();
}


#endif //SRC_POCKETACCOUNTRPC_H
