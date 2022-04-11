// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include "pocketdb/web/PocketRpc.h"

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
    {"hidden",       "generatepocketnettransaction",      &GenerateTransaction,             {"address", "privKey", "outCount", "type", "payload"}},
    {"hidden",       "generatepocketnetaddress",          &GenerateAddress,                 {}},

    // Old methods
    {"artifacts", "getrecommendedposts",              &gettemplate,                       {"address", "count", "height", "lang", "contenttypes"}},
    {"artifacts", "getusercontents",                  &gettemplate,                       {"address", "height", "start_txid", "count", "lang", "tags", "contenttypes"}},
    {"artifacts", "getrecomendedsubscriptionsforuser",&gettemplate,                       {"address", "count"}},

    // Search
    {"search",          "search",                           &Search,                        {"keyword", "type", "topBlock", "pageStart", "pageSize", "address"}},
    {"search",          "searchlinks",                      &SearchLinks,                   {"links", "contenttypes", "height", "count"}},
    {"search",          "searchusers",                      &SearchUsers,                   {"keyword", "fieldtypes", "orderbyrank"}},

    // Recomendations
    // TODO (o1q): Remove below methods when the client gui switches to new methods
    {"search",          "getrecomendedaccountsbysubscriptions",           &GetRecomendedAccountsBySubscriptions,           {"address", "count"}},
    {"search",          "getrecomendedaccountsbyscoresonsimilaraccounts", &GetRecomendedAccountsByScoresOnSimilarAccounts, {"address", "contenttypes", "height", "depth", "count"}},
    {"search",          "getrecomendedaccountsbyscoresfromaddress",       &GetRecomendedAccountsByScoresFromAddress,       {"address", "contenttypes", "height", "depth", "count"}},
    {"search",          "getrecomendedaccountsbytags",                    &GetRecomendedAccountsByTags,                    {"tags", "count"}},
    {"search",          "getrecomendedcontentsbyscoresonsimilarcontents", &GetRecomendedContentsByScoresOnSimilarContents, {"contentid", "contenttypes", "depth", "count"}},
    {"search",          "getrecomendedcontentsbyscoresfromaddress",       &GetRecomendedContentsByScoresFromAddress,       {"address", "contenttypes", "height", "depth", "count"}},
    // TODO (o1q): Remove above methods when the client gui switches to new (below) methods
    {"search",          "getrecommendedcontentbycontentid", &GetRecommendedContentByContentId, {"contentid", "address", "contenttypes", "lang", "count"}},
    {"search",          "getrecommendedcontentbyaddress",   &GetRecommendedContentByAddress,   {"address", "addressExclude", "contenttypes", "lang", "count"}},

    // WebSocket
    {"websocket",       "getmissedinfo",                    &GetMissedInfo,                 {"address", "blocknumber"}},

    // Contents
    {"contents",        "gethotposts",                      &GetHotPosts,                   {"count", "depth", "height", "lang", "contenttypes", "address"}},
    {"contents",        "gethistoricalfeed",                &GetHistoricalFeed,             {"topHeight","topContentHash","countOut","lang","tags","contentTypes","txIdsExcluded","adrsExcluded","tagsExcluded","address"}},
    {"contents",        "gethistoricalstrip",               &GetHistoricalFeed,             {"topHeight","topContentHash","countOut","lang","tags","contentTypes","txIdsExcluded","adrsExcluded","tagsExcluded","address"}},
    {"contents",        "gethierarchicalfeed",              &GetHierarchicalFeed,           {"topHeight","topContentHash","countOut","lang","tags","contentTypes","txIdsExcluded","adrsExcluded","tagsExcluded","address"}},
    {"contents",        "gethierarchicalstrip",             &GetHierarchicalFeed,           {"topHeight","topContentHash","countOut","lang","tags","contentTypes","txIdsExcluded","adrsExcluded","tagsExcluded","address"}},
    {"contents",        "getboostfeed",                     &GetBoostFeed,                  {"topHeight","topContentHash","countOut","lang","tags","contentTypes","txIdsExcluded","adrsExcluded","tagsExcluded","address"}},
    {"contents",        "getprofilefeed",                   &GetProfileFeed,                {"topHeight","topContentHash","countOut","lang","tags","contentTypes","txIdsExcluded","adrsExcluded","tagsExcluded","address", "address_feed"}},
    {"contents",        "getsubscribesfeed",                &GetSubscribesFeed,             {"topHeight","topContentHash","countOut","lang","tags","contentTypes","txIdsExcluded","adrsExcluded","tagsExcluded","address", "address_feed"}},
    {"contents",        "getrawtransactionwithmessage",     &FeedSelector,                  {"address_from", "address_to", "start_txid", "count", "lang", "tags", "contenttypes"}},
    {"contents",        "getrawtransactionwithmessagebyid", &GetContent,                    {"ids", "address"}},
    {"contents",        "getcontent",                       &GetContent,                    {"ids", "address"}},
    {"contents",        "getcontentsstatistic",             &GetContentsStatistic,          {"addresses", "contentTypes", "height", "depth"}},
    {"contents",        "getcontents",                      &GetContents,                   {"address"}},
    {"contents",        "getrandomcontents",                &GetRandomContents,             {}},

    // Tags
