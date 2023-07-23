// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include "pocketdb/web/BarteronRpc.h"

namespace PocketWeb::PocketWebRpc
{
    RPCHelpMan GetBarteronAccounts()
    {
        return RPCHelpMan{"getbarteronaccounts",
            "\nGet barteron accounts information.\n",
            {
                { "addresses", RPCArg::Type::STR, RPCArg::Optional::OMITTED_NAMED_ARG, "Array of address hashes" },
            },
            RPCResult{ RPCResult::Type::ARR, "", "", {
                { RPCResult::Type::STR_HEX, "hash", "Tx hash" },
            }},
            RPCExamples{
                HelpExampleCli("getbarteronaccounts", "addresses") +
                HelpExampleRpc("getbarteronaccounts", "addresses")
            },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
        {
            RPCTypeCheck(request.params, { UniValue::VARR });
            auto addresses = ParseArrayAddresses(request.params[0].get_array());

            auto addressTxHashes = request.DbConnection()->BarteronRepoInst->GetAccountIds(addresses);
            auto txs = request.DbConnection()->TransactionRepoInst->List(addressTxHashes, true);

            UniValue result(UniValue::VARR);
            for (const auto& tx : *txs)
                result.push_back(ConstructTransaction(tx));

            return result;
        }};
    }
}