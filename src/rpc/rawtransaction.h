// Copyright (c) 2017-2018 The Pocketcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef POCKETCOIN_RPC_RAWTRANSACTION_H
#define POCKETCOIN_RPC_RAWTRANSACTION_H

#include <chain.h>
#include <coins.h>
#include <compat/byteswap.h>
#include <consensus/validation.h>
#include <core_io.h>
#include <index/txindex.h>
#include <key_io.h>
#include <keystore.h>
#include <merkleblock.h>
#include <net.h>
#include <policy/policy.h>
#include <policy/rbf.h>
#include <primitives/transaction.h>
#include <rpc/server.h>
#include <script/script.h>
#include <script/script_error.h>
#include <script/sign.h>
#include <script/standard.h>
#include <txmempool.h>
#include <uint256.h>
#include <utilstrencodings.h>
#include <validation.h>
#include <validationinterface.h>

#include <future>
#include <stdint.h>

#include <univalue.h>

#include "antibot/antibot.h"
#include "html.h"
#include "index/addrindex.h"

class CBasicKeyStore;
struct CMutableTransaction;
class UniValue;

/** Sign a transaction with the given keystore and previous transactions */
UniValue SignTransaction(CMutableTransaction& mtx, const UniValue& prevTxs, CBasicKeyStore *keystore, bool tempKeystore, const UniValue& hashType);

/** Create a transaction from univalue parameters */
CMutableTransaction ConstructTransaction(const UniValue& inputs_in, const UniValue& outputs_in, const UniValue& locktime, const UniValue& rbf);

#endif // POCKETCOIN_RPC_RAWTRANSACTION_H
