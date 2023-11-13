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
            auto additionalData = request.DbConnection()->BarteronRepoInst->GetAccountsAdditionalInfo(addressTxHashes);

            UniValue result(UniValue::VARR);
            for (const auto& tx : *txs) {
                auto txData = ConstructTransaction(tx);
                if (auto itr = additionalData.find(*tx->GetHash()); itr != additionalData.end()) {
                    UniValue addData(UniValue::VOBJ);
                    addData.pushKV("regdate", itr->second.RegDate);
                    txData.pushKV("additional", addData);
                }
                result.push_back(txData);
            }

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

    UniValue _list_tx_to_uni(TransactionRepository& repo, const vector<string> ids)
    {
        auto txs = repo.List(ids, true);
        UniValue arr(UniValue::VARR);

        for (const auto& tx: *txs) {
            arr.push_back(ConstructTransaction(tx));
        }

        return arr;
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
                feedArgs.Page = ParsePaginationArgs(args);

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

            auto offerIds = request.DbConnection()->BarteronRepoInst->GetFeed(feedArgs);
            auto offerScoreIds = request.DbConnection()->WebRpcRepoInst->GetContentScores(offerIds);
            auto commentIds = request.DbConnection()->WebRpcRepoInst->GetContentComments(offerIds);
            auto commentScoreIds = request.DbConnection()->WebRpcRepoInst->GetCommentScores(commentIds);

            vector<string> allTxs;
            copy(offerIds.begin(), offerIds.end(), back_inserter(allTxs));
            copy(offerScoreIds.begin(), offerScoreIds.end(), back_inserter(allTxs));
            copy(commentIds.begin(), commentIds.end(), back_inserter(allTxs));
            copy(commentScoreIds.begin(), commentScoreIds.end(), back_inserter(allTxs));
            auto addressesIds = request.DbConnection()->WebRpcRepoInst->GetAddresses(allTxs);

            // Build result array with original sorting
            UniValue result(UniValue::VOBJ);

            UniValue offersUni(UniValue::VARR);
            auto offers = request.DbConnection()->TransactionRepoInst->List(offerIds, true);
            for (const auto& hash : offerIds)
                for (const auto& offer : *offers)
                    if (*offer->GetHash() == hash)
                        offersUni.push_back(ConstructTransaction(offer));

            result.pushKV("offers", offersUni);

            result.pushKV("offerScores", _list_tx_to_uni(*request.DbConnection()->TransactionRepoInst, offerScoreIds));
            result.pushKV("comments", _list_tx_to_uni(*request.DbConnection()->TransactionRepoInst, commentIds));
            result.pushKV("commentScoreIds", _list_tx_to_uni(*request.DbConnection()->TransactionRepoInst, commentScoreIds));

            return result;
        }};
    }

    RPCHelpMan GetBarteronDeals()
    {
        return RPCHelpMan{"getbarterondeals",
            "\nGet barteron offers feed.\n",
            {
                { "request", RPCArg::Type::STR, RPCArg::Optional::NO, "JSON object for filter offers" },
            },
            RPCResult{ RPCResult::Type::ARR, "", "", {
                { RPCResult::Type::STR_HEX, "hash", "Tx hash" },
            }},
            RPCExamples{
                HelpExampleCli("getbarterondeals", "request") +
                HelpExampleRpc("getbarterondeals", "request")
            },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
        {
            RPCTypeCheck(request.params, { UniValue::VOBJ });

            BarteronOffersDealDto args;
            {
                auto _args = request.params[0].get_obj();
                args.Page = ParsePaginationArgs(_args);

                if (auto arg = _args.At("theirtags", true); arg.isArray())
                    for (int i = 0; i < arg.size(); i++)
                        args.TheirTags.emplace_back(arg[i].get_int());

                if (auto arg = _args.At("mytags", true); arg.isArray())
                    for (int i = 0; i < arg.size(); i++)
                        args.MyTags.emplace_back(arg[i].get_int());

                if (auto arg = _args.At("addresses", true); arg.isArray())
                    for (int i = 0; i < arg.size(); i++)
                        args.Addresses.emplace_back(arg[i].get_str());

                if (auto arg = _args.At("location", true); arg.isStr())
                    args.Location = arg.get_str();

                if (auto arg = _args.At("excludeaddresses", true); arg.isArray())
                    for (int i = 0; i < arg.size(); i++)
                        args.ExcludeAddresses.emplace_back(arg[i].get_str());

                if (auto arg = _args.At("price", true); arg.isNum())
                    args.Price = arg.get_int();
            }

            auto hashes = request.DbConnection()->BarteronRepoInst->GetDeals(args);
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