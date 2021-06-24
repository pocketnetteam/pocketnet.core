// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 Bitcoin developers
// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include "PocketSystemRpc.h"

UniValue PocketWeb::PocketSystemRpc::GetTime(const JSONRPCRequest& request)
{
    if (request.fHelp)
        throw std::runtime_error(
            "gettime\n"
            "\nReturn node time.\n");

    UniValue entry(UniValue::VOBJ);
    entry.pushKV("time", GetAdjustedTime());

    return entry;
}
