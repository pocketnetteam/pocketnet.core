// Copyright (c) 2018-2022 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef SRC_POCKETSCORESRPC_H
#define SRC_POCKETSCORESRPC_H

#include "rpc/server.h"
// #include "rpc/rawtransaction.h"

namespace PocketWeb::PocketWebRpc
{
    using namespace std;

    RPCHelpMan GetAddressScores();
    RPCHelpMan GetPostScores();
    RPCHelpMan GetPagesScores();
    RPCHelpMan GetCoinInfo();

}


#endif //SRC_POCKETSCORESRPC_H
