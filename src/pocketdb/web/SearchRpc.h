// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef SRC_SEARCH_RPC_H
#define SRC_SEARCH_RPC_H

#include "rpc/server.h"
#include "util/html.h"
#include "pocketdb/helpers/TransactionHelper.h"
#include "pocketdb/models/base/PocketTypes.h"
#include "pocketdb/models/web/SearchRequest.h"
#include "pocketdb/web/WebRpcUtils.h"

namespace PocketWeb::PocketWebRpc
{
    using namespace PocketDbWeb;
    using namespace PocketTx;

    RPCHelpMan Search();
    RPCHelpMan SearchUsers();
    RPCHelpMan SearchLinks();
    RPCHelpMan SearchContents();

    #pragma region Recomendations
    // Accounts recommendations based on subscriptions
    // Get some accounts that were followed by people who've followed this Account
    // This should be run only if Account already has followers
    RPCHelpMan GetRecomendedAccountsBySubscriptions();

    // Accounts recommendations based on scores on other Account
    // Get some Accounts that were scored by people who've scored this Account  (not long ago - several blocks ago)
    RPCHelpMan GetRecomendedAccountsByScoresOnSimilarAccounts();

    // Accounts recommendations based on address scores
    // Get some Accounts that were scored by people who've scored like address  (not long ago - several blocks ago)
    // Get address scores -> Get scored contents -> Get Scores to these contents -> Get scores accounts -> Get their scores -> Get their scored contents -> Get these contents Authors
    RPCHelpMan GetRecomendedAccountsByScoresFromAddress();

    // Accounts recommendations based on tags
    RPCHelpMan GetRecomendedAccountsByTags();

    // Contents recommendations by others contents
    // Get some contents that were liked by people who've seen this content (not long ago - several blocks ago)
    // This should be run only if Content already has >XXXX likes
    RPCHelpMan GetRecomendedContentsByScoresOnSimilarContents();

    // Contents recommendations by address scores
    // Get some contents that were liked by people who've scored like address  (not long ago - several blocks ago)
    RPCHelpMan GetRecomendedContentsByScoresFromAddress();
    #pragma endregion
}

#endif //SRC_SEARCH_RPC_H
