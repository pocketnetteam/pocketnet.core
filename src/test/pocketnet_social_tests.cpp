// Copyright (c) 2022 The Pocketcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <consensus/validation.h>
#include <key.h>
#include <txmempool.h>
#include <validation.h>
#include <random.h>
#include <script/sign.h>
#include <script/signingprovider.h>
#include <script/standard.h>
#include <test/util/setup_common.h>
#include <core_io.h>
#include <policy/policy.h>
#include "pocketdb/web/PocketRpc.h"
#include "pocketdb/services/Serializer.h"
#include "pocketdb/pocketnet.h"

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(pocketnet_social_tests)

BOOST_FIXTURE_TEST_CASE(pocketnet_ratings, TestChain100Setup)
{
	RPCHelpMan rpc = GenerateAddress();
	UniValue keys;
	keys.setArray();
	UniValue key;
	key.setObject();
	key.pushKV("wallet", "testwallet");
	key.pushKV("scriptPubKey", HexStr(GetScriptForRawPubKey(coinbaseKey.GetPubKey())));
	key.pushKV("timestamp", 0);
	key.pushKV("internal", UniValue(true));
	keys.push_back(key);
	key.clear();
	key.setObject();
	CKey futureKey;
	futureKey.MakeNewKey(true);
	key.pushKV("scriptPubKey", HexStr(GetScriptForRawPubKey(futureKey.GetPubKey())));
	//key.pushKV("timestamp", GetTime() + TIMESTAMP_WINDOW + 1);
	key.pushKV("internal", UniValue(true));
	keys.push_back(key);
	util::Ref context;
	JSONRPCRequest request(context);
	request.params.setArray();
	request.params.push_back(keys);

	//rpc.HandleRequest(request);
	//BOOST_CHECK(value != nullptr);
}

BOOST_AUTO_TEST_SUITE_END()