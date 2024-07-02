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

            UniValue result(UniValue::VOBJ);

            // Fetch txs
            auto _txs = request.DbConnection()->AppRepoInst->List(appListDto);

            // Fetch additional data
            auto _additionalData = request.DbConnection()->AppRepoInst->AdditionalInfo(_txs);

            // Construct result from txs
            auto txs = request.DbConnection()->TransactionRepoInst->List(_txs, true);

            // Build result array with original sorting
            UniValue result(UniValue::VARR);
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
                            // TODO app : add additional data fields
                            // addData.pushKV("regdate", itr->second.RegDate);
                            // addData.pushKV("rating", itr->second.Rating);
                            txData.pushKV("additional", addData);
                        }

                        result.push_back(txData);
                    }
                }
            }

            return result;
        }};
    }

}