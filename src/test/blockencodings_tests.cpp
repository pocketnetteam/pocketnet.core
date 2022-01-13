// Copyright (c) 2011-2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <blockencodings.h>
#include <consensus/merkle.h>
#include <chainparams.h>
#include <script/sign.h>
#include <pow.h>
#include <random.h>
#include <key.h>
#include <keystore.h>

#include <test/test_pocketcoin.h>

#include <boost/test/unit_test.hpp>

#include <validation.h>
#include <consensus/validation.h>

std::vector<std::pair<uint256, CTransactionRef>> extra_txn;

struct RegtestingSetup : public TestingSetup {
    RegtestingSetup() : TestingSetup(CBaseChainParams::REGTEST) {}
};

BOOST_FIXTURE_TEST_SUITE(blockencodings_tests, RegtestingSetup)

static CBlock BuildBlockTestCase() {
    CBlock block;
    CMutableTransaction tx;
    CBasicKeyStore keystore;
    CKey key;

    // Add key to the keystore:
    key.MakeNewKey(0);
    keystore.AddKey(key);

    tx.vin.resize(1);
    tx.vin[0].scriptSig.resize(10);
    tx.vout.resize(1);
    tx.vout[0].nValue = 500000;

    block.vtx.resize(3);
    block.vtx[0] = MakeTransactionRef(tx);
    block.nVersion = 42;
    block.hashPrevBlock = InsecureRand256();
    block.nBits = 0x207fffff;

    tx.vin[0].prevout.hash = InsecureRand256();
    tx.vin[0].prevout.n = 0;
    // PocketNet coinstake transaction requires at least 2 outputs
    tx.vout.resize(2);
    tx.vout[0].nValue=0; 
    tx.vout[1].nValue=50;
    tx.vout[1].scriptPubKey << ToByteVector(key.GetPubKey()) << OP_CHECKSIG;
    block.vtx[1] = MakeTransactionRef(tx);

    tx.vin.resize(10);
    for (size_t i = 0; i < tx.vin.size(); i++) {
        tx.vin[i].prevout.hash = InsecureRand256();
        tx.vin[i].prevout.n = 0;
    }
    tx.vout[0].nValue = 1000;
    block.vtx[2] = MakeTransactionRef(tx);

    bool mutated;
    block.hashMerkleRoot = BlockMerkleRoot(block, &mutated);
    assert(!mutated);
    while (!CheckProofOfWork(block.GetHash(), block.nBits, Params().GetConsensus(), 0)) ++block.nNonce;
    
    // Sign block now that it is assembled 
    key.Sign(block.GetHash(), block.vchBlockSig);
    return block;
}

// Number of shared use_counts we expect for a tx we haven't touched
// (block + mempool + our copy from the GetSharedTx call)
constexpr long SHARED_TX_OFFSET{3};

