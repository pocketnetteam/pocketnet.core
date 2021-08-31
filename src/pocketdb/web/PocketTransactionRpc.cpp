// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include "pocketdb/web/PocketTransactionRpc.h"

namespace PocketWeb::PocketWebRpc
{

    UniValue AddTransaction(const JSONRPCRequest& request)
    {
        if (request.fHelp)
            throw std::runtime_error(
                "addtransaction\n"
                "\nAdd new pocketnet transaction.\n"
            );

        RPCTypeCheck(request.params, {UniValue::VSTR, UniValue::VOBJ});

        CMutableTransaction mTx;
        if (!DecodeHexTx(mTx, request.params[0].get_str()))
            throw JSONRPCError(RPC_DESERIALIZATION_ERROR, "TX decode failed");

        const auto tx = MakeTransactionRef(std::move(mTx));
        
        string address;
        // TODO (brangr): get address from TxOuputs by TxHash and Number
        // if (!GetInputAddress(tx->vin[0].prevout.hash, tx->vin[0].prevout.n, address)) {
        //     throw JSONRPCError(RPC_INVALID_PARAMS, "Invalid address");
        // }

        auto data = request.params[1];
        data.pushKV("txAddress", address);

        auto[ok, ptx] = PocketServices::TransactionSerializer::DeserializeTransactionRpc(tx, data);

        // TODO (brangr): implement check & validate with consensus
        // TODO (brangr): implement insert into mempool
        
        return *ptx->GetHash();
    }

}