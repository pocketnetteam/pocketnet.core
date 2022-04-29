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
#include "pocketdb/repositories/web/WebRpcRepository.h"
#include "pocketdb/SQLiteConnection.h"
#include "websocket/notifyprocessor.h"

#include <boost/test/unit_test.hpp>


namespace pocketnet_social_tests {
struct PocketNetTestingSetup : public TestChain100Setup {
    unordered_map<std::string, CKey> keyMap;
    int tx_index;
    bool ToMemPool(const CMutableTransaction& tx, const PTransactionRef ptx);
    std::string GetAccountAddress(std::string name);
    CTransactionRef SendCoins(std::string name);
    CTransactionRef CreateAccount(std::string name, CTransactionRef txIn);
    CTransactionRef CreatePost(std::string name, CTransactionRef txIn);
    CTransactionRef CreateAdPost(std::string name1, std::string name2, CTransactionRef txIn);
    bool AcceptPost(std::string name, CTransactionRef txIn);
    bool RetractAdPost(std::string name, CTransactionRef txIn);
};
}


BOOST_FIXTURE_TEST_SUITE(pocketnet_social_tests, PocketNetTestingSetup)

const UniValue NullUniValue;

std::string PocketNetTestingSetup::GetAccountAddress(std::string name)
{
    CPubKey pubkey = keyMap[name].GetPubKey();
    CTxDestination dest = GetDestinationForKey(pubkey, OutputType::LEGACY);
    return EncodeDestination(dest);
}

bool PocketNetTestingSetup::ToMemPool(const CMutableTransaction& tx, const PTransactionRef ptx)
{
    LOCK(cs_main);
    LOCK(m_node.mempool->cs);

    TxValidationState state;
    return AcceptToMemoryPool(*m_node.mempool, state, MakeTransactionRef(tx), ptx,
        nullptr /* plTxnReplaced */, false /* bypass_limits */);
};

bool PocketNetTestingSetup::RetractAdPost(std::string name, CTransactionRef txIn)
{

    unsigned int flags = SCRIPT_VERIFY_STRICTENC | SCRIPT_VERIFY_CHECKSEQUENCEVERIFY;
    CAmount amount = 0;
    ScriptError err;

    // Create a CONTENT_POST transaction
    CMutableTransaction mTx;
    // TX version 2 required for OP_CHECKSEQUENCEVERIFY
    mTx.nVersion = 2;
    mTx.vin.resize(1);
    // Create a transaction with input from earlier block
    mTx.vin[0].nSequence = 10;
    mTx.vin[0].prevout.hash = txIn->GetHash();
    mTx.vin[0].prevout.n = 1;
    mTx.vout.resize(1);
    amount = 6*CENT;
    mTx.vout[0].nValue = amount;
    mTx.vout[0].scriptPubKey = CScript() << OP_DUP << OP_HASH160 << ToByteVector(keyMap[name].GetPubKey().GetID()) << OP_EQUALVERIFY << OP_CHECKSIG;

    // Sign:
    std::vector<unsigned char> vchSig;
    uint256 hash = SignatureHash(txIn->vout[1].scriptPubKey, mTx, 0, SIGHASH_ALL, 0, SigVersion::BASE);
    BOOST_CHECK(keyMap[name].Sign(hash, vchSig));
    vchSig.push_back((unsigned char)SIGHASH_ALL);
    mTx.vin[0].scriptSig << vchSig << ToByteVector(keyMap[name].GetPubKey()) << OP_FALSE;

    if (!VerifyScript(mTx.vin[0].scriptSig, txIn->vout[1].scriptPubKey, nullptr, flags, MutableTransactionSignatureChecker(&mTx, 0, amount), &err))
        return false;

    CTransactionRef tx = MakeTransactionRef(mTx);

    auto[ok, pocketTx] = PocketServices::Serializer::DeserializeTransaction(tx);
    BOOST_CHECK(ok);

    // Insert into mempool
    return ToMemPool(mTx, pocketTx);
}