BOOST_AUTO_TEST_CASE(SimpleRoundTripTest)
{
    CTxMemPool pool;
    TestMemPoolEntryHelper entry;
    CBlock block(BuildBlockTestCase());

    CValidationState state;
    BOOST_CHECK_MESSAGE(CheckBlock(block, state, Params().GetConsensus()), "CheckBlock of initial block failed!");
    BOOST_TEST_MESSAGE(block.ToString());

    LOCK(pool.cs);
    pool.addUnchecked(entry.FromTx(block.vtx[2]));
    BOOST_CHECK_EQUAL(pool.mapTx.find(block.vtx[2]->GetHash())->GetSharedTx().use_count(), SHARED_TX_OFFSET + 0);

    BOOST_TEST_MESSAGE("Transaction record vtx[0] = \n");
    BOOST_TEST_MESSAGE(block.vtx[0]->ToString());

    BOOST_TEST_MESSAGE("Transaction record vtx[1] = \n");
    BOOST_TEST_MESSAGE(block.vtx[1]->ToString());

    BOOST_TEST_MESSAGE("Transaction record vtx[2] = \n");
    BOOST_TEST_MESSAGE(block.vtx[2]->ToString());

    // Do a simple ShortTxIDs RT
    {
        CBlockHeaderAndShortTxIDs shortIDs(block, true);

        CDataStream stream(SER_NETWORK, PROTOCOL_VERSION);
        stream << shortIDs;

        CBlockHeaderAndShortTxIDs shortIDs2;
        stream >> shortIDs2;

        BOOST_CHECK_EQUAL_COLLECTIONS(shortIDs.vchBlockSig.begin(), shortIDs.vchBlockSig.end(), shortIDs2.vchBlockSig.begin(), shortIDs2.vchBlockSig.end());

        PartiallyDownloadedBlock partialBlock(&pool);
        BOOST_CHECK_MESSAGE(partialBlock.InitData(shortIDs2, extra_txn) == READ_STATUS_OK, "InitData failed");
        BOOST_CHECK_MESSAGE( partialBlock.IsTxAvailable(0), "TX 0 not available");
        BOOST_CHECK_MESSAGE(!partialBlock.IsTxAvailable(1), "TX 1 present when it should not be");
        BOOST_CHECK_MESSAGE( partialBlock.IsTxAvailable(2), "TX 2 not availabe");

        BOOST_CHECK_EQUAL(pool.mapTx.find(block.vtx[2]->GetHash())->GetSharedTx().use_count(), SHARED_TX_OFFSET + 1);

        size_t poolSize = pool.size();
        pool.removeRecursive(*block.vtx[2], MemPoolRemovalReason::REPLACED);
        BOOST_CHECK_EQUAL(pool.size(), poolSize - 1);

        CBlock block2;
        {
            PartiallyDownloadedBlock tmp = partialBlock;
            BOOST_CHECK(partialBlock.FillBlock(block2, {}) == READ_STATUS_INVALID); // No transactions
            partialBlock = tmp;
        }

        // Wrong transaction
        {
            PartiallyDownloadedBlock tmp = partialBlock;
            partialBlock.FillBlock(block2, {block.vtx[2]}); // Current implementation doesn't check txn here, but don't require that
            partialBlock = tmp;
        }
        bool mutated;
        BOOST_CHECK(block.hashMerkleRoot != BlockMerkleRoot(block2, &mutated));

        BOOST_TEST_MESSAGE(block.vtx[1]->ToString());
        CBlock block3;
        BOOST_CHECK(partialBlock.FillBlock(block3, {block.vtx[1]}) == READ_STATUS_OK);
        BOOST_CHECK_EQUAL(block.GetHash().ToString(), block3.GetHash().ToString());
        BOOST_CHECK_EQUAL(block.hashMerkleRoot.ToString(), BlockMerkleRoot(block3, &mutated).ToString());
        BOOST_CHECK(!mutated);
    }
}

class TestHeaderAndShortIDs {
    // Utility to encode custom CBlockHeaderAndShortTxIDs
public:
    CBlockHeader header;
    uint64_t nonce;
    std::vector<unsigned char> vchBlockSig;
    std::vector<uint64_t> shorttxids;
    std::vector<PrefilledTransaction> prefilledtxn;

    explicit TestHeaderAndShortIDs(const CBlockHeaderAndShortTxIDs& orig) {
        CDataStream stream(SER_NETWORK, PROTOCOL_VERSION);
        stream << orig;
        stream >> *this;
    }
    explicit TestHeaderAndShortIDs(const CBlock& block) :
        TestHeaderAndShortIDs(CBlockHeaderAndShortTxIDs(block, true)) {}

    uint64_t GetShortID(const uint256& txhash) const {
        CDataStream stream(SER_NETWORK, PROTOCOL_VERSION);
        stream << *this;
        CBlockHeaderAndShortTxIDs base;
        stream >> base;
        return base.GetShortID(txhash);
    }

    SERIALIZE_METHODS(TestHeaderAndShortIDs, obj) { READWRITE(obj.header, obj.nonce, Using<VectorFormatter<CustomUintFormatter<CBlockHeaderAndShortTxIDs::SHORTTXIDS_LENGTH>>>(obj.shorttxids), obj.prefilledtxn, obj.vchBlockSig); }
};

