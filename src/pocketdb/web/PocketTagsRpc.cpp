// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include "pocketdb/web/PocketTagsRpc.h"

namespace PocketWeb::PocketWebRpc
{
    RPCHelpMan GetTags()
    {
        return RPCHelpMan{
                "gettags",
                "\nReturn N top used tags for language\n",
                // TODO (team): provide description for args, returns and examples
                {
                    
                },
                {

                },
                RPCExamples{
                    ""
                },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
    {
        if (request.fHelp)
            throw std::runtime_error(
                "gettags\n"
                "\nReturn N top used tags for language\n");

        int pageStart = 0;
        if (request.params.size() > 0)
        {
            if (request.params[0].isNum())
                pageStart = request.params[0].get_int();
            else if (request.params[0].isStr() && !request.params[0].empty())
                if (!ParseInt32(request.params[0].get_str(), &pageStart))
                    throw JSONRPCError(RPC_INVALID_PARAMETER, "Failed to parse int from string");
        }

        int pageSize = 50;
        if (request.params.size() > 1)
        {
            if (request.params[1].isNum())
                pageSize = request.params[1].get_int();
            else if (request.params[1].isStr() && !request.params[1].empty())
                if (!ParseInt32(request.params[1].get_str(), &pageSize))
                    throw JSONRPCError(RPC_INVALID_PARAMETER, "Failed to parse int from string");
        }

        string lang = "en";
        if (request.params.size() > 3) {
            lang = request.params[3].get_str();
        }

        return request.DbConnection()->WebRpcRepoInst->GetTags(lang, pageSize, pageStart);
    },
        };
    }
}
