// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include "pocketdb/web/BarteronRpc.h"

namespace PocketWeb::PocketWebRpc
{
    RPCHelpMan GetAccount()
    {
        return RPCHelpMan{"getbarteronaccount",
            "\nGet barteron account information.\n",
            {
                { "address", RPCArg::Type::STR, RPCArg::Optional::NO, "Address hash" },
            },
            RPCResult{RPCResult::Type::NONE, "", ""},
            RPCExamples{
                HelpExampleCli("getbarteronaccount", "address") +
                HelpExampleRpc("getbarteronaccount", "address")
            },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
        {
            RPCTypeCheck(request.params, { UniValue::VSTR });
            auto address = request.params[0].get_str();

            return request.DbConnection()->BarteronRepoInst->GetAccount(address);
        }};
    }
}