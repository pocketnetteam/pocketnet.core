// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 Bitcoin developers
// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include "PocketRpc.h"

UniValue debugweb(const JSONRPCRequest& request)
{
    PocketWeb::PocketContentRpc content;
    return content.GetContentsData(request); //PocketWeb::PocketContentRpc::GetContentsDataPub(request);
}
//----------------------------------------------------------

static const CRPCCommand commands[] =
    {
        {"pocketnetrpc", "debugweb",      &debugweb,      {}, false},
    };

void RegisterPocketnetWebRPCCommands(CRPCTable& t)
{
    for (unsigned int vcidx = 0; vcidx < ARRAYLEN(commands); vcidx++)
        t.appendCommand(commands[vcidx].name, &commands[vcidx]);
}
