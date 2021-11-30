// Copyright (c) 2017 The Pocketcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef POCKETCOIN_RPC_MINING_H
#define POCKETCOIN_RPC_MINING_H

#include <script/script.h>

#include <univalue.h>
#include <rpc/server.h>

/** Default max iterations to try in RPC generatetodescriptor, generatetoaddress, and generateblock. */
static const uint64_t DEFAULT_MAX_TRIES{1000000};

RPCHelpMan estimatesmartfee();

#endif
