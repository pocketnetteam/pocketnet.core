// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include "pocketdb/web/PocketCommentsRpc.h"

namespace PocketWeb::PocketWebRpc
{
    RPCHelpMan GetCommentsByPost()
    {
        return RPCHelpMan{"getcomments",
                "\nGet Pocketnet comments.\n",
                {
                    {"postid", RPCArg::Type::NUM, RPCArg::Optional::NO, "Post to get comments from."},
                    {"parentid", RPCArg::Type::NUM, RPCArg::Optional::OMITTED_NAMED_ARG, ""},
                    {"address", RPCArg::Type::STR, RPCArg::Optional::OMITTED_NAMED_ARG, ""},
                    {"commend_ids", RPCArg::Type::ARR, RPCArg::Optional::OMITTED_NAMED_ARG, ""},
                },
                {
                    // TODO (losty-fur): provide return description
                },
                RPCExamples{
                    HelpExampleCli("getcomments", "\"postid\", \"parentid\", \"address\", [\"commend_id\",\"commend_id\",...]") +
                    HelpExampleRpc("getcomments", "\"postid\", \"parentid\", \"address\", [\"commend_id\",\"commend_id\",...]")
                },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
    {
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

        return request.DbConnection()->WebRpcRepoInst->GetCommentsByPost(postHash, parentHash, addressHash);
    },
        };
    }

    RPCHelpMan GetLastComments()
    {
        return RPCHelpMan{"getlastcomments",
                "\nGet Pocketnet last comments.\n",
                {
                    {"count", RPCArg::Type::NUM, RPCArg::Optional::OMITTED_NAMED_ARG, ""}
                },
                {
                    // TODO (losty-fur): provide return description
                },
                RPCExamples{
                    HelpExampleCli("getlastcomments", "") +
                    HelpExampleRpc("getlastcomments", "")
                },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
    {
        if (request.fHelp)
            throw runtime_error(
                "getlastcomments (count)\n"
                "\nGet Pocketnet last comments.\n");

        int resultCount = 10;
        if (!request.params.empty()) {
            if (request.params[0].isNum()) {
                resultCount = request.params[0].get_int();
            }
        }

        string address;
        if (request.params.size() > 1) {
            address = request.params[1].get_str();
        }

        string lang;
        if (request.params.size() > 2) {
            lang = request.params[2].get_str();
        }

        int nHeight = ::ChainActive().Height();

        return request.DbConnection()->WebRpcRepoInst->GetLastComments(resultCount, nHeight, lang);
    },
        };
    }

}