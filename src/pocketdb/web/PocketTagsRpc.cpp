// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include "pocketdb/web/PocketTagsRpc.h"

namespace PocketWeb::PocketWebRpc
{
    UniValue GetTags(const JSONRPCRequest& request)
    {
        std::string address = "";
        if (!request.params[0].isNull() && request.params[0].get_str().length() > 0) {
            RPCTypeCheckArgument(request.params[0], UniValue::VSTR);
            CTxDestination dest = DecodeDestination(request.params[0].get_str());

            if (!IsValidDestination(dest)) {
                throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, std::string("Invalid Pocketcoin address: ") + request.params[0].get_str());
            }

            address = request.params[0].get_str();
        }

        int count = 50;
        if (request.params.size() > 1) {
            ParseInt32(request.params[1].get_str(), &count);
        }

        int from = 0;
        if (request.params.size() > 2) {
            ParseInt32(request.params[2].get_str(), &from);
        }

        std::string lang = "";
        if (request.params.size() > 3) {
            lang = request.params[3].get_str();
        }

        std::map<std::string, int> mapTags;

        return UniValue(UniValue::VARR);
    }
}