BOOST_AUTO_TEST_CASE(NonCoinbasePreforwardRTTest)
{
    CTxMemPool pool;
    TestMemPoolEntryHelper entry;
    CBlock block(BuildBlockTestCase());

    LOCK(pool.cs);
    pool.addUnchecked(entry.FromTx(block.vtx[2]));
    BOOST_CHECK_EQUAL(pool.mapTx.find(block.vtx[2]->GetHash())->GetSharedTx().use_count(), SHARED_TX_OFFSET + 0);
    uint256 txhash;

    // Test with pre-forwarding tx 1, but not coinbase
    {
        TestHeaderAndShortIDs shortIDs(block);

        shortIDs.prefilledtxn.resize(1);
        shortIDs.prefilledtxn[0] = {1, block.vtx[1]};
        shortIDs.shorttxids.resize(2);
        shortIDs.shorttxids[0] = shortIDs.GetShortID(block.vtx[0]->GetHash());
        shortIDs.shorttxids[1] = shortIDs.GetShortID(block.vtx[2]->GetHash());

        CDataStream stream(SER_NETWORK, PROTOCOL_VERSION);
        stream << shortIDs;

        CBlockHeaderAndShortTxIDs shortIDs2;
        stream >> shortIDs2;

        PartiallyDownloadedBlock partialBlock(&pool);
        BOOST_CHECK(partialBlock.InitData(shortIDs2, extra_txn) == READ_STATUS_OK);
        BOOST_CHECK(!partialBlock.IsTxAvailable(0));
        BOOST_CHECK( partialBlock.IsTxAvailable(1));
        BOOST_CHECK( partialBlock.IsTxAvailable(2));

        BOOST_CHECK_EQUAL(pool.mapTx.find(block.vtx[2]->GetHash())->GetSharedTx().use_count(), SHARED_TX_OFFSET + 1); // +1 because of partialBlock

        CBlock block2;
        {
            PartiallyDownloadedBlock tmp = partialBlock;
            BOOST_CHECK(partialBlock.FillBlock(block2, {}) == READ_STATUS_INVALID); // No transactions
            partialBlock = tmp;
        }

        // Wrong transaction
        {
            PartiallyDownloadedBlock tmp = partialBlock;
            partialBlock.FillBlock(block2, {block.vtx[1]}); // Current implementation doesn't check txn here, but don't require that
            partialBlock = tmp;
        }
        BOOST_CHECK_EQUAL(pool.mapTx.find(block.vtx[2]->GetHash())->GetSharedTx().use_count(), SHARED_TX_OFFSET + 2); // +2 because of partialBlock and block2
        bool mutated;
        BOOST_CHECK(block.hashMerkleRoot != BlockMerkleRoot(block2, &mutated));

        CBlock block3;
        PartiallyDownloadedBlock partialBlockCopy = partialBlock;
        BOOST_CHECK(partialBlock.FillBlock(block3, {block.vtx[0]}) == READ_STATUS_OK);
        BOOST_CHECK_EQUAL(block.GetHash().ToString(), block3.GetHash().ToString());
        BOOST_CHECK_EQUAL(block.hashMerkleRoot.ToString(), BlockMerkleRoot(block3, &mutated).ToString());
        BOOST_CHECK(!mutated);

        BOOST_CHECK_EQUAL(pool.mapTx.find(block.vtx[2]->GetHash())->GetSharedTx().use_count(), SHARED_TX_OFFSET + 3); // +2 because of partialBlock and block2 and block3

        txhash = block.vtx[2]->GetHash();
        block.vtx.clear();
        block2.vtx.clear();
        block3.vtx.clear();
        BOOST_CHECK_EQUAL(pool.mapTx.find(txhash)->GetSharedTx().use_count(), SHARED_TX_OFFSET + 1 - 1); // + 1 because of partialBlock; -1 because of block.
    }
    BOOST_CHECK_EQUAL(pool.mapTx.find(txhash)->GetSharedTx().use_count(), SHARED_TX_OFFSET - 1); // -1 because of block
}

BOOST_AUTO_TEST_CASE(SufficientPreforwardRTTest)
{
    CTxMemPool pool;
    TestMemPoolEntryHelper entry;
    CBlock block(BuildBlockTestCase());

    LOCK(pool.cs);
    pool.addUnchecked(entry.FromTx(block.vtx[1]));
    BOOST_CHECK_EQUAL(pool.mapTx.find(block.vtx[1]->GetHash())->GetSharedTx().use_count(), SHARED_TX_OFFSET + 0);

    uint256 txhash;

    // Test with pre-forwarding coinbase + tx 2 with tx 1 in mempool
    {
        TestHeaderAndShortIDs shortIDs(block);
        shortIDs.prefilledtxn.resize(2);
        shortIDs.prefilledtxn[0] = {0, block.vtx[0]};
        shortIDs.prefilledtxn[1] = {1, block.vtx[2]}; // id == 1 as it is 1 after index 1
        shortIDs.shorttxids.resize(1);
        shortIDs.shorttxids[0] = shortIDs.GetShortID(block.vtx[1]->GetHash());

        CDataStream stream(SER_NETWORK, PROTOCOL_VERSION);
        stream << shortIDs;

        CBlockHeaderAndShortTxIDs shortIDs2;
        stream >> shortIDs2;

        PartiallyDownloadedBlock partialBlock(&pool);
        BOOST_CHECK(partialBlock.InitData(shortIDs2, extra_txn) == READ_STATUS_OK);
        BOOST_CHECK( partialBlock.IsTxAvailable(0));
        BOOST_CHECK( partialBlock.IsTxAvailable(1));
        BOOST_CHECK( partialBlock.IsTxAvailable(2));

        BOOST_CHECK_EQUAL(pool.mapTx.find(block.vtx[1]->GetHash())->GetSharedTx().use_count(), SHARED_TX_OFFSET + 1);

        CBlock block2;
        PartiallyDownloadedBlock partialBlockCopy = partialBlock;
        BOOST_CHECK(partialBlock.FillBlock(block2, {}) == READ_STATUS_OK);
        BOOST_CHECK_EQUAL(block.GetHash().ToString(), block2.GetHash().ToString());
        bool mutated;
        BOOST_CHECK_EQUAL(block.hashMerkleRoot.ToString(), BlockMerkleRoot(block2, &mutated).ToString());
        BOOST_CHECK(!mutated);

        txhash = block.vtx[1]->GetHash();
        block.vtx.clear();
        block2.vtx.clear();
        BOOST_CHECK_EQUAL(pool.mapTx.find(txhash)->GetSharedTx().use_count(), SHARED_TX_OFFSET + 1 - 1); // + 1 because of partialBlock; -1 because of block.
    }
    BOOST_CHECK_EQUAL(pool.mapTx.find(txhash)->GetSharedTx().use_count(), SHARED_TX_OFFSET - 1); // -1 because of block
}

