// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 Bitcoin developers
// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include "PocketRpc.h"

#include "rpc/register.h"
#include "rpc/server.h"

UniValue debugweb(const JSONRPCRequest& request)
{
    PocketWeb::PocketContentRpc content;
    return content.GetContentsData(request); //PocketWeb::PocketContentRpc::GetContentsDataPub(request);
}


static const CRPCCommand commands[] =
{
    {"pocketnetrpc", "debugweb",      &debugweb,      {}, false},

    // Explorer
    { "explorer",         "getstatistic",               &PocketExplorerRpc::GetStatistic,               {"endTime","depth"}, false},
    { "explorer",         "getaddressspent",            &PocketExplorerRpc::GetAddressSpent,            {"address"}, false },
    { "explorer",         "getcompactblock",            &PocketExplorerRpc::GetCompactBlock,            {"blockHash"}, false },
    { "explorer",         "getlastblocks",              &PocketExplorerRpc::GetLastBlocks,              {"count","lastHeight","verbose"}, false },
    { "explorer",         "searchbyhash",               &PocketExplorerRpc::SearchByHash,               {"value"}, false },
    { "explorer",         "gettransactions",            &PocketExplorerRpc::GetTransactions,            {"transactions"}, false },
    { "explorer",         "getaddresstransactions",     &PocketExplorerRpc::GetAddressTransactions,     {"address"}, false },
    { "explorer",         "getblocktransactions",       &PocketExplorerRpc::GetBlockTransactions,       {"blockHash"}, false },
};

void RegisterPocketnetWebRPCCommands(CRPCTable& t)
{
    for (const auto & command : commands)
        t.appendCommand(command.name, &command);
}
