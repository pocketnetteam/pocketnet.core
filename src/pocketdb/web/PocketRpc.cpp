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

    return GetContentsData(request);
}

// @formatter:off
static const CRPCCommand commands[] =
{
    {"debug", "debugweb", &debug, {}},

    // Contents
    { "contents", "getcontentsdata",        &GetContentsData,      {"ids"}},
    { "contents", "gethistoricalstrip",     &GetHistoricalStrip,   {"endTime", "depth"}},
    { "contents", "gethierarchicalstrip",   &GetHierarchicalStrip, {"endTime", "depth"}},

    // Comments


    // Accounts
    { "accounts", "getreputations",         &GetReputations,                  {}},
    { "accounts", "getuserprofile",         &GetUserProfile,                  {}},
    { "accounts", "getuseraddress",         &GetUserAddress,                  {}},
    { "accounts", "getaddressregistration", &GetAddressRegistration,                  {}},
    { "accounts", "getuserstate",           &GetUserState,                  {}},

    // Explorer
    { "explorer", "getstatistic",           &GetStatistic,                   {"endTime", "depth"}},
    { "explorer", "getaddressspent",        &GetAddressSpent,                {"address"}},
    { "explorer", "getcompactblock",        &GetCompactBlock,                {"blockHash"}},
    { "explorer", "getlastblocks",          &GetLastBlocks,                  {"count", "lastHeight", "verbose"}},
    { "explorer", "searchbyhash",           &SearchByHash,                   {"value"}},
    { "explorer", "gettransactions",        &GetTransactions,                {"transactions"}},
    { "explorer", "getaddresstransactions", &GetAddressTransactions,         {"address"}},
    { "explorer", "getblocktransactions",   &GetBlockTransactions,           {"blockHash"}}
};
// @formatter:on

void RegisterPocketnetWebRPCCommands(CRPCTable& t)
{
    for (const auto& command : commands)
        t.appendCommand(command.name, &command);
}