BOOST_AUTO_TEST_CASE(EmptyBlockRoundTripTest)
{
    CTxMemPool pool;
    CMutableTransaction coinbase;
    coinbase.vin.resize(1);
    coinbase.vin[0].scriptSig.resize(10);
    coinbase.vout.resize(1);
    coinbase.vout[0].nValue = 42;

    CBlock block;
    block.vtx.resize(1);
    block.vtx[0] = MakeTransactionRef(std::move(coinbase));
    block.nVersion = 42;
    block.hashPrevBlock = InsecureRand256();
    block.nBits = 0x207fffff;

    bool mutated;
    block.hashMerkleRoot = BlockMerkleRoot(block, &mutated);
    assert(!mutated);
    while (!CheckProofOfWork(block.GetHash(), block.nBits, Params().GetConsensus(), 0)) ++block.nNonce;

    // Test simple header round-trip with only coinbase
    {
        CBlockHeaderAndShortTxIDs shortIDs(block, false);

        CDataStream stream(SER_NETWORK, PROTOCOL_VERSION);
        stream << shortIDs;

        CBlockHeaderAndShortTxIDs shortIDs2;
        stream >> shortIDs2;

        PartiallyDownloadedBlock partialBlock(&pool);
        BOOST_CHECK(partialBlock.InitData(shortIDs2, extra_txn) == READ_STATUS_OK);
        BOOST_CHECK(partialBlock.IsTxAvailable(0));

        CBlock block2;
        std::vector<CTransactionRef> vtx_missing;
        BOOST_CHECK(partialBlock.FillBlock(block2, vtx_missing) == READ_STATUS_OK);
        BOOST_CHECK_EQUAL(block.GetHash().ToString(), block2.GetHash().ToString());
        BOOST_CHECK_EQUAL(block.hashMerkleRoot.ToString(), BlockMerkleRoot(block2, &mutated).ToString());
        BOOST_CHECK(!mutated);
    }
}

BOOST_AUTO_TEST_CASE(TransactionsRequestSerializationTest) {
    BlockTransactionsRequest req1;
    req1.blockhash = InsecureRand256();
    req1.indexes.resize(4);
    req1.indexes[0] = 0;
    req1.indexes[1] = 1;
    req1.indexes[2] = 3;
    req1.indexes[3] = 4;

    CDataStream stream(SER_NETWORK, PROTOCOL_VERSION);
    stream << req1;

    BlockTransactionsRequest req2;
    stream >> req2;

    BOOST_CHECK_EQUAL(req1.blockhash.ToString(), req2.blockhash.ToString());
    BOOST_CHECK_EQUAL(req1.indexes.size(), req2.indexes.size());
    BOOST_CHECK_EQUAL(req1.indexes[0], req2.indexes[0]);
    BOOST_CHECK_EQUAL(req1.indexes[1], req2.indexes[1]);
    BOOST_CHECK_EQUAL(req1.indexes[2], req2.indexes[2]);
    BOOST_CHECK_EQUAL(req1.indexes[3], req2.indexes[3]);
}

BOOST_AUTO_TEST_SUITE_END()