bool PocketNetTestingSetup::AcceptPost(std::string name, CTransactionRef txIn)
{
    unsigned char* opData = (unsigned char*) "share";
    std::vector<unsigned char> op(opData, opData + sizeof("share") - 1);
    unsigned int flags = SCRIPT_VERIFY_STRICTENC;
    CAmount amount = 0;
    ScriptError err;

    // An CONTENT_POST payload is required in order to accept and AdPost offer.  If the payload is not present in the transaction
    // will be rejected from the mempool
    UniValue pocketData;
    pocketData.setObject();
    pocketData.pushKV("l", "en");
    pocketData.pushKV("c", "");
    pocketData.pushKV("m", "repost%20Checkout%20buy%20bobs%20stuff");
    pocketData.pushKV("u", "");
    pocketData.pushKV("s", "{\"a\":\"\",\"v\":\"\",\"videos\":[],\"image\":\"\",\"f\":\"0\"}");
    pocketData.pushKV("t", "[\"tag1\",\"tag2\"]");
    pocketData.pushKV("i", "[]");
	shared_ptr<Transaction> _ptx = PocketHelpers::TransactionHelper::CreateInstance(PocketTx::CONTENT_POST);

    PocketBlockRef adBlock = PocketDb::TransRepoInst.List(std::vector<std::string>{txIn->GetHash().ToString()}, true, false, false);
    BOOST_CHECK(*(*adBlock)[0]->GetType() == PocketTx::ACTION_AD_POST);
    PocketConsensus::AdPostRef adPost = static_pointer_cast<AdPost>((*adBlock)[0]);
    std::shared_ptr<std::string> postTxHash = adPost->GetContentTxHash();
    pocketData.pushKV("txidRepost", *postTxHash);
    _ptx->SetHash("");
    _ptx->DeserializeRpc(pocketData);


    CMutableTransaction mTx;
    mTx.nVersion = 2;
    mTx.vin.resize(1);
    // Create a transaction with input from earlier block
    mTx.vin[0].prevout.hash = txIn->GetHash();
    mTx.vin[0].prevout.n = 1;

    mTx.vout.resize(2);
    mTx.vout[0].nValue = 0;
    mTx.vout[0].scriptPubKey = CScript() << OP_RETURN << op << ParseHex(_ptx->BuildHash());
    amount = 6*CENT;
    mTx.vout[1].nValue = amount;
    mTx.vout[1].scriptPubKey = CScript() << OP_DUP << OP_HASH160 << ToByteVector(keyMap[name].GetPubKey().GetID()) << OP_EQUALVERIFY << OP_CHECKSIG;

    // Sign:
    std::vector<unsigned char> vchSig;
    uint256 hash = SignatureHash(txIn->vout[1].scriptPubKey, mTx, 0, SIGHASH_ALL, 0, SigVersion::BASE);
    BOOST_CHECK(keyMap[name].Sign(hash, vchSig));
    vchSig.push_back((unsigned char)SIGHASH_ALL);
    mTx.vin[0].scriptSig << vchSig << ToByteVector(keyMap[name].GetPubKey()) << ToByteVector(std::string("ad")) << OP_TRUE;

    const auto tx = MakeTransactionRef(mTx);

    // Deserialize incoming data
    auto[deserializeOk, ptx] = PocketServices::Serializer::DeserializeTransactionRpc(tx, pocketData);
    if (!deserializeOk)
        throw JSONRPCError(RPC_DESERIALIZATION_ERROR, "TX deserialize failed");

    ptx->SetAddress(GetAccountAddress(name));

    BOOST_CHECK(VerifyScript(mTx.vin[0].scriptSig, txIn->vout[1].scriptPubKey, nullptr, flags, MutableTransactionSignatureChecker(&mTx, 0, amount), &err));

    // Insert into mempool
    return ToMemPool(mTx, ptx);
}

