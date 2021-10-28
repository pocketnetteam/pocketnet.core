// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include "pocketdb/web/PocketTransactionRpc.h"

namespace PocketWeb::PocketWebRpc
{
    UniValue AddTransaction(const JSONRPCRequest& request)
    {
        if (request.fHelp)
            throw runtime_error(
                "addtransaction\n"
                "\nAdd new pocketnet transaction.\n"
            );

        if (Params().NetworkID() != NetworkTest)
            throw JSONRPCError(RPC_DESERIALIZATION_ERROR, "Only for TEST network");

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

        // Antibot checked transaction with pocketnet consensus rules
        if (auto[ok, result] = PocketConsensus::SocialConsensusHelper::Check(tx, ptx); !ok)
            throw JSONRPCError(result, PocketConsensus::SocialConsensusResultString(result));

        // And validating
        if (auto[ok, result] = PocketConsensus::SocialConsensusHelper::Validate(ptx, chainActive.Height() + 1); !ok)
            throw JSONRPCError(result, PocketConsensus::SocialConsensusResultString(result));

        // Insert into mempool
        return _accept_transaction(tx, ptx);
    }
    
    UniValue GetTransaction(const JSONRPCRequest& request)
    {
        if (request.fHelp)
            throw runtime_error(
                "getrawtransaction\n"
                "\nGet transaction data.\n"
            );

        RPCTypeCheck(request.params, {UniValue::VSTR});
        string txid = request.params[0].get_str();

        return request.DbConnection()->ExplorerRepoInst->GetTransactions({ txid }, 1, 2);
    }

    UniValue _accept_transaction(const CTransactionRef& tx, const PTransactionRef& ptx)
    {
        promise<void> promise;
        CAmount nMaxRawTxFee = maxTxFee;
        const uint256& txid = tx->GetHash();

        { // cs_main scope
            LOCK(cs_main);

            CCoinsViewCache& view = *pcoinsTip;
            bool fHaveChain = false;
            for (size_t o = 0; !fHaveChain && o < tx->vout.size(); o++) {
                const Coin& existingCoin = view.AccessCoin(COutPoint(txid, o));
                fHaveChain = !existingCoin.IsSpent();
            }
            
            bool fHaveMempool = mempool.exists(txid);

            if (!fHaveMempool && !fHaveChain)
            {
                // push to local node and sync with wallets
                CValidationState state;
                bool fMissingInputs;
                if (!AcceptToMemoryPool(mempool, state, tx, ptx, &fMissingInputs, nullptr /* plTxnReplaced */, false /* bypass_limits */, nMaxRawTxFee)) {
                    if (state.IsInvalid()) {
                        throw JSONRPCError(RPC_TRANSACTION_REJECTED, FormatStateMessage(state));
                    } else {
                        if (state.GetRejectCode() == RPC_POCKETTX_MATURITY) {
                            throw JSONRPCError(RPC_POCKETTX_MATURITY, FormatStateMessage(state));
                        } else {
                            if (fMissingInputs) {
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
            } else if (fHaveChain)
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