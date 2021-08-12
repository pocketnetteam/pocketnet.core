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

// @formatter:off
static const CRPCCommand commands[] =
{
    {"debug", "debugweb", &debug, {}},

    // Contents
    { "contents", "getcontentsdata",        &PocketWeb::PocketContentRpc::GetContentsData,      {"ids"}},
    { "contents", "gethistoricalstrip",     &PocketWeb::PocketContentRpc::GetHistoricalStrip,   {"endTime", "depth"}},
    { "contents", "gethierarchicalstrip",   &PocketWeb::PocketContentRpc::GetHierarchicalStrip, {"endTime", "depth"}},

    // Comments


    // Explorer
    { "explorer", "getstatistic",           &PocketExplorerRpc::GetStatistic,                   {"endTime", "depth"}},
    { "explorer", "getaddressspent",        &PocketExplorerRpc::GetAddressSpent,                {"address"}},
    { "explorer", "getcompactblock",        &PocketExplorerRpc::GetCompactBlock,                {"blockHash"}},
    { "explorer", "getlastblocks",          &PocketExplorerRpc::GetLastBlocks,                  {"count", "lastHeight", "verbose"}},
    { "explorer", "searchbyhash",           &PocketExplorerRpc::SearchByHash,                   {"value"}},
    { "explorer", "gettransactions",        &PocketExplorerRpc::GetTransactions,                {"transactions"}},
    { "explorer", "getaddresstransactions", &PocketExplorerRpc::GetAddressTransactions,         {"address"}},
    { "explorer", "getblocktransactions",   &PocketExplorerRpc::GetBlockTransactions,           {"blockHash"}}
};
// @formatter:on

void RegisterPocketnetWebRPCCommands(CRPCTable& t)
{
    for (const auto & command : commands)
        t.appendCommand(command.name, &command);
}