CTransactionRef PocketNetTestingSetup::CreateAdPost(std::string name1, std::string name2, CTransactionRef txIn)
{
    unsigned char* opData = (unsigned char*) "adpost";
    std::vector<unsigned char> op(opData, opData + sizeof("adpost") - 1);

    unsigned int flags = SCRIPT_VERIFY_STRICTENC;
    ScriptError err;
    CAmount amount = 0;

    UniValue pocketData;
    pocketData.setObject();
    pocketData.pushKV("c", txIn->GetHash().GetHex());
    pocketData.pushKV("t", GetAccountAddress(name2));
	shared_ptr<Transaction> _ptx = PocketHelpers::TransactionHelper::CreateInstance(PocketTx::ACTION_AD_POST);
    _ptx->SetHash("");
    _ptx->DeserializeRpc(pocketData);

    // Create a CONTENT_POST transaction
    CMutableTransaction mTx;
    mTx.nVersion = 2;
    mTx.vin.resize(1);
    // Create a transaction with input from earlier block
    mTx.vin[0].prevout.hash = txIn->GetHash();
    mTx.vin[0].prevout.n = 1;
    mTx.vout.resize(2);
    mTx.vout[0].nValue = 0;
    mTx.vout[0].scriptPubKey = CScript() << OP_RETURN << op << ParseHex(_ptx->BuildHash());
    amount = 7*CENT;
    mTx.vout[1].nValue = amount;
    mTx.vout[1].scriptPubKey = CScript() << 
                                OP_IF << 
                                    ToByteVector(std::string("ad")) << OP_EQUALVERIFY <<
                                    OP_DUP << OP_HASH160 << ToByteVector(keyMap[name2].GetPubKey().GetID()) << OP_EQUALVERIFY << OP_CHECKSIG <<
                                OP_ELSE << 
                                    10 << OP_CHECKSEQUENCEVERIFY << OP_DROP <<
                                    OP_DUP << OP_HASH160 << ToByteVector(keyMap[name1].GetPubKey().GetID()) << OP_EQUALVERIFY << OP_CHECKSIG <<
                                OP_ENDIF;

    // Sign:
    std::vector<unsigned char> vchSig;
    uint256 hash = SignatureHash(txIn->vout[1].scriptPubKey, mTx, 0, SIGHASH_ALL, 0, SigVersion::BASE);
    BOOST_CHECK(keyMap[name1].Sign(hash, vchSig));
    vchSig.push_back((unsigned char)SIGHASH_ALL);
    mTx.vin[0].scriptSig << vchSig << ToByteVector(keyMap[name1].GetPubKey());

    BOOST_CHECK(VerifyScript(mTx.vin[0].scriptSig, txIn->vout[1].scriptPubKey, nullptr, flags, MutableTransactionSignatureChecker(&mTx, 0, amount), &err));

    const auto tx = MakeTransactionRef(mTx);
    
    // Deserialize incoming data
    auto[deserializeOk, ptx] = PocketServices::Serializer::DeserializeTransactionRpc(tx, pocketData);
    if (!deserializeOk)
        throw JSONRPCError(RPC_DESERIALIZATION_ERROR, "TX deserialize failed");

    ptx->SetAddress(GetAccountAddress(name1));

    BOOST_CHECK(ToMemPool(mTx, ptx));

    return tx;
}

CTransactionRef  PocketNetTestingSetup::CreatePost(std::string name, CTransactionRef txIn)
{
    unsigned char* opData = (unsigned char*) "share";
    std::vector<unsigned char> op(opData, opData + sizeof("share") - 1);

    unsigned int flags = SCRIPT_VERIFY_STRICTENC;
    CAmount amount = 0;
    ScriptError err;

    UniValue pocketData;
    pocketData.setObject();
    pocketData.pushKV("l", "en");
    pocketData.pushKV("c", name + " caption");
    pocketData.pushKV("m", name + " post message");
    pocketData.pushKV("u", name + ".com");
    pocketData.pushKV("s", name + "settings");
    pocketData.pushKV("t", "[\"tag1\",\"tag2\"]");
    pocketData.pushKV("i", name + " images");
	shared_ptr<Transaction> _ptx = PocketHelpers::TransactionHelper::CreateInstance(PocketTx::CONTENT_POST);
    _ptx->SetHash("");
    _ptx->DeserializeRpc(pocketData);

    // Create a CONTENT_POST transaction
    CMutableTransaction mTx;
    mTx.nVersion = 1;
    mTx.vin.resize(1);
    // Create a transaction with input from earlier block
    mTx.vin[0].prevout.hash = txIn->GetHash();
    mTx.vin[0].prevout.n = 1;
    mTx.vout.resize(2);
    mTx.vout[0].nValue = 0;
    mTx.vout[0].scriptPubKey = CScript() << OP_RETURN << op << ParseHex(_ptx->BuildHash());
    amount = 8*CENT;
    mTx.vout[1].nValue = amount;
    mTx.vout[1].scriptPubKey = CScript() << OP_DUP << OP_HASH160 << ToByteVector(keyMap[name].GetPubKey().GetID()) << OP_EQUALVERIFY << OP_CHECKSIG;

    // Sign:
    std::vector<unsigned char> vchSig;
    uint256 hash = SignatureHash(txIn->vout[1].scriptPubKey, mTx, 0, SIGHASH_ALL, 0, SigVersion::BASE);
    BOOST_CHECK(keyMap[name].Sign(hash, vchSig));
    vchSig.push_back((unsigned char)SIGHASH_ALL);
    mTx.vin[0].scriptSig << vchSig << ToByteVector(keyMap[name].GetPubKey());

    BOOST_CHECK(VerifyScript(mTx.vin[0].scriptSig, txIn->vout[1].scriptPubKey, nullptr, flags, MutableTransactionSignatureChecker(&mTx, 0, amount), &err));

    const auto tx = MakeTransactionRef(mTx);
    
    // Deserialize incoming data
    auto[deserializeOk, ptx] = PocketServices::Serializer::DeserializeTransactionRpc(tx, pocketData);
    if (!deserializeOk)
        throw JSONRPCError(RPC_DESERIALIZATION_ERROR, "TX deserialize failed");

    ptx->SetAddress(GetAccountAddress(name));

    // Insert into mempool
    BOOST_CHECK(ToMemPool(mTx, ptx));

    return tx;
}


