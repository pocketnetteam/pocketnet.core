// Copyright (c) 2018-2022 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include "pocketdb/web/PocketCommentsRpc.h"

namespace PocketWeb::PocketWebRpc
{
    UniValue GetCommentsByPost(const JSONRPCRequest& request)
    {
        if (request.fHelp)
            throw runtime_error(
                "getcomments (\"postid\", \"parentid\", \"address\", [\"commend_id\",\"commend_id\",...])\n"
                "\nGet Pocketnet comments.\n");

        string postHash;
        if (!request.params.empty()) {
            postHash = request.params[0].get_str();
            if (postHash.length() == 0 && request.params.size() < 3)
                throw JSONRPCError(RPC_INVALID_PARAMS, "Invalid postid");
        }

        string parentHash;
        if (request.params.size() > 1)
            parentHash = request.params[1].get_str();

        string addressHash;
        if (request.params.size() > 2)
            addressHash = request.params[2].get_str();

        vector<string> cmntHashes;
        if (request.params.size() > 3)
        {
            if (request.params[3].isArray())
            {
                UniValue hashes = request.params[3].get_array();
                for (unsigned int id = 0; id < hashes.size(); id++)
                    cmntHashes.push_back(hashes[id].get_str());
            }
            else
            {
                throw JSONRPCError(RPC_INVALID_PARAMS, "Invalid inputs params");
            }
        }

        if (!cmntHashes.empty())
            return request.DbConnection()->WebRpcRepoInst->GetCommentsByHashes(cmntHashes, addressHash);
        else
            return request.DbConnection()->WebRpcRepoInst->GetCommentsByPost(postHash, parentHash, addressHash);
    }

    UniValue GetLastComments(const JSONRPCRequest& request)
    {
        if (request.fHelp)
            throw runtime_error(
                "getlastcomments (count)\n"
                "\nGet Pocketnet last comments.\n");

        int resultCount = 10;
        if (request.params[0].isNum())
            resultCount = request.params[0].get_int();

        string address;
        if (request.params[1].isStr())
            address = request.params[1].get_str();

        string lang = "en";
        if (request.params[2].isStr())
            lang = request.params[2].get_str();

        int nHeight = chainActive.Height();

        return request.DbConnection()->WebRpcRepoInst->GetLastComments(resultCount, nHeight, lang);
    }

}