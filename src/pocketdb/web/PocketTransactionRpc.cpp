// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include "pocketdb/web/PocketTransactionRpc.h"
#include "pocketdb/consensus/social/AccountUser.hpp"

namespace PocketWeb::PocketWebRpc
{
    UniValue AddTransaction(const JSONRPCRequest& request)
    {
        if (request.fHelp)
            throw runtime_error(
                "addtransaction\n"
                "\nAdd new pocketnet transaction.\n"
            );

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
        ptx->SetString1(address);

        // TODO (team): TEMPORARY
        // Temporary check for UserConsensus_checkpoint_login_limitation checkpoint
        // Remove this after 1647000 in main net
        if (*ptx->GetType() == ACCOUNT_USER)
        {
            std::shared_ptr<PocketConsensus::AccountUserConsensus> accountUserConsensus = std::make_shared<PocketConsensus::AccountUserConsensus_checkpoint_login_limitation>(chainActive.Height());
            if (auto[ok, result] = accountUserConsensus->Check(tx, static_pointer_cast<User>(ptx)); !ok)
                throw JSONRPCError((int)result, strprintf("Failed SocialConsensusHelper::Check with result %d\n", (int)result));
        }

        // Insert into mempool
        return _accept_transaction(tx, ptx);
    }
    
    UniValue EstimateSmartFee(const JSONRPCRequest& request)
    {
        return estimatesmartfee(request);
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

            CScript scriptPubKey = GetScriptForDestination(destination);
            CAmount nAmount = outputs[name_].get_int64();

            CTxOut out(nAmount, scriptPubKey);
            rawTx.vout.push_back(out);
        }

