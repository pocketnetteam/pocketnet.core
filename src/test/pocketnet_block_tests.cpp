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
#include "pocketdb/services/Serializer.h"
#include "pocketdb/pocketnet.h"

#include <boost/test/unit_test.hpp>

const std::string GENESIS_HASH("aced9fcaba8f66be3d4d51a95cb048dda6611b8f2d2bf4541d9e2e16c07ee1c9");

BOOST_AUTO_TEST_SUITE(pocketnet_block_tests)

static UniValue verifydb(CTxMemPool &mempool)
{
    UniValue result(UniValue::VARR);
    uint256 tx_hash;
    uint256 block_hash;

    LOCK(cs_main);
    LOCK(mempool.cs);
    auto hashes = PocketDb::ChainRepoInst.GetTransactionHashes();

    for (auto &hash : hashes)
    {
        // Make sure each transaction exists in either the blockchain or the mempool
        tx_hash = uint256S(hash);
        CTransactionRef tx_ref = GetTransaction(nullptr, &mempool, tx_hash, Params().GetConsensus(), block_hash);
        if (tx_ref == nullptr)
        {
            result.push_back(tx_hash.ToString());
        }
    }

    return result;
}

BOOST_FIXTURE_TEST_CASE(pocketnet_block_rollback, TestChain100Setup)
{
    const CChainParams& chainparams = Params();
    BlockValidationState state;
	bool ret = true;

    BOOST_CHECK(ActivateBestChain(state, chainparams));
    UniValue result = verifydb(*m_node.mempool);
	BOOST_CHECK(result[0].get_str() == GENESIS_HASH);

	const std::vector<unsigned char> op_true{OP_TRUE};
    uint256 witness_program;

    const auto ToMemPool = [this](const CMutableTransaction& tx) {
        LOCK(cs_main);
        LOCK(m_node.mempool->cs);

        auto transaction = new CTransaction(tx);
        CTransactionRef txref(transaction);

        auto[ok, pocketTx] = PocketServices::Serializer::DeserializeTransaction(txref);
        BOOST_CHECK(ok);

        TxValidationState state;
        return AcceptToMemoryPool(*m_node.mempool, state, MakeTransactionRef(tx), pocketTx,
            nullptr /* plTxnReplaced */, true /* bypass_limits */);
    };

    CSHA256().Write(&op_true[0], op_true.size()).Finalize(witness_program.begin());

    const CScript SCRIPT_PUB{CScript(OP_0) << std::vector<unsigned char>{witness_program.begin(), witness_program.end()}};

	BOOST_CHECK(InvalidateBlock(state, chainparams, ::ChainActive().Tip()) );

    result = verifydb(*m_node.mempool);
	BOOST_CHECK(result[0].get_str() == GENESIS_HASH);

    CScript scriptPubKey = CScript() <<  ToByteVector(coinbaseKey.GetPubKey()) << OP_CHECKSIG;

    BOOST_CHECK(ActivateBestChain(state, Params()));

    // Create 2block with no transactions to get us to the correct spend height, 100 block coinbase maturity is required before we spend coins
    std::vector<CMutableTransaction> noTxns;

    CBlock block = CreateAndProcessBlock(noTxns, scriptPubKey);
    BOOST_CHECK(ActivateBestChain(state, Params()));

    block = CreateAndProcessBlock(noTxns, scriptPubKey);
    BOOST_CHECK(ActivateBestChain(state, Params()));

    std::vector<CMutableTransaction> spends;
	int txcount = 2;
    spends.resize(txcount);
    for (int i = 0; i < txcount; i++)
	{
        spends[i].nVersion = 1;
        spends[i].vin.resize(1);
        // Create a transaction with input from earlier block
        spends[i].vin[0].prevout.hash = m_coinbase_txns[i]->GetHash();
        spends[i].vin[0].prevout.n = 0;
        spends[i].vout.resize(1);
        spends[i].vout[0].nValue = 11*CENT;
        spends[i].vout[0].scriptPubKey = scriptPubKey;

        // Sign:
        std::vector<unsigned char> vchSig;
        uint256 hash = SignatureHash(scriptPubKey, spends[i], 0, SIGHASH_ALL, 0, SigVersion::BASE);
        BOOST_CHECK(coinbaseKey.Sign(hash, vchSig));
        vchSig.push_back((unsigned char)SIGHASH_ALL);
        spends[i].vin[0].scriptSig << vchSig;
    }

    // Test 1: make sure this block is accepted.
    block = CreateAndProcessBlock(spends, scriptPubKey);

    result = verifydb(*m_node.mempool);
	BOOST_CHECK(result[0].get_str() == GENESIS_HASH);

    BOOST_CHECK(ActivateBestChain(state, Params()));
    BOOST_CHECK(::ChainActive().Tip()->GetBlockHash() == block.GetHash());

    result = verifydb(*m_node.mempool);
	BOOST_CHECK(result[0].get_str() == GENESIS_HASH);

	BOOST_CHECK(CVerifyDB().VerifyDB(Params(), &::ChainstateActive().CoinsTip(), 4, 102));


    // Generate 4 new blocks so we can spend earlier transactions with 100 block coinbase maturity
    CreateAndProcessBlock(noTxns, scriptPubKey);
    CreateAndProcessBlock(noTxns, scriptPubKey);
    CreateAndProcessBlock(noTxns, scriptPubKey);
    block = CreateAndProcessBlock(noTxns, scriptPubKey);
	BOOST_CHECK(::ChainActive().Tip()->GetBlockHash() == block.GetHash());


	// Add transaction to mempool, verify it's in Transactions database

    std::vector<CMutableTransaction> spends1;
	txcount = 2;
    spends1.resize(txcount);
    for (int i = 0; i < txcount; i++)
	{
        spends1[i].nVersion = 1;
        spends1[i].vin.resize(1);
        // Create a transaction with input from earlier block
        spends1[i].vin[0].prevout.hash = m_coinbase_txns[i+2]->GetHash();
        spends1[i].vin[0].prevout.n = 0;
        spends1[i].vout.resize(1);
        spends1[i].vout[0].nValue = 11*CENT;
        spends1[i].vout[0].scriptPubKey = scriptPubKey;

        // Sign:
        std::vector<unsigned char> vchSig;
        uint256 hash = SignatureHash(scriptPubKey, spends1[i], 0, SIGHASH_ALL, 0, SigVersion::BASE);
        BOOST_CHECK(coinbaseKey.Sign(hash, vchSig));
        vchSig.push_back((unsigned char)SIGHASH_ALL);
        spends1[i].vin[0].scriptSig << vchSig;
    }

    BOOST_CHECK(ToMemPool(spends1[0]));
    BOOST_CHECK(ToMemPool(spends1[1]));

	result = verifydb(*m_node.mempool);
	BOOST_CHECK(result[0].get_str() == GENESIS_HASH);

    block = CreateAndProcessBlock(noTxns, scriptPubKey);
	BOOST_CHECK(ActivateBestChain(state, Params()));
    BOOST_CHECK(::ChainActive().Tip()->GetBlockHash() == block.GetHash());

	result = verifydb(*m_node.mempool);
	BOOST_CHECK(result.size() == 1 || result[0].get_str() == GENESIS_HASH);

	// Eject from mempool, verify Transaction database again

	// Add transaction to mempool, then accept it into block, verify db

	// Rollback block

	// Add block with transactions, then second block which takes inputs from previous, then rollback

	// expire mempool
}

BOOST_AUTO_TEST_SUITE_END()
