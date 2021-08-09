// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include "PocketRpc.h"

UniValue debug(const JSONRPCRequest& request)
{
    if (request.fHelp)
        throw std::runtime_error(
            "debug\n"
            "\n.\n");

    return PocketWeb::PocketContentRpc::GetContentsData(request);
}


static const CRPCCommand commands[] =
{
    {"debug", "debugweb", &debug, {}, false},

    //Contents
    { "contents", "gethistoricalstrip",     &PocketWeb::PocketContentRpc::GetHistoricalStrip,   {"endTime", "depth"}, false},
    { "contents", "gethierarchicalstrip",   &PocketWeb::PocketContentRpc::GetHierarchicalStrip, {"endTime", "depth"}, false},

    // Explorer
    { "explorer", "getstatistic",           &PocketExplorerRpc::GetStatistic,                   {"endTime", "depth"}, false},
    { "explorer", "getaddressspent",        &PocketExplorerRpc::GetAddressSpent,                {"address"}, false },
    { "explorer", "getcompactblock",        &PocketExplorerRpc::GetCompactBlock,                {"blockHash"}, false },
    { "explorer", "getlastblocks",          &PocketExplorerRpc::GetLastBlocks,                  {"count", "lastHeight", "verbose"}, false },
    { "explorer", "searchbyhash",           &PocketExplorerRpc::SearchByHash,                   {"value"}, false },
    { "explorer", "gettransactions",        &PocketExplorerRpc::GetTransactions,                {"transactions"}, false },
    { "explorer", "getaddresstransactions", &PocketExplorerRpc::GetAddressTransactions,         {"address"}, false },
    { "explorer", "getblocktransactions",   &PocketExplorerRpc::GetBlockTransactions,           {"blockHash"}, false },
};

void RegisterPocketnetWebRPCCommands(CRPCTable& t)
{
    for (const auto & command : commands)
        t.appendCommand(command.name, &command);
}
