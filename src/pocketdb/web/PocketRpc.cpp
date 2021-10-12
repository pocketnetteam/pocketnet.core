// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include "pocketdb/web/PocketRpc.h"

UniValue debug(const JSONRPCRequest& request)
{
    if (request.fHelp)
        throw std::runtime_error(
            "debug\n"
            "\n.\n");

    return UniValue();// GetContentsData(request);
}

UniValue getrawtransactionwithmessagebyid(const JSONRPCRequest& request)
{
    if (request.fHelp)
        throw std::runtime_error(
            "getrawtransactionwithmessagebyid\n"
            "\nReturn Pocketnet posts.\n");

    return UniValue();// GetContentsData(request);
}

UniValue gettemplate(const JSONRPCRequest& request)
{
    if (request.fHelp)
        throw std::runtime_error(
            "getrawtransactionwithmessagebyid\n"
            "\nReturn Pocketnet posts.\n");

    UniValue aResult(UniValue::VARR);
    return aResult;
}

// @formatter:off
static const CRPCCommand commands[] =
{
    {"debug", "debugweb", &debug, {}},

    // Old methods
    {"artifacts", "getrawtransactionwithmessagebyid", &getrawtransactionwithmessagebyid,  {"ids"}},
    {"artifacts", "getrawtransactionwithmessage",     &gettemplate,                       {"address_from", "address_to", "start_txid", "count", "lang", "tags", "contenttypes"}},
    {"artifacts", "getrecommendedposts",              &gettemplate,                       {"address", "count", "height", "lang", "contenttypes"}},
    {"artifacts", "search",                           &gettemplate,                       {"search_string", "type", "count"}},
    {"artifacts", "searchlinks",                      &gettemplate,                       {"search_request", "contenttypes", "height", "count"}},
    {"artifacts", "getusercontents",                  &gettemplate,                       {"address", "height", "start_txid", "count", "lang", "tags", "contenttypes"}},
    {"artifacts", "getrecomendedsubscriptionsforuser",&gettemplate,                       {"address", "count"}},

    // WebSocket
    { "websocket",      "getmissedinfo",                    &GetMissedInfo,                 {"address", "blocknumber"}},

    // Contents
    {"contents",        "gethotposts",                      &GetHotPosts,                   {"count", "depth", "height", "lang", "contenttypes"}},
    {"contents",        "getcontents",                      &GetContents,                   {"address"}},
    //{ "contents",       "getcontentsdata",                  &GetContentsData,               {"ids"}},
    { "contents",       "gethistoricalstrip",               &GetHistoricalStrip,            {"endTime", "depth"}},
    { "contents",       "gethierarchicalstrip",             &GetHierarchicalStrip,          {"endTime", "depth"}},

    // Tags
//    {"artifacts", "searchtags",                       &gettemplate,                       {"search_string", "count"}},
    { "tags",           "gettags",                          &GetTags,                       {"address", "count", "height", "lang"}},

    // Comments
    {"comments",        "getcomments",                      &GetComments,                   {"postid", "parentid", "address", "ids"}},
    {"comments",        "getlastcomments",                  &GetLastComments,               {"count", "address"}},
    // TODO (only1question): implement
    // GetCommentsByPost
    // GetCommentsByIds

    // Accounts
    { "accounts",       "getuserprofile",                   &GetUserProfile,                {"addresses", "short"}},
    { "accounts",       "getuseraddress",                   &GetUserAddress,                {"name"}},
    { "accounts",       "getaddressregistration",           &GetAddressRegistration,        {"addresses"}},
    { "accounts",       "getuserstate",                     &GetUserState,                  {"address"}},
    { "accounts",       "txunspent",                        &GetUnspents,                   {"addresses", "minconf", "maxconf", "include_unsafe", "query_options"}},
    { "accounts",       "getaddressid",                     &GetAddressId,                  {"address_or_id"}},

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
    { "system",         "gettime",                          &GetTime,                       {}},

    { "transaction",    "sendrawtransactionwithmessage",    &AddTransaction,                {"hexstring", "message", "type"}},
    { "transaction",    "addtransaction",                   &AddTransaction,                {"hexstring", "message", "type"}},
    { "transaction",    "getrawtransaction",                &GetTransaction,                {"txid"}},
};
// @formatter:on

void RegisterPocketnetWebRPCCommands(CRPCTable& t)
{
    for (const auto& command : commands)
        t.appendCommand(command.name, &command);
}
