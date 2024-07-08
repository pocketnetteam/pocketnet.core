// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include "pocketdb/web/AppRpc.h"

namespace PocketWeb::PocketWebRpc
{

    RPCHelpMan GetApps()
    {
        return RPCHelpMan{"getapps",
            "\nGet applications list.\n",
            {
                { "request", RPCArg::Type::STR, RPCArg::Optional::NO, "JSON object for filter details" },
            },
            RPCResult{ RPCResult::Type::ARR, "", "", {
                { RPCResult::Type::STR_HEX, "hash", "Tx hash" },
            }},
            RPCExamples{
                HelpExampleCli("getapps", "request") +
                HelpExampleRpc("getapps", "request")
            },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
        {
            RPCTypeCheck(request.params, { UniValue::VOBJ });

            // Parse request params
            auto _args = request.params[0].get_obj();

            AppListDto appListDto;
            appListDto.Page = ParsePaginationArgs(_args);

            if (auto arg = _args.At("tags", true); arg.isArray())
                for (size_t i = 0; i < arg.size(); i++)
                    appListDto.Tags.push_back(arg[i].get_str());

            if (auto arg = _args.At("search", true); arg.isStr())
                appListDto.Search = arg.get_str();

            UniValue result(UniValue::VARR);

            // Fetch txs
            auto _txs = request.DbConnection()->AppRepoInst->List(appListDto);

            // Fetch additional data
            auto _additionalData = request.DbConnection()->AppRepoInst->AdditionalInfo(_txs);

            // Construct result from txs
            auto txs = request.DbConnection()->TransactionRepoInst->List(_txs, true);

            // Build result array with original sorting
            for (const auto& _tx : _txs)
            {
                for (const auto& tx : *txs)
                {
                    if (*tx->GetHash() == _tx)
                    {
                        auto txData = ConstructTransaction(tx);

                        // Add additional data if exists
                        if (auto itr = _additionalData.find(*tx->GetHash()); itr != _additionalData.end()) {
                            UniValue addData(UniValue::VOBJ);
                            txData.pushKV("ad", itr->second);
                        }

                        result.push_back(txData);
                    }
                }
            }

            return result;
        }};
    }

    RPCHelpMan GetAppScores()
    {
        return RPCHelpMan{"getappscores",
            "\nGet application scores list.\n",
            {
                { "request", RPCArg::Type::STR, RPCArg::Optional::NO, "JSON object for filter details" },
            },
            RPCResult{ RPCResult::Type::ARR, "", "", {
                { RPCResult::Type::STR_HEX, "hash", "Tx hash" },
            }},
            RPCExamples{
                HelpExampleCli("getappscores", "request") +
                HelpExampleRpc("getappscores", "request")
            },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
        {
            RPCTypeCheck(request.params, { UniValue::VOBJ });

            // Parse request params
            auto _args = request.params[0].get_obj();

            string txHash;
            if (auto arg = _args.At("app", true); arg.isStr())
                txHash = arg.get_str();
            else
                throw JSONRPCError(RPC_INVALID_PARAMS, "Missing 'app' parameter");

            Pagination pg = ParsePaginationArgs(_args);

            UniValue result(UniValue::VARR);

            // Fetch txs
            auto _txs = request.DbConnection()->AppRepoInst->Scores(txHash, pg);

            // Construct result from txs
            auto txs = request.DbConnection()->TransactionRepoInst->List(_txs, true);

            // Build result array with original sorting
            for (const auto& _tx : _txs)
                for (const auto& tx : *txs)
                    if (*tx->GetHash() == _tx)
                        result.push_back(ConstructTransaction(tx));
                        
            return result;
        }};
    }

    RPCHelpMan GetAppComments()
    {
        return RPCHelpMan{"getappcomments",
            "\nGet application comments list.\n",
            {
                { "request", RPCArg::Type::STR, RPCArg::Optional::NO, "JSON object for filter details" },
            },
            RPCResult{ RPCResult::Type::ARR, "", "", {
                { RPCResult::Type::STR_HEX, "hash", "Tx hash" },
            }},
            RPCExamples{
                HelpExampleCli("getappcomments", "request") +
                HelpExampleRpc("getappcomments", "request")
            },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
        {
            RPCTypeCheck(request.params, { UniValue::VOBJ });

            // Parse request params
            auto _args = request.params[0].get_obj();

            string txHash;
            if (auto arg = _args.At("app", true); arg.isStr())
                txHash = arg.get_str();
            else
                throw JSONRPCError(RPC_INVALID_PARAMS, "Missing 'app' parameter");

            Pagination pg = ParsePaginationArgs(_args);

            UniValue result(UniValue::VARR);

            // Fetch txs
            auto _txs = request.DbConnection()->AppRepoInst->Comments(txHash, pg);

            // Construct result from txs
            auto txs = request.DbConnection()->TransactionRepoInst->List(_txs, true);

            // Build result array with original sorting
            for (const auto& _tx : _txs)
                for (const auto& tx : *txs)
                    if (*tx->GetHash() == _tx)
                        result.push_back(ConstructTransaction(tx));
                        
            return result;
        }};
    }

}