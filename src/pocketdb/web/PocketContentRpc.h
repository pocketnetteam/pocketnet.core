// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef SRC_POCKETDEBUG_H
#define SRC_POCKETDEBUG_H

#include "rpc/server.h"
#include "validation.h"
#include "pocketdb/helpers/TransactionHelper.h"
#include "pocketdb/web/WebRpcUtils.h"
#include "pocketdb/consensus/Reputation.h"

namespace PocketWeb::PocketWebRpc
{
    using namespace PocketHelpers;
    using namespace PocketConsensus;

    RPCHelpMan GetContent();
    RPCHelpMan GetContents();
    RPCHelpMan GetHotPosts();
    RPCHelpMan GetHistoricalFeed();
    RPCHelpMan GetHierarchicalFeed();
    RPCHelpMan GetBoostFeed();
    RPCHelpMan GetTopFeed();
    RPCHelpMan GetMostCommentedFeed();
    RPCHelpMan GetProfileFeed();
    RPCHelpMan GetProfileCollections();
    RPCHelpMan GetSubscribesFeed();
    RPCHelpMan GetContentsStatistic();
    RPCHelpMan GetRandomContents();
    RPCHelpMan GetContentActions();
    RPCHelpMan GetEvents(); // TODO (losty): probably move to another place
    RPCHelpMan GetNotifications();
    RPCHelpMan GetActivities();
    RPCHelpMan GetNotificationsSummary();
}

#endif //SRC_POCKETDEBUG_H
