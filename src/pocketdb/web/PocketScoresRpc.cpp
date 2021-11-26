
// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include "pocketdb/web/PocketScoresRpc.h"

namespace PocketWeb::PocketWebRpc
{
    UniValue GetAddressScores(const JSONRPCRequest& request)
    {
        if (request.fHelp)
            throw std::runtime_error(
                "getaddressscores\n"
                "\nGet scores from address.\n");

        std::string address;
        if (!request.params.empty() && request.params[0].isStr())
        {
            CTxDestination dest = DecodeDestination(request.params[0].get_str());

            if (!IsValidDestination(dest))
                throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, std::string("Invalid address: ") + request.params[0].get_str());

            address = request.params[0].get_str();
        }
        else
        {
            throw JSONRPCError(RPC_INVALID_PARAMS, "There is no address.");
        }

        vector<string> postHashes;
        if (request.params.size() > 1)
        {
            if (request.params[1].isArray())
            {
                UniValue txid = request.params[1].get_array();
                for (unsigned int idx = 0; idx < txid.size(); idx++)
                {
                    postHashes.push_back(txid[idx].get_str());
                }
            }
            else if (request.params[1].isStr())
            {
                postHashes.push_back(request.params[1].get_str());
            }
            else
            {
                throw JSONRPCError(RPC_INVALID_PARAMS, "Invalid txs format");
            }
        }

        return request.DbConnection()->WebRpcRepoInst->GetAddressScores(postHashes, address);
    }

    UniValue GetPostScores(const JSONRPCRequest& request)
    {
        if (request.fHelp)
            throw std::runtime_error(
                "getpostscores\n"
                "\nGet scores from address.\n");

        RPCTypeCheck(request.params, {UniValue::VSTR, UniValue::VSTR, UniValue::VARR});

        auto postTxHash = request.params[0].get_str();
        return request.DbConnection()->WebRpcRepoInst->GetPostScores(postTxHash);
    }

    UniValue GetPagesScores(const JSONRPCRequest& request)
    {
        if (request.fHelp)
            throw std::runtime_error(
                "getpagescores postHashes[], \"address\", commentHashes[]\n"
                "\nGet scores for posts and comments from address.\n");

        RPCTypeCheck(request.params, {UniValue::VARR, UniValue::VSTR, UniValue::VARR});

        vector<string> postIds;
        UniValue postTxIds = request.params[0].get_array();
        for (unsigned int idx = 0; idx < postTxIds.size(); idx++)
            postIds.push_back(postTxIds[idx].get_str());
            
        CTxDestination dest = DecodeDestination(request.params[1].get_str());
        if (!IsValidDestination(dest))
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, std::string("Invalid address: ") + request.params[1].get_str());
        string address = request.params[1].get_str();

        vector<string> commentIds;
        UniValue commentTxIds = request.params[2].get_array();
        for (unsigned int idx = 0; idx < commentTxIds.size(); idx++)
            commentIds.push_back(commentTxIds[idx].get_str());

        return request.DbConnection()->WebRpcRepoInst->GetPagesScores(postIds, commentIds, address);
    }
}