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
