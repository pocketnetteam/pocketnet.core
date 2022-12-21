// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef SRC_MODERATION_RPC_H
#define SRC_MODERATION_RPC_H

#include "rpc/server.h"
#include "pocketdb/web/WebRpcUtils.h"
#include "pocketdb/consensus/Reputation.h"

namespace PocketWeb::PocketWebRpc
{
    using namespace std;
    using namespace PocketConsensus;

    RPCHelpMan GetAssignedJury();

}


#endif //SRC_MODERATION_RPC_H