CTransactionRef PocketNetTestingSetup::CreateAccount(std::string name, CTransactionRef txIn)
{
    unsigned char* opData = (unsigned char*) "userInfo";
    std::vector<unsigned char> op(opData, opData + sizeof("userInfo") - 1);

    unsigned int flags = SCRIPT_VERIFY_STRICTENC;
    CAmount amount = 0;
    ScriptError err;

    UniValue pocketData;
    pocketData.setObject();
    pocketData.pushKV("r", keyMap[name].GetPubKey().GetID().ToString());
    pocketData.pushKV("l", "en");
    pocketData.pushKV("n", name.c_str());
    pocketData.pushKV("a", name + " avatar");
    pocketData.pushKV("s", name + ".com");
    pocketData.pushKV("k", HexStr(GetScriptForRawPubKey(coinbaseKey.GetPubKey())).c_str());
    pocketData.pushKV("b", name + " donations");
	shared_ptr<Transaction> _ptx = PocketHelpers::TransactionHelper::CreateInstance(PocketTx::ACCOUNT_USER);
    _ptx->SetHash("");
    _ptx->DeserializeRpc(pocketData);

    // Create a ACCOUNT_USER transaction
    CMutableTransaction mTx;
    mTx.nVersion = 1;
    mTx.vin.resize(1);
    // Create a transaction with input from earlier block
    mTx.vin[0].prevout.hash = txIn->GetHash();
    mTx.vin[0].prevout.n = 0;
    mTx.vout.resize(2);
    mTx.vout[0].nValue = 0;
    mTx.vout[0].scriptPubKey = CScript() << OP_RETURN << op << ParseHex(_ptx->BuildHash());
    amount = 9*CENT;
    mTx.vout[1].nValue = amount;
    mTx.vout[1].scriptPubKey = CScript() << OP_DUP << OP_HASH160 << ToByteVector(keyMap[name].GetPubKey().GetID()) << OP_EQUALVERIFY << OP_CHECKSIG;

    // Sign:
    std::vector<unsigned char> vchSig;
    uint256 hash = SignatureHash(txIn->vout[0].scriptPubKey, mTx, 0, SIGHASH_ALL, 0, SigVersion::BASE);
    BOOST_CHECK(keyMap[name].Sign(hash, vchSig));
    vchSig.push_back((unsigned char)SIGHASH_ALL);
    mTx.vin[0].scriptSig << vchSig << ToByteVector(keyMap[name].GetPubKey());

    BOOST_CHECK(VerifyScript(mTx.vin[0].scriptSig, txIn->vout[0].scriptPubKey, nullptr, flags, MutableTransactionSignatureChecker(&mTx, 0, amount), &err));

    const auto tx = MakeTransactionRef(mTx);

    // Deserialize incoming data
    auto[deserializeOk, ptx] = PocketServices::Serializer::DeserializeTransactionRpc(tx, pocketData);
    if (!deserializeOk)
        throw JSONRPCError(RPC_DESERIALIZATION_ERROR, "TX deserialize failed");
 
    ptx->SetAddress(GetAccountAddress(name));

    // Insert into mempool
    BOOST_CHECK(ToMemPool(mTx, ptx));

    return tx;
}

