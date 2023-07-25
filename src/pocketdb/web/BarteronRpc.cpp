// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include "pocketdb/web/BarteronRpc.h"

namespace PocketWeb::PocketWebRpc
{
    RPCHelpMan GetBarteronAccounts()
    {
        return RPCHelpMan{"getbarteronaccounts",
            "\nGet barteron accounts information.\n",
            {
                { "addresses", RPCArg::Type::STR, RPCArg::Optional::NO, "Array of address hashes" },
            },
            RPCResult{ RPCResult::Type::ARR, "", "", {
                { RPCResult::Type::STR_HEX, "hash", "Tx hash" },
            }},
            RPCExamples{
                HelpExampleCli("getbarteronaccounts", "addresses") +
                HelpExampleRpc("getbarteronaccounts", "addresses")
            },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
        {
            RPCTypeCheck(request.params, { UniValue::VARR });
            auto addresses = ParseArrayAddresses(request.params[0].get_array());

            auto addressTxHashes = request.DbConnection()->BarteronRepoInst->GetAccountIds(addresses);
            auto txs = request.DbConnection()->TransactionRepoInst->List(addressTxHashes, true);

            UniValue result(UniValue::VARR);
            for (const auto& tx : *txs)
                result.push_back(ConstructTransaction(tx));

            return result;
        }};
    }
    
    RPCHelpMan GetBarteronOffersByHashes()
    {
        return RPCHelpMan{"getbarteronoffersbyhashes",
            "\nGet barteron offers information.\n",
            {
                { "hashes", RPCArg::Type::STR, RPCArg::Optional::NO, "Array of tx hashes" },
            },
            RPCResult{ RPCResult::Type::ARR, "", "", {
                { RPCResult::Type::STR_HEX, "hash", "Tx hash" },
            }},
            RPCExamples{
                HelpExampleCli("getbarteronoffersbyhashes", "hashes") +
                HelpExampleRpc("getbarteronoffersbyhashes", "hashes")
            },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
        {
            RPCTypeCheck(request.params, { UniValue::VARR });
            auto hashes = ParseArrayHashes(request.params[0].get_array());

            auto txs = request.DbConnection()->TransactionRepoInst->List(hashes, true);

            UniValue result(UniValue::VARR);
            for (const auto& tx : *txs)
                result.push_back(ConstructTransaction(tx));

            return result;
        }};
    }
    
    RPCHelpMan GetBarteronOffersByAddress()
    {
        return RPCHelpMan{"getbarteronoffersbyaddress",
            "\nGet barteron offers information by address.\n",
            {
                { "address", RPCArg::Type::STR, RPCArg::Optional::NO, "Address" },
            },
            RPCResult{ RPCResult::Type::ARR, "", "", {
                { RPCResult::Type::STR_HEX, "hash", "Tx hash" },
            }},
            RPCExamples{
                HelpExampleCli("getbarteronoffersbyaddress", "hashes") +
                HelpExampleRpc("getbarteronoffersbyaddress", "hashes")
            },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
        {
            RPCTypeCheck(request.params, { UniValue::VSTR });
            auto address = request.params[0].get_str();

            auto hashes = request.DbConnection()->BarteronRepoInst->GetAccountOffersIds(address);
            auto txs = request.DbConnection()->TransactionRepoInst->List(hashes, true);

            UniValue result(UniValue::VARR);
            for (const auto& tx : *txs)
                result.push_back(ConstructTransaction(tx));

            return result;
        }};
    }

    RPCHelpMan GetBarteronFeed()
    {
        return RPCHelpMan{"getbarteronfeed",
            "\nGet barteron offers feed.\n",
            {
                { "request", RPCArg::Type::STR, RPCArg::Optional::NO, "JSON object for filter offers" },
            },
            RPCResult{ RPCResult::Type::ARR, "", "", {
                { RPCResult::Type::STR_HEX, "hash", "Tx hash" },
            }},
            RPCExamples{
                HelpExampleCli("getbarteronfeed", "request") +
                HelpExampleRpc("getbarteronfeed", "request")
            },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
        {
            RPCTypeCheck(request.params, { UniValue::VOBJ });

            BarteronOffersFeedDto feedArgs;
            {
                auto args = request.params[0].get_obj();
                feedArgs.Pagination = ParsePaginationArgs(args);

                if (auto arg = args.At("lang", true); arg.isStr())
                    feedArgs.Language = arg.get_str();
                    
                if (auto arg = args.At("tags", true); arg.isArray())
                    for (size_t i = 0; i < arg.size(); i++)
                        if (arg[i].isNum())
                            feedArgs.Tags.push_back(arg[i].get_int());
                    
                if (auto arg = args.At("location", true); arg.isStr())
                    feedArgs.Location = arg.get_str();
                    
                if (auto arg = args.At("priceMax", true); arg.isNum())
                    feedArgs.PriceMax = arg.get_int();
                
                if (auto arg = args.At("priceMin", true); arg.isNum())
                    feedArgs.PriceMin = arg.get_int();
                    
                if (auto arg = args.At("search", true); arg.isStr())
                    feedArgs.Search = arg.get_str();
            }

            auto hashes = request.DbConnection()->BarteronRepoInst->GetFeed(feedArgs);
            auto txs = request.DbConnection()->TransactionRepoInst->List(hashes, true);

            // Build result array with original sorting
            UniValue result(UniValue::VARR);
            for (const auto& hash : hashes)
                for (const auto& tx : *txs)
                    if (*tx->GetHash() == hash)
                        result.push_back(ConstructTransaction(tx));

            return result;
        }};
    }


}