
// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include "pocketdb/web/PocketScoresRpc.h"
#include "rpc/util.h"

namespace PocketWeb::PocketWebRpc
{
    RPCHelpMan GetAddressScores()
    {
        return RPCHelpMan{"getaddressscores",
                "\nGet scores from address.\n",
                {
                    // TODO (losty-fur): provide arguments description
                },
                RPCResult{
                    RPCResult::Type::ARR, "", "", 
                    {
                        {
                            RPCResult::Type::OBJ, "", "",
                            {
                                {RPCResult::Type::STR, "posttxid", ""},
                                {RPCResult::Type::STR, "address", ""},
                                {RPCResult::Type::STR, "name", ""},
                                {RPCResult::Type::STR, "avatar", ""},
                                {RPCResult::Type::NUM, "reputation", ""},
                                {RPCResult::Type::NUM, "value", ""},
                            }
                        }
                    },
                },
                RPCExamples{
                    // TODO (losty-fur): provide correct examples
                    HelpExampleCli("getaddressscores", "") +
                    HelpExampleRpc("getaddressscores", "")
                },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
    {
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
    },
        };
    }

    RPCHelpMan GetPostScores()
    {
        return RPCHelpMan{"getpostscores",
                "\nGet scores from address.\n",
                {
                    // TODO (losty-fur): provide arguments description
                },
                RPCResult{
                    RPCResult::Type::ARR, "", "", 
                    {
                        {
                            RPCResult::Type::OBJ, "", /* optional (means array may be empty) */ true, "",
                            {
                                {RPCResult::Type::STR, "posttxid", ""},
                                {RPCResult::Type::STR, "value", ""}
                            }
                        }
                    },
                },
                RPCExamples{
                    // TODO (losty-fur): provide correct examples
                    HelpExampleCli("getpostscores", "") +
                    HelpExampleRpc("getpostscores", "")
                },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
    {
        if (request.fHelp)
            throw std::runtime_error(
                "getpostscores\n"
                "\nGet scores from address.\n");

        RPCTypeCheck(request.params, {UniValue::VSTR, UniValue::VSTR, UniValue::VARR});

        auto postTxHash = request.params[0].get_str();
        return request.DbConnection()->WebRpcRepoInst->GetPostScores(postTxHash);
    },
        };
    }

    RPCHelpMan GetPagesScores()
    {
        return RPCHelpMan{"getpagescores",
                "\nGet scores for posts and comments from address.\n",
                {
                    {"postHashes", RPCArg::Type::ARR, RPCArg::Optional::NO, "",
                        {
                            {"postHash", RPCArg::Type::STR, RPCArg::Optional::NO, ""}   
                        }
                    },
                    {"address", RPCArg::Type::STR, RPCArg::Optional::NO, ""},
                    {"commentHashes", RPCArg::Type::ARR, RPCArg::Optional::NO, "",
                        {
                            {"commentHash", RPCArg::Type::STR, RPCArg::Optional::NO, ""}   
                        }
                    }
                },
                RPCResult{
                    RPCResult::Type::ARR, "", "", 
                    {
                        {
                            RPCResult::Type::OBJ, "", "",
                            {
                                // TODO (losty-fur): may be change above. The reason everything is optional is
                                // that there are two types of objects in array (post or comment)
                            // Comment scores
                                {RPCResult::Type::STR, "cmntid", /* optional */ true, ""},
                                {RPCResult::Type::STR, "scoreUp", /* optional */ true, ""},
                                {RPCResult::Type::STR, "scoreDown", /* optional */ true, ""},
                                {RPCResult::Type::STR, "reputation", /* optional */ true, ""},
                                {RPCResult::Type::STR, "myScore", /* optional */ true, ""},
                            // Post scores
                                {RPCResult::Type::STR, "posttxid", /* optional */ true, ""},
                                {RPCResult::Type::STR, "value", /* optional */ true, ""},
                            }
                        }
                    },
                },
                RPCExamples{
                    // TODO (losty-fur): provide correct examples
                    // HelpExampleCli("getpagescores", "1231") +
                    // HelpExampleRpc("getpagescores", "1231")
                    ""
                },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
    {
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
    },
        };
    }
}