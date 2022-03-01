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
    const auto ToMemPool = [this](const CMutableTransaction& tx, const PTransactionRef ptx) {
        LOCK(cs_main);
        LOCK(m_node.mempool->cs);

        TxValidationState state;
        return AcceptToMemoryPool(*m_node.mempool, state, MakeTransactionRef(tx), ptx,
            nullptr /* plTxnReplaced */, true /* bypass_limits */);
    };

	CScript scriptPubKey = CScript() <<  ToByteVector(coinbaseKey.GetPubKey()) << OP_CHECKSIG;
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


    // Create a ACCOUNT_USER transaction
	CMutableTransaction mTx;
	mTx.nVersion = 1;
	mTx.vin.resize(1);
	// Create a transaction with input from earlier block
	mTx.vin[0].prevout.hash = m_coinbase_txns[0]->GetHash();
	mTx.vin[0].prevout.n = 0;
	mTx.vout.resize(1);
	mTx.vout[0].nValue = 11*CENT;
	mTx.vout[0].scriptPubKey = scriptPubKey;

	// Sign:
	std::vector<unsigned char> vchSig;
	uint256 hash = SignatureHash(scriptPubKey, mTx, 0, SIGHASH_ALL, 0, SigVersion::BASE);
	BOOST_CHECK(coinbaseKey.Sign(hash, vchSig));
	vchSig.push_back((unsigned char)SIGHASH_ALL);
	mTx.vin[0].scriptSig << vchSig;

	const auto tx = MakeTransactionRef(mTx);

	auto txOutput = PocketDb::TransRepoInst.GetTxOutput(tx->vin[0].prevout.hash.GetHex(), tx->vin[0].prevout.n);
	if (!txOutput) throw JSONRPCError(RPC_INVALID_PARAMS, "Invalid address");
	string address = *txOutput->GetAddressHash();

	UniValue pocketData;
	pocketData.pushKV("r", "fakeaddress");
	pocketData.pushKV("l", "en");
	pocketData.pushKV("n", "bob");
	pocketData.pushKV("a", "bob avatar");
	pocketData.pushKV("s", "url");
	pocketData.pushKV("k", "bob pubkey");
	pocketData.pushKV("b", "bob donations");
	
	// Deserialize incoming data
	auto[deserializeOk, ptx] = PocketServices::Serializer::DeserializeTransactionRpc(tx, pocketData);
	if (!deserializeOk)
		throw JSONRPCError(RPC_DESERIALIZATION_ERROR, "TX deserialize failed");

	// Set required fields
	ptx->SetAddress(address);

	// Insert into mempool
	//return _accept_transaction(tx, ptx, *node.mempool, *node.connman); // TODO (losty-fur): possible null
	BOOST_CHECK(ToMemPool(mTx, ptx));


}

BOOST_AUTO_TEST_SUITE_END()