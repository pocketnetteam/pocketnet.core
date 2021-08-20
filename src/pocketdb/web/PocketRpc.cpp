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
    { "contents",       "getcontentsdata",                  &GetContentsData,               {"ids"}},
    { "contents",       "gethistoricalstrip",               &GetHistoricalStrip,            {"endTime", "depth"}},
    { "contents",       "gethierarchicalstrip",             &GetHierarchicalStrip,          {"endTime", "depth"}},

    // Comments
    {"comments",        "getcomments",                      &GetComments,                   {"postid", "parentid", "address", "ids"}},
    {"comments",        "getlastcomments",                  &GetLastComments,               {"count", "address"}},
    // TODO (team): implement
    // GetCommentsByPost
    // GetCommentsByIds

    // Accounts
    { "accounts",       "getuserprofile",                   &GetUserProfile,                {"addresses", "short"}},
    { "accounts",       "getuseraddress",                   &GetUserAddress,                {"name"}},
    { "accounts",       "getaddressregistration",           &GetAddressRegistration,        {"addresses"}},
    { "accounts",       "getuserstate",                     &GetUserState,                  {"address"}},
    // TODO (team): maybe remove
    // { "accounts",       "getreputations",                &GetReputations,                {}},

    // Scores
    {"scores",          "getaddressscores",                 &GetAddressScores,              {"address", "txs"}},
    {"scores",          "getpostscores",                    &GetPostScores,                 {"txs", "address"}},
    {"scores",          "getpagescores",                    &GetPageScores,                 {"txs", "address", "cmntids"}},

    // Explorer
    { "explorer",       "getstatistic",                     &GetStatistic,                  {"endTime", "depth"}},
    { "explorer",       "getaddressspent",                  &GetAddressSpent,               {"address"}},
    { "explorer",       "getcompactblock",                  &GetCompactBlock,               {"blockHash"}},
    { "explorer",       "getlastblocks",                    &GetLastBlocks,                 {"count", "lastHeight", "verbose"}},
    { "explorer",       "searchbyhash",                     &SearchByHash,                  {"value"}},
    { "explorer",       "gettransactions",                  &GetTransactions,               {"transactions"}},
    { "explorer",       "getaddresstransactions",           &GetAddressTransactions,        {"address"}},
    { "explorer",       "getblocktransactions",             &GetBlockTransactions,          {"blockHash"}},

    // System
    { "system",         "getpeerinfo",                      &GetPeerInfo,                   {}},
    { "system",         "getnodeinfo",                      &GetNodeInfo,                   {}},
    // TODO (team): implement
    // { "system",         "getrawtransaction",                &GetRawTransaction,             {"txid", "verbose", "blockhash"}},
    // { "system",         "sendrawtransactionwithmessage",    &SetTransaction,                {"hexstring", "message", "type"}},
};
// @formatter:on

void RegisterPocketnetWebRPCCommands(CRPCTable& t)
{
    for (const auto& command : commands)
        t.appendCommand(command.name, &command);
}