//    {"artifacts", "searchtags",                       &gettemplate,                       {"search_string", "count"}},
    {"tags",           "gettags",                          &GetTags,                        {"address", "count", "height", "lang"}},

    // Comments
    {"comments",        "getcomments",                      &GetCommentsByPost,             {"postid", "parentid", "address", "ids"}},
    {"comments",        "getlastcomments",                  &GetLastComments,               {"count", "address"}},

    // Accounts
    {"accounts",        "getuserprofile",                   &GetAccountProfiles,             {"addresses", "short"}},
    {"accounts",        "getuseraddress",                   &GetAccountAddress,              {"name"}},
    {"accounts",        "getaddressregistration",           &GetAccountRegistration,         {"addresses"}},
    {"accounts",        "getuserstate",                     &GetAccountState,                {"address"}},
    {"accounts",        "txunspent",                        &GetAccountUnspents,             {"addresses", "minconf", "maxconf", "include_unsafe", "query_options"}},
    {"accounts",        "getaddressid",                     &GetAccountId,                   {"address_or_id"}},
    {"accounts",        "getaccountsetting",                &GetAccountSetting,              {"address"}},
    {"accounts",        "getuserstatistic",                 &GetAccountStatistic,            {"address", "height", "depth"}},
    {"accounts",        "getusersubscribes",                &GetAccountSubscribes,           {"address", "height", "depth"}},
    {"accounts",        "getusersubscribers",               &GetAccountSubscribers,          {"address", "height", "depth"}},
    {"accounts",        "getuserblockings",                 &GetAccountBlockings,            {"address", "height", "depth"}},

    // Scores
    {"scores",          "getaddressscores",                 &GetAddressScores,              {"address", "txs"}},
    {"scores",          "getpostscores",                    &GetPostScores,                 {"txHash"}},
    {"scores",          "getpagescores",                    &GetPagesScores,                {"postIds", "address", "cmntIds"}},

    // Explorer
    {"explorer",       "getstatisticbyhours",              &GetStatisticByHours,            {"topHeight", "depth"}},
    {"explorer",       "getstatisticbydays",               &GetStatisticByDays,             {"topHeight", "depth"}},
    {"explorer",       "getstatisticcontentbyhours",       &GetStatisticContentByHours,     {"topHeight", "depth"}},
    {"explorer",       "getstatisticcontentbydays",        &GetStatisticContentByDays,      {"topHeight", "depth"}},
    {"explorer",       "getaddressinfo",                   &GetAddressInfo,                 {"address"}},
    {"explorer",       "getcompactblock",                  &GetCompactBlock,                {"blockHash"}},
    {"explorer",       "getlastblocks",                    &GetLastBlocks,                  {"count", "lastHeight", "verbose"}},
    {"explorer",       "searchbyhash",                     &SearchByHash,                   {"value"}},
    {"explorer",       "gettransactions",                  &GetTransactions,                {"transactions"}},
    {"explorer",       "getaddresstransactions",           &GetAddressTransactions,         {"address", "pageStart", "pageSize"}},
    {"explorer",       "getblocktransactions",             &GetBlockTransactions,           {"blockHash", "pageStart", "pageSize"}},
    {"explorer",       "getbalancehistory",                &GetBalanceHistory,              {"address", "topHeight", "count"}},

    // System
    {"system",         "getpeerinfo",                      &GetPeerInfo,                    {}},
    {"system",         "getnodeinfo",                      &GetNodeInfo,                    {}},
    {"system",         "gettime",                          &GetTime,                        {}},
    {"system",         "getcoininfo",                      &GetCoinInfo,                    {"height"}},

    // Transactions
    {"transaction",    "getrawtransaction",                &GetTransaction,                 {"transactions"}},
    {"transaction",    "estimatesmartfee",                 &EstimateSmartFee,               {"conf_target", "estimate_mode"} },
};
// @formatter:on

// @formatter:off
static const CRPCCommand commands_post[] =
{
    {"transaction",    "sendrawtransactionwithmessage",    &AddTransaction,                 {"hexstring", "message"}},
    {"transaction",    "addtransaction",                   &AddTransaction,                 {"hexstring", "message"}},
    {"transaction",    "sendrawtransaction",               &AddTransaction,                 {"hexstring", "message"}},
};
// @formatter:on

void RegisterPocketnetWebRPCCommands(CRPCTable &tableRPC, CRPCTable &tablePostRPC)
{
    for (const auto& command : commands)
        tableRPC.appendCommand(command.name, &command);

    for (const auto& command : commands_post)
        tablePostRPC.appendCommand(command.name, &command);
}