// Send coins from the coinbase account to a new user account
CTransactionRef PocketNetTestingSetup::SendCoins(std::string name)
{
    CKey userKey;
    ScriptError err;
    CAmount amount = 0;
    unsigned int flags = SCRIPT_VERIFY_STRICTENC;

    userKey.MakeNewKey(true);
    keyMap[name] = userKey;

    CScript scriptPubKey = CScript() << OP_DUP << OP_HASH160 << ToByteVector(keyMap[name].GetPubKey().GetID()) << OP_EQUALVERIFY << OP_CHECKSIG;

    // Send coinbase coins to new account
    CMutableTransaction mTx;
    mTx.nVersion = 1;
    mTx.vin.resize(1);
    // Create a transaction with input from earlier block
    mTx.vin[0].scriptWitness = CScriptWitness();
    mTx.vin[0].prevout.hash = m_coinbase_txns[tx_index]->GetHash();
    mTx.vin[0].prevout.n = 0;
    mTx.vin[0].nSequence = CTxIn::SEQUENCE_FINAL;
    mTx.vout.resize(1);
    amount = 10*CENT;
    mTx.vout[0].nValue = amount;
    mTx.vout[0].scriptPubKey = scriptPubKey;

    // Sign:
    std::vector<unsigned char> vchSig;
    uint256 hash = SignatureHash(m_coinbase_txns[tx_index]->vout[0].scriptPubKey, mTx, 0, SIGHASH_ALL, amount, SigVersion::BASE);
    BOOST_CHECK(coinbaseKey.Sign(hash, vchSig));
    vchSig.push_back((unsigned char)SIGHASH_ALL);
    mTx.vin[0].scriptSig << vchSig;

    BOOST_CHECK(VerifyScript(mTx.vin[0].scriptSig, m_coinbase_txns[tx_index]->vout[0].scriptPubKey, nullptr, flags, MutableTransactionSignatureChecker(&mTx, 0, amount), &err));

    CTransactionRef tx = MakeTransactionRef(mTx);

    auto[ok, pocketTx] = PocketServices::Serializer::DeserializeTransaction(tx);
    BOOST_CHECK(ok);

    tx_index++;

    BOOST_CHECK(ToMemPool(mTx, pocketTx));

    return tx;
}

