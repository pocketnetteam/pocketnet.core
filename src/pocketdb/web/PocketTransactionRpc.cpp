// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include "pocketdb/web/PocketTransactionRpc.h"
#include "consensus/validation.h"
#include "primitives/transaction.h"
#include "rpc/blockchain.h"
#include "script/signingprovider.h"
#include "util/rbf.h"

namespace PocketWeb::PocketWebRpc
{
    RPCHelpMan AddTransaction()
    {
        return RPCHelpMan{"addtransaction",
                "\nGet transaction data.\n"
                "in BIP 141 (witness data is discounted).\n",
                {
                    // TODO (losty-fur): provide arguments description
                },
                {
                    // TODO (losty-fur): provide return description
                },
                RPCExamples{
                    "" // TODO (losty-fur): provide examples
                },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
    {
        auto& node = EnsureNodeContext(request.context);

        RPCTypeCheck(request.params, {UniValue::VSTR, UniValue::VOBJ});

        CMutableTransaction mTx;
        if (!DecodeHexTx(mTx, request.params[0].get_str()))
            throw JSONRPCError(RPC_DESERIALIZATION_ERROR, "TX decode failed");

        const auto tx = MakeTransactionRef(move(mTx));

        auto txOutput = request.DbConnection()->TransactionRepoInst->GetTxOutput(
            tx->vin[0].prevout.hash.GetHex(), tx->vin[0].prevout.n);
        if (!txOutput) throw JSONRPCError(RPC_INVALID_PARAMS, "Invalid address");
        string address = *txOutput->GetAddressHash();

        // Deserialize incoming data
        auto[deserializeOk, ptx] = PocketServices::Serializer::DeserializeTransactionRpc(tx, request.params[1]);
        if (!deserializeOk)
            throw JSONRPCError(RPC_DESERIALIZATION_ERROR, "TX deserialize failed");

        // Set required fields
        ptx->SetAddress(address);

        // Insert into mempool
        return _accept_transaction(tx, ptx, *node.mempool, *node.connman); // TODO (losty-fur): possible null
    },
        };
    }
    
    RPCHelpMan EstimateSmartFee()
    {
        return estimatesmartfee();
    }

    RPCHelpMan GetTransaction()
    {
        return RPCHelpMan{"getrawtransaction",
                "\nGet transaction data.\n"
                "in BIP 141 (witness data is discounted).\n",
                {
                    // TODO (losty-fur): provide arguments description
                },
                {
                    // TODO (losty-fur): provide return description
                },
                RPCExamples{
                    "" // TODO (losty-fur): provide examples
                },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
    {
        RPCTypeCheck(request.params, {UniValue::VSTR});
        string txid = request.params[0].get_str();

        UniValue txs = request.DbConnection()->ExplorerRepoInst->GetTransactions({ txid }, 1, 2);
        if (txs.empty())
            return UniValue(UniValue::VOBJ);

        return txs[0];
    },
        };
    }

    CMutableTransaction ConstructPocketnetTransaction(const UniValue& inputs_in, const CTxOut& dataOut,
        const UniValue& outputs_in, const UniValue& locktime, const UniValue& rbf)
    {
        if (inputs_in.isNull() || outputs_in.isNull())
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid parameter, arguments 1 and 2 must be non-null");

        UniValue inputs = inputs_in.get_array();
        const bool outputs_is_obj = outputs_in.isObject();
        UniValue outputs = outputs_is_obj ? outputs_in.get_obj() : outputs_in.get_array();

        CMutableTransaction rawTx;

        if (!locktime.isNull()) {
            int64_t nLockTime = locktime.get_int64();
            if (nLockTime < 0 || nLockTime > std::numeric_limits<uint32_t>::max())
                throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid parameter, locktime out of range");
            rawTx.nLockTime = nLockTime;
        }

        bool rbfOptIn = rbf.isTrue();

        for (unsigned int idx = 0; idx < inputs.size(); idx++) {
            const UniValue& input = inputs[idx];
            const UniValue& o = input.get_obj();

            uint256 txid = ParseHashO(o, "txid");

            const UniValue& vout_v = find_value(o, "vout");
            if (!vout_v.isNum())
                throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid parameter, missing vout key");
            int nOutput = vout_v.get_int();
            if (nOutput < 0)
                throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid parameter, vout must be positive");

            uint32_t nSequence;
            if (rbfOptIn) {
                nSequence = MAX_BIP125_RBF_SEQUENCE;
            } else if (rawTx.nLockTime) {
                nSequence = std::numeric_limits<uint32_t>::max() - 1;
            } else {
                nSequence = std::numeric_limits<uint32_t>::max();
            }

            // set the sequence number if passed in the parameters object
            const UniValue& sequenceObj = find_value(o, "sequence");
            if (sequenceObj.isNum()) {
                int64_t seqNr64 = sequenceObj.get_int64();
                if (seqNr64 < 0 || seqNr64 > std::numeric_limits<uint32_t>::max()) {
                    throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid parameter, sequence number is out of range");
                } else {
                    nSequence = (uint32_t)seqNr64;
                }
            }

            CTxIn in(COutPoint(txid, nOutput), CScript(), nSequence);

            rawTx.vin.push_back(in);
        }

        std::set<CTxDestination> destinations;
        if (!outputs_is_obj) {
            // Translate array of key-value pairs into dict
            UniValue outputs_dict = UniValue(UniValue::VOBJ);
            for (size_t i = 0; i < outputs.size(); ++i) {
                const UniValue& output = outputs[i];
                if (!output.isObject()) {
                    throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid parameter, key-value pair not an object as expected");
                }
                if (output.size() != 1) {
                    throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid parameter, key-value pair must contain exactly one key");
                }
                outputs_dict.pushKVs(output);
            }
            outputs = std::move(outputs_dict);
        }

        // Push zero data output
        rawTx.vout.push_back(dataOut);

        // Push address outputs
        for (const std::string& name_ : outputs.getKeys()) {
            CTxDestination destination = DecodeDestination(name_);
            if (!IsValidDestination(destination))
                throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, std::string("Invalid Pocketcoin address: ") + name_);
            if (!destinations.insert(destination).second)
                throw JSONRPCError(RPC_INVALID_PARAMETER, std::string("Invalid parameter, duplicated address: ") + name_);

            CScript scriptPubKey = GetScriptForDestination(destination);
            CAmount nAmount = outputs[name_].get_int64();

            CTxOut out(nAmount, scriptPubKey);
            rawTx.vout.push_back(out);
        }

        if (!rbf.isNull() && rawTx.vin.size() > 0 && rbfOptIn != SignalsOptInRBF(CTransaction(rawTx))) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid parameter combination: Sequence number(s) contradict replaceable option");
        }

        if(rawTx.nTime == 0)
            rawTx.nTime = GetAdjustedTime();

        return rawTx;
    }

    RPCHelpMan GenerateTransaction()
    {
        return RPCHelpMan{"generatetransaction",
                "\nAdd new pocketnet transaction.\n",
                {
                    // TODO (losty-fur): provide arguments description
                },
                {
                    // TODO (losty-fur): provide return description
                },
                RPCExamples{
                    "" // TODO (losty-fur): provide examples
                },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
    {
        if (Params().NetworkIDString() != CBaseChainParams::TESTNET)
            throw runtime_error("Only for testnet\n");

        RPCTypeCheck(request.params, {UniValue::VSTR, UniValue::VARR, UniValue::VNUM, UniValue::VSTR, UniValue::VOBJ});

        // Address for use in transaction
        string address = request.params[0].get_str();
        CTxDestination destAddress = DecodeDestination(address);
        if (!IsValidDestination(destAddress)) {
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid address");
        }

        // Private key for signing
        UniValue privKeys = request.params[1].get_array();

        // Count of outputs for new transaction
        int outputCount = request.params[2].get_int();

        // Get pocketnet transaction type string
        string txTypeHex = request.params[3].get_str();

        // Get payload object
        UniValue txPayload = request.params[4].get_obj();

        // Build template for transaction
        auto txType = PocketHelpers::TransactionHelper::ConvertOpReturnToType(txTypeHex);
        shared_ptr<Transaction> _ptx = PocketHelpers::TransactionHelper::CreateInstance(txType);
        if (!_ptx) throw JSONRPCError(RPC_PARSE_ERROR, "Failed create pocketnet transaction payload");

        // Deserialize params
        _ptx->SetHash("");
        _ptx->DeserializeRpc(txPayload);

        // Get unspents
        vector<pair<string, uint32_t>> mempoolInputs;
        vector<string> addresses {address};
        auto unsp = request.DbConnection()->WebRpcRepoInst->GetUnspents(addresses, ChainActive().Height(), mempoolInputs);

        // Build inputs
        UniValue _inputs(UniValue::VARR);
        _inputs.push_back(unsp[0]);

        // Build outputs
        UniValue _outputs(UniValue::VARR);
        auto totalAmount = unsp[0]["amountSat"].get_int64();
        auto chunkAmount = totalAmount / outputCount;
        for (int i = 0; i < outputCount; i++)
        {
            UniValue _output_address(UniValue::VOBJ);
            _output_address.pushKV(address, chunkAmount - (i + 1 == outputCount ? 1 : 0));
            _outputs.push_back(_output_address);
        }

        // generate transaction
        CTxOut _dataOut(0, CScript() << OP_RETURN << ParseHex(txTypeHex) << ParseHex(_ptx->BuildHash()));
        CMutableTransaction mTx = ConstructPocketnetTransaction(_inputs, _dataOut, _outputs, NullUniValue, NullUniValue);

        // Decode private keys
        FillableSigningProvider keystore; // TODO (losty-critical): CBasicKeyStore removed.
        for (unsigned int idx = 0; idx < privKeys.size(); ++idx) {
            UniValue k = privKeys[idx];
            CKey key = DecodeSecret(k.get_str());
            if (!key.IsValid())
                throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid private key");
            keystore.AddKey(key);
        }

        // Try sign transaction
        UniValue signResult /* = SignTransaction(mTx, NullUniValue, &keystore, true, NullUniValue)*/;  // TODO (losty-critical): SignTransaction changed. Required vector of coins 
        if (!signResult["complete"].get_bool()) {
            if (signResult.exists("errors"))
                throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid sign: " + signResult["errors"].write());
            else
                throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid sign without error");
        }

        // Build new transaction
        const auto tx = MakeTransactionRef(move(mTx));

        auto[deserializeOk, ptx] = PocketServices::Serializer::DeserializeTransactionRpc(tx, txPayload);
        if (!deserializeOk)
            throw JSONRPCError(RPC_DESERIALIZATION_ERROR, "TX deserialize failed");

        // Set required fields
        ptx->SetAddress(address);

        const auto& node = EnsureNodeContext(request.context);

        // TODO (losty-fur): possible null mempool and connman
        // Insert into mempool
        return _accept_transaction(tx, ptx, *node.mempool, *node.connman);
    },
        };
    }

    UniValue _accept_transaction(const CTransactionRef& tx, const PTransactionRef& ptx, CTxMemPool& mempool, CConnman& connman)
    {
        promise<void> promise;
        // CAmount nMaxRawTxFee = maxTxFee; // TODO (losty-fur): validate corresponding check is performed in wallet by using walletInstance->m_default_max_tx_fee
        const uint256& txid = tx->GetHash();

        { // cs_main scope
            LOCK(cs_main);

            CCoinsViewCache& view = ::ChainstateActive().CoinsTip();
            bool fHaveChain = false;
            for (size_t o = 0; !fHaveChain && o < tx->vout.size(); o++)
            {
                const Coin& existingCoin = view.AccessCoin(COutPoint(txid, o));
                fHaveChain = !existingCoin.IsSpent();
            }
            
            bool fHaveMempool = mempool.exists(txid);

            if (!fHaveMempool && !fHaveChain)
            {
                // push to local node and sync with wallets
                TxValidationState state;
                if (!AcceptToMemoryPool(mempool, state, tx, ptx,
                    nullptr /* plTxnReplaced */, false /* bypass_limits */))
                {
                    if (state.IsConsensusFailed())
                    {
                        // TODO (losty-critical): GetRejectCode removed
                        // throw JSONRPCError(state.GetRejectCode(), FormatStateMessage(state));
                    }
                    else if (state.IsInvalid())
                    {
                        throw JSONRPCError(RPC_TRANSACTION_REJECTED, state.ToString());
                    }
                    else
                    {
                        // TODO (losty+): GetRejectCode is removed. Probably need to add POCKETTX_MATURITY to TxValidationResult, but it seems this code is never using
                        // RPC_POCKETTX_MATURITY используется в src/consensus/tx_verify.cpp:224 + нужно поднять это условие выше if (state.IsInvalid())
                        // также необходим мерж с текущими изменениями
                        // if (state.GetRejectCode() == RPC_POCKETTX_MATURITY)
                        // {
                            throw JSONRPCError(RPC_POCKETTX_MATURITY, state.ToString());
                        // }
                        // else
                        // {
                        //     throw JSONRPCError(RPC_TRANSACTION_ERROR, FormatStateMessage(state));
                        // }
                    }
                }
                else
                {
                    // If wallet is enabled, ensure that the wallet has been made aware
                    // of the new transaction prior to returning. This prevents a race
                    // where a user might call sendrawtransaction with a transaction
                    // to/from their wallet, immediately call some wallet RPC, and get
                    // a stale result because callbacks have not yet been processed.
                    CallFunctionInValidationInterfaceQueue([&promise] {
                        promise.set_value();
                    });
                }
            }
            else if (fHaveChain)
            {
                throw JSONRPCError(RPC_TRANSACTION_ALREADY_IN_CHAIN, "transaction already in block chain");
            }
            else
            {
                // Make sure we don't block forever if re-sending
                // a transaction already in mempool.
                promise.set_value();
            }
        } // cs_main

        promise.get_future().wait();

        CInv inv(MSG_TX, txid);
        connman.ForEachNode([&inv](CNode* pnode) {
            // TODO (losty-fur): Validate this is working
            pnode->PushTxInventory(inv.hash);
        });

        return txid.GetHex();
    }
}