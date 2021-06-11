// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 Bitcoin developers
// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include "PocketRpc.h"

#include <pos.h>
#include <validation.h>
#include <logging.h>

UniValue PocketWeb::PocketRpc::GetLastComments(const JSONRPCRequest& request)
{
    if (request.fHelp)
        throw std::runtime_error(
            "getlastcomments (count)\n"
            "\nGet Pocketnet last comments.\n");

    int resultCount = 10;
    if (request.params.size() > 0) {
        if (request.params[0].isNum()) {
            resultCount = request.params[0].get_int();
        }
    }

    std::string address = "";
    if (request.params.size() > 1) {
        address = request.params[1].get_str();
    }

    std::string lang = "";
    if (request.params.size() > 2) {
        lang = request.params[2].get_str();
    }

    int nHeight = chainActive.Height();

    auto aResult = PocketDb::WebRepoInst.GetLastComments(resultCount, nHeight, lang);

    return aResult;
}