BOOST_AUTO_TEST_CASE(pocketnet_adpost_accept)
{
    std::vector<CMutableTransaction> noTxns;
    CScript scriptPubKey = CScript() <<  ToByteVector(coinbaseKey.GetPubKey()) << OP_CHECKSIG;
    BlockValidationState state;
    auto sqlConnection = std::make_shared<PocketDb::SQLiteConnection>();

    tx_index = 0;

    // Create 2 blocks with no transactions to get us to the correct spend height
    // 100 block coinbase maturity is required before we spend coins
    CBlock block = CreateAndProcessBlock(noTxns, scriptPubKey);
    BOOST_CHECK(ActivateBestChain(state, Params()));
    block = CreateAndProcessBlock(noTxns, scriptPubKey);
    BOOST_CHECK(ActivateBestChain(state, Params()));

    // Send some coins to Alice and Bob so they can create new acocunts 
    CTransactionRef aliceTx = SendCoins("alice");
    CTransactionRef bobTx = SendCoins("bob");

    // Create a new block and make sure both Bob and Alice's transactions are present
    block = CreateAndProcessMempoolBlock(*m_node.mempool, scriptPubKey);
    BOOST_CHECK(block.vtx.size() == 3);
    BOOST_CHECK(ActivateBestChain(state, Params()));
    BOOST_CHECK(::ChainActive().Tip()->GetBlockHash() == block.GetHash());

    // Create some empty blocks to ensure coin maturity
    block = CreateAndProcessBlock(noTxns, scriptPubKey);
    BOOST_CHECK(ActivateBestChain(state, Params()));
    block = CreateAndProcessBlock(noTxns, scriptPubKey);
    BOOST_CHECK(ActivateBestChain(state, Params()));
    block = CreateAndProcessBlock(noTxns, scriptPubKey);
    BOOST_CHECK(ActivateBestChain(state, Params()));
    block = CreateAndProcessBlock(noTxns, scriptPubKey);
    BOOST_CHECK(ActivateBestChain(state, Params()));

    // Create new account for Alice and Bob
    aliceTx = CreateAccount("alice", aliceTx);
    bobTx = CreateAccount("bob", bobTx);

    block = CreateAndProcessMempoolBlock(*m_node.mempool, scriptPubKey);
    BOOST_CHECK(block.vtx.size() == 3);
    BOOST_CHECK(ActivateBestChain(state, Params()));
    BOOST_CHECK(::ChainActive().Tip()->GetBlockHash() == block.GetHash());

    UniValue bobAddress = sqlConnection->WebRpcRepoInst->GetUserAddress("bob");
    UniValue addr = bobAddress[0].get_obj();
    UniValue a1 = find_value(addr, "address");
    std::vector<string> a2{a1.get_str()};
    auto bobProfile = sqlConnection->WebRpcRepoInst->GetAccountProfiles(a2);

    bobTx = CreatePost("bob", bobTx);
    CTransactionRef postTx = bobTx;

    block = CreateAndProcessMempoolBlock(*m_node.mempool, scriptPubKey);
    BOOST_CHECK(block.vtx.size() == 2);
    BOOST_CHECK(ActivateBestChain(state, Params()));
    BOOST_CHECK(::ChainActive().Tip()->GetBlockHash() == block.GetHash());

    UniValue contents = sqlConnection->WebRpcRepoInst->GetContentsForAddress(a1.get_str());

    bobTx = CreateAdPost("bob", "alice", bobTx);
    CTransactionRef adPostTx = bobTx;

    block = CreateAndProcessMempoolBlock(*m_node.mempool, scriptPubKey);
    BOOST_CHECK(block.vtx.size() == 2);
    BOOST_CHECK(ActivateBestChain(state, Params()));
    BOOST_CHECK(::ChainActive().Tip()->GetBlockHash() == block.GetHash());

    BOOST_CHECK(AcceptPost("alice", bobTx));

    block = CreateAndProcessMempoolBlock(*m_node.mempool, scriptPubKey);
    BOOST_CHECK(block.vtx.size() == 2);
    BOOST_CHECK(ActivateBestChain(state, Params()));
    BOOST_CHECK(::ChainActive().Tip()->GetBlockHash() == block.GetHash());

    // Bob attempting to redeem the transaction should fail because coins have already been redeemed
    BOOST_CHECK(RetractAdPost("bob", bobTx) == false);

    UniValue historicalFeed = sqlConnection->WebRpcRepoInst->GetHistoricalFeed(10, 0, ::ChainActive().Height(), "en",
        std::vector<string>(), {200, 201, 202}, std::vector<string>(), std::vector<string>(), std::vector<string>(), "", -20);

    // Verify both posts show up in the historical feed
    BOOST_CHECK(historicalFeed.size() == 1);
    UniValue bobPost = historicalFeed[0].get_obj();
    UniValue b = find_value(bobPost, "m");
    std::vector<string> message{b.get_str()};
    BOOST_CHECK(std::string("bob post message") == message[0]);
    // Verify repost
    UniValue repost = find_value(bobPost, "reposted");
    BOOST_CHECK(repost.get_int() == 1);

    // Verify repost shows up in Alice's feed
    UniValue profileFeed = sqlConnection->WebRpcRepoInst->GetProfileFeed(GetAccountAddress("alice"), 10, 0, ::ChainActive().Height(), "en",
        std::vector<string>(), {200, 201, 202}, std::vector<string>(), std::vector<string>(), std::vector<string>(), GetAccountAddress("alice"));
    UniValue repostComment = find_value(profileFeed[0].get_obj(), "m");
    std::vector<string> message2{repostComment.get_str()};
    BOOST_CHECK(std::string("repost%20Checkout%20buy%20bobs%20stuff") == message2[0]);

    auto response = PocketDb::NotifierRepoInst.GetAdPost(adPostTx->GetHash().GetHex());
    BOOST_CHECK(response["address"].get_str() == GetAccountAddress("bob"));
    BOOST_CHECK(response["contenttxhash"].get_str() == postTx->GetHash().GetHex());
    BOOST_CHECK(response["targetaddress"].get_str() == GetAccountAddress("alice"));

    // cleanup
    keyMap.erase("alice");
    keyMap.erase("bob");
}

BOOST_AUTO_TEST_CASE(pocketnet_adpost_rescind)
{
    std::vector<CMutableTransaction> noTxns;
    CScript scriptPubKey = CScript() <<  ToByteVector(coinbaseKey.GetPubKey()) << OP_CHECKSIG;
    BlockValidationState state;
    int i = 0;
    auto sqlConnection = std::make_shared<PocketDb::SQLiteConnection>();

    tx_index = 0;

    // Create 2 blocks with no transactions to get us to the correct spend height
    // 100 block coinbase maturity is required before we spend coins
    CBlock block = CreateAndProcessBlock(noTxns, scriptPubKey);
    BOOST_CHECK(ActivateBestChain(state, Params()));
    block = CreateAndProcessBlock(noTxns, scriptPubKey);
    BOOST_CHECK(ActivateBestChain(state, Params()));

    // Create Alice and Bob new acocunts 
    CTransactionRef aliceTx = SendCoins("alice");
    CTransactionRef bobTx = SendCoins("bob");

    block = CreateAndProcessMempoolBlock(*m_node.mempool, scriptPubKey);
    BOOST_CHECK(block.vtx.size() == 3);
    BOOST_CHECK(ActivateBestChain(state, Params()));
    BOOST_CHECK(::ChainActive().Tip()->GetBlockHash() == block.GetHash());
 
    block = CreateAndProcessBlock(noTxns, scriptPubKey);
    BOOST_CHECK(ActivateBestChain(state, Params()));
    block = CreateAndProcessBlock(noTxns, scriptPubKey);
    BOOST_CHECK(ActivateBestChain(state, Params()));
    block = CreateAndProcessBlock(noTxns, scriptPubKey);
    BOOST_CHECK(ActivateBestChain(state, Params()));
    block = CreateAndProcessBlock(noTxns, scriptPubKey);
    BOOST_CHECK(ActivateBestChain(state, Params()));

    aliceTx = CreateAccount("alice", aliceTx);
    bobTx = CreateAccount("bob", bobTx);

    block = CreateAndProcessMempoolBlock(*m_node.mempool, scriptPubKey);
    BOOST_CHECK(block.vtx.size() == 3);
    BOOST_CHECK(ActivateBestChain(state, Params()));
    BOOST_CHECK(::ChainActive().Tip()->GetBlockHash() == block.GetHash());

    UniValue bobAddress = sqlConnection->WebRpcRepoInst->GetUserAddress("bob");
    UniValue addr = bobAddress[0].get_obj();
    UniValue a1 = find_value(addr, "address");
    std::vector<string> a2{a1.get_str()};
    auto bobProfile = sqlConnection->WebRpcRepoInst->GetAccountProfiles(a2);

    bobTx = CreatePost("bob", bobTx);

    block = CreateAndProcessMempoolBlock(*m_node.mempool, scriptPubKey);
    BOOST_CHECK(block.vtx.size() == 2);
    BOOST_CHECK(ActivateBestChain(state, Params()));
    BOOST_CHECK(::ChainActive().Tip()->GetBlockHash() == block.GetHash());

    UniValue contents = sqlConnection->WebRpcRepoInst->GetContentsForAddress(a1.get_str());

    bobTx = CreateAdPost("bob", "alice", bobTx);

    block = CreateAndProcessMempoolBlock(*m_node.mempool, scriptPubKey);
    BOOST_CHECK(block.vtx.size() == 2);
    BOOST_CHECK(ActivateBestChain(state, Params()));
    BOOST_CHECK(::ChainActive().Tip()->GetBlockHash() == block.GetHash());

    //  Bob attempting to redeem the transaction should fail because coins cannot be redeemed until 10 blocks have passed
    BOOST_CHECK(RetractAdPost("bob", bobTx) == false);

    for (i = 0; i < 10; i++)
    {
        block = CreateAndProcessMempoolBlock(*m_node.mempool, scriptPubKey);
        BOOST_CHECK(ActivateBestChain(state, Params()));
        BOOST_CHECK(::ChainActive().Tip()->GetBlockHash() == block.GetHash());
    }

    //  Now it should work because 10 blocks have passed
    BOOST_CHECK(RetractAdPost("bob", bobTx) == true);
    block = CreateAndProcessMempoolBlock(*m_node.mempool, scriptPubKey);
    BOOST_CHECK(block.vtx.size() == 2);
    BOOST_CHECK(ActivateBestChain(state, Params()));
    BOOST_CHECK(::ChainActive().Tip()->GetBlockHash() == block.GetHash());

    // Alice attempting to redeem to ad post after Bob reclaims the coins should fail
    BOOST_CHECK(AcceptPost("alice", bobTx) == false);

    // cleanup
    keyMap.erase("alice");
    keyMap.erase("bob");
}

BOOST_AUTO_TEST_SUITE_END()