// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 Bitcoin developers
// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include "PocketUserRpc.h"

std::map<std::string, UniValue> PocketWeb::PocketUserRpc::GetUsersProfiles(std::vector<std::string> addresses, bool shortForm, int option)
{
    auto result = PocketDb::WebRepoInst.GetUserProfile(addresses, shortForm, option);

    if (shortForm)
    {
        return result;
    }

    auto subscribes = PocketDb::WebRepoInst.GetSubscribesAddresses(addresses);
    auto subscribers = PocketDb::WebRepoInst.GetSubscribersAddresses(addresses);
    auto blocking = PocketDb::WebRepoInst.GetBlockingToAddresses(addresses);

    for (auto i = result.begin(); i != result.end(); ++i)
    {
        if (subscribes.find(i->first) != subscribes.end())
        {
            i->second.pushKV("subscribes", subscribes[i->first]);
        }

        if (subscribers.find(i->first) != subscribers.end())
        {
            i->second.pushKV("subscribers", subscribers[i->first]);
        }

        if (blocking.find(i->first) != blocking.end())
        {
            i->second.pushKV("blocking", blocking[i->first]);
        }
    }

    return result;
}

UniValue PocketWeb::PocketUserRpc::GetReputations(const JSONRPCRequest& request)
{
    return UniValue();
}

UniValue PocketWeb::PocketUserRpc::GetUserProfile(const JSONRPCRequest& request)
{
    if (request.fHelp)
        throw std::runtime_error(
            "getuserprofile \"address\" ( shortForm )\n"
            "\nReturn Pocketnet user profile.\n");

    if (request.params.size() < 1)
        throw JSONRPCError(RPC_INVALID_PARAMETER, "There is no address");

    std::vector<string> addresses;
    if (request.params[0].isStr())
        addresses.push_back(request.params[0].get_str());
    else if (request.params[0].isArray()) {
        UniValue addr = request.params[0].get_array();
        for (unsigned int idx = 0; idx < addr.size(); idx++) {
            addresses.push_back(addr[idx].get_str());
        }
    } else
        throw JSONRPCError(RPC_INVALID_PARAMS, "Invalid inputs params");

    // Short profile form is: address, b, i, name
    bool shortForm = false;
    if (request.params.size() >= 2) {
        shortForm = request.params[1].get_str() == "1";
    }

    UniValue aResult(UniValue::VARR);

    std::map<std::string, UniValue> profiles = GetUsersProfiles(addresses, shortForm);
    for (auto& p : profiles) {
        aResult.push_back(p.second);
    }

    return aResult;
}