        if (!rbf.isNull() && rawTx.vin.size() > 0 && rbfOptIn != SignalsOptInRBF(rawTx)) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid parameter combination: Sequence number(s) contradict replaceable option");
        }

        if(rawTx.nTime == 0)
            rawTx.nTime = GetAdjustedTime();

        return rawTx;
    }

    UniValue GenerateTransaction(const JSONRPCRequest& request)
    {
        if (request.fHelp)
            throw runtime_error(
                "generatetransaction\n"
                "\nAdd new pocketnet transaction.\n"
            );

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

        // Fee
        int64_t fee = 1;
        if (request.params[5].isNum())
            fee = request.params[5].get_int64();

        // Content Author address
        string contentAddressValue = "";
        if (request.params[6].isStr() && txPayload.exists("value") && txPayload["value"].isNum())
            contentAddressValue = request.params[6].get_str() + " " + to_string(txPayload["value"].get_int());

        // Build template for transaction
        auto txType = PocketHelpers::TransactionHelper::ConvertOpReturnToType(txTypeHex);
        shared_ptr<Transaction> _ptx = PocketHelpers::TransactionHelper::CreateInstance(txType);
        if (!_ptx) throw JSONRPCError(RPC_PARSE_ERROR, "Failed create pocketnet transaction payload");

        // Deserialize params
        _ptx->SetHash("");
        _ptx->DeserializeRpc(txPayload);

        // Get unspents
        vector<pair<string, uint32_t>> mempoolInputs;
        mempool.GetAllInputs(mempoolInputs);
        UniValue unsp = request.DbConnection()->WebRpcRepoInst->GetUnspents({ address }, chainActive.Height(), mempoolInputs);

        // Build inputs
        int64_t totalAmount = 0;
        UniValue _inputs(UniValue::VARR);
        int i = 0;
        while (totalAmount <= (fee + outputCount) && i < unsp.size())
        {
            totalAmount += unsp[i]["amountSat"].get_int64();
            _inputs.push_back(unsp[i]);
            i += 1;
        }

        // Build outputs
        UniValue _outputs(UniValue::VARR);
        int64_t returned = totalAmount - fee;
        int64_t chunkAmount = (totalAmount - fee) / outputCount;
        for (int i = 0; i < outputCount; i++)
        {
            returned -= chunkAmount;
            UniValue _output_address(UniValue::VOBJ);
            _output_address.pushKV(address, chunkAmount + (i + 1 == outputCount ? returned : 0));
            _outputs.push_back(_output_address);
        }

        // generate transaction
        auto _opReturn = CScript() << OP_RETURN << ParseHex(txTypeHex) << ParseHex(_ptx->BuildHash());
        if (contentAddressValue != "") _opReturn = _opReturn << ParseHex(HexStr(contentAddressValue));
        CTxOut _dataOut(0, _opReturn);
        CMutableTransaction mTx = ConstructPocketnetTransaction(_inputs, _dataOut, _outputs, NullUniValue, NullUniValue);

        // Decode private keys
        CBasicKeyStore keystore;
        for (unsigned int idx = 0; idx < privKeys.size(); ++idx) {
            UniValue k = privKeys[idx];
            CKey key = DecodeSecret(k.get_str());
            if (!key.IsValid())
                throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid private key");
            keystore.AddKey(key);
        }

        // Try sign transaction
        UniValue signResult = SignTransaction(mTx, NullUniValue, &keystore, true, NullUniValue);
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
        ptx->SetString1(address);

        // Insert into mempool
        return _accept_transaction(tx, ptx);
        //const CTransaction& ctx = *tx;
        //return ctx.ToString();
    }

    UniValue GenerateAddress(const JSONRPCRequest& request)
    {
        if (request.fHelp)
            throw runtime_error(
                "generateaddress\n"
                "\nCreate new pocketnet address.\n"
            );

        if (Params().NetworkIDString() != CBaseChainParams::TESTNET)
            throw runtime_error("Only for testnet\n");

        std::shared_ptr<CWallet> const wallet = GetWalletForJSONRPCRequest(request);
        CWallet* const pwallet = wallet.get();

        if (!EnsureWalletIsAvailable(pwallet, request.fHelp)) {
            return NullUniValue;
        }

        // // Amount for full address
        // CAmount nAmount = 2000;

        UniValue result(UniValue::VOBJ);

        LOCK2(cs_main, pwallet->cs_wallet);

        // Create address
        bool fCompressed = pwallet->CanSupportFeature(FEATURE_COMPRPUBKEY); // default to compressed public keys if we want 0.6.0 wallets
        CKey secretKey;
        secretKey.MakeNewKey(fCompressed);
        CPubKey pubkey = secretKey.GetPubKey();

        OutputType output_type = pwallet->m_default_address_type;
        CTxDestination dest = GetDestinationForKey(pubkey, output_type);
        auto addressDest = EncodeDestination(dest);

        // // Send money
        // CCoinControl coin_control;
        // mapValue_t mapValue;
        // auto tx = SendMoney(pwallet, dest, nAmount, false, coin_control, std::move(mapValue));


        result.pushKV("address", addressDest);
        result.pushKV("privkey", EncodeSecret(secretKey));
        //result.pushKV("refill", tx->GetHash().GetHex());
        return result;
    }

    UniValue _accept_transaction(const CTransactionRef& tx, const PTransactionRef& ptx)
    {
        const uint256& txid = tx->GetHash();

        promise<void> promise;
        CAmount nMaxRawTxFee = maxTxFee;
        if (ptx && *ptx->GetType() == PocketTx::BOOST_CONTENT)
            nMaxRawTxFee = 0;

        { // cs_main scope
            LOCK(cs_main);

            CCoinsViewCache& view = *pcoinsTip;
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
                CValidationState state;
                bool fMissingInputs;
                if (!AcceptToMemoryPool(mempool, state, tx, ptx, &fMissingInputs,
                    nullptr /* plTxnReplaced */, false /* bypass_limits */, nMaxRawTxFee))
                {
                    if (state.IsConsensusFailed())
                    {
                        throw JSONRPCError(state.GetRejectCode(), FormatStateMessage(state));
                    }
                    else if (state.IsInvalid())
                    {
                        throw JSONRPCError(RPC_TRANSACTION_REJECTED, FormatStateMessage(state));
                    }
                    else
                    {
                        if (state.GetRejectCode() == RPC_POCKETTX_MATURITY)
                        {
                            throw JSONRPCError(RPC_POCKETTX_MATURITY, FormatStateMessage(state));
                        }
                        else
                        {
                            if (fMissingInputs)
                            {
                                throw JSONRPCError(RPC_TRANSACTION_ERROR, "Missing inputs");
                            }

                            throw JSONRPCError(RPC_TRANSACTION_ERROR, FormatStateMessage(state));
                        }
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

        if (g_connman)
        {
            CInv inv(MSG_TX, txid);
            g_connman->ForEachNode([&inv](CNode* pnode) {
                pnode->PushInventory(inv);
            });
        }

        return txid.GetHex();
    }
}