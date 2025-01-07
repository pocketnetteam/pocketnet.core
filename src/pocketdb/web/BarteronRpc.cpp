// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include "pocketdb/web/BarteronRpc.h"

namespace PocketWeb::PocketWebRpc
{
    // accTxIds required just for optimizations. Remove it if necessary and collect ids from txs param
    static UniValue _serialize_account_txs_with_additional_info(const vector<PTransactionRef>& txs, BarteronRepository& bRepo)
    {
        vector<string> txIds;
        for (const auto& tx: txs) {
            txIds.emplace_back(*tx->GetHash());
        }
        auto additionalData = bRepo.GetAccountsAdditionalInfo(txIds);

        UniValue result(UniValue::VARR);
        for (const auto& tx : txs) {
            auto txData = ConstructTransaction(tx);
            if (auto itr = additionalData.find(*tx->GetHash()); itr != additionalData.end()) {
                UniValue addData(UniValue::VOBJ);
                addData.pushKV("regdate", itr->second.RegDate);
                addData.pushKV("rating", itr->second.Rating);
                txData.pushKV("additional", addData);
            }
            result.push_back(txData);
        }

        return result;
    }

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
            return _serialize_account_txs_with_additional_info(*txs, *request.DbConnection()->BarteronRepoInst);
        }};
    }
    
    RPCHelpMan GetBarteronOffersByRootTxHashes()
    {
        return RPCHelpMan{"getbarteronoffersbyroottxhashes",
            "\nGet barteron offers information.\n",
            {
                { "hashes", RPCArg::Type::STR, RPCArg::Optional::NO, "Array of tx hashes" },
            },
            RPCResult{ RPCResult::Type::ARR, "", "", {
                { RPCResult::Type::STR_HEX, "hash", "Tx hash" },
            }},
            RPCExamples{
                HelpExampleCli("getbarteronoffersbyroottxhashes", "hashes") +
                HelpExampleRpc("getbarteronoffersbyroottxhashes", "hashes")
            },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
        {
            RPCTypeCheck(request.params, { UniValue::VARR });
            auto hashes = ParseArrayHashes(request.params[0].get_array());

            auto lastTxHashes = request.DbConnection()->WebRpcRepoInst->GetLastContentHashesByRootTxHashes(hashes);
            auto txs = request.DbConnection()->TransactionRepoInst->List(lastTxHashes, true);

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
                    
                if (auto arg = args.At("location", true); arg.isArray())
                    for (size_t i = 0; i < arg.size(); i++)
                        if (arg[i].isStr())
                            feedArgs.Location.push_back(arg[i].get_str());
                    
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

    RPCHelpMan GetBarteronGroups()
    {
        return RPCHelpMan{"getbarterongroups",
            "\nGet barteron groups feed.\n",
            {
                { "request", RPCArg::Type::STR, RPCArg::Optional::NO, "JSON object for filter groups" },
            },
            RPCResult{ RPCResult::Type::ARR, "", "", {
                { RPCResult::Type::STR_HEX, "hash", "Tx hash" },
            }},
            RPCExamples{
                HelpExampleCli("getbarterongroups", "request") +
                HelpExampleRpc("getbarterongroups", "request")
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
                    
                if (auto arg = args.At("location", true); arg.isArray())
                    for (size_t i = 0; i < arg.size(); i++)
                        if (arg[i].isStr())
                            feedArgs.Location.push_back(arg[i].get_str());
                
                if (auto arg = args.At("locationGroup", true); arg.isNum())
                    feedArgs.LocationGroup = arg.get_int();
                    
                if (auto arg = args.At("priceMax", true); arg.isNum())
                    feedArgs.PriceMax = arg.get_int();
                
                if (auto arg = args.At("priceMin", true); arg.isNum())
                    feedArgs.PriceMin = arg.get_int();
                    
                if (auto arg = args.At("search", true); arg.isStr())
                    feedArgs.Search = arg.get_str();
            }

            return request.DbConnection()->BarteronRepoInst->GetGroups(feedArgs);
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

                if (auto arg = _args.At("location", true); arg.isArray())
                    for (size_t i = 0; i < arg.size(); i++)
                        if (arg[i].isStr())
                            args.Location.push_back(arg[i].get_str());

                if (auto arg = _args.At("excludeaddresses", true); arg.isArray())
                    for (int i = 0; i < arg.size(); i++)
                        args.ExcludeAddresses.emplace_back(arg[i].get_str());

                if (auto arg = _args.At("pricemin", true); arg.isNum())
                    args.PriceMin = arg.get_int();

                if (auto arg = _args.At("pricemax", true); arg.isNum())
                    args.PriceMax = arg.get_int();

                if (auto arg = _args.At("search", true); arg.isStr())
                    args.Search = arg.get_str();
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

    RPCHelpMan GetBarteronOffersDetails()
    {
        return RPCHelpMan{"getbarteronoffersdetails",
            "\nGet barteron offers feed.\n",
            {
                { "request", RPCArg::Type::STR, RPCArg::Optional::NO, "JSON object for filter details" },
            },
            RPCResult{ RPCResult::Type::ARR, "", "", {
                { RPCResult::Type::STR_HEX, "hash", "Tx hash" },
            }},
            RPCExamples{
                HelpExampleCli("getbarteronoffersdetails", "request") +
                HelpExampleRpc("getbarteronoffersdetails", "request")
            },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
        {
            RPCTypeCheck(request.params, { UniValue::VOBJ });

            auto _args = request.params[0].get_obj();

            vector<string> offerIds;
            bool includeScores = false, includeComments = false, includeCommentScores = false, includeAccounts = false;

            if (auto arg = _args.At("offerids", true); arg.isArray())
                for (int i = 0; i < arg.size(); i++)
                    offerIds.emplace_back(arg[i].get_str());

            if (auto arg = _args.At("includescores", true); arg.isBool())
                includeScores = arg.get_bool();

            if (auto arg = _args.At("includecomments", true); arg.isBool())
                includeComments = arg.get_bool();

            if (auto arg = _args.At("includecommentscores", true); arg.isBool())
                includeCommentScores = arg.get_bool();

            if (auto arg = _args.At("includeaccounts", true); arg.isBool())
                includeAccounts = arg.get_bool();

            vector<string> allTxs = offerIds;
            UniValue result(UniValue::VOBJ);

            if (includeScores) {
                auto offerScoreIds = request.DbConnection()->WebRpcRepoInst->GetContentScores(offerIds);
                result.pushKV("offerScores", _list_tx_to_uni(*request.DbConnection()->TransactionRepoInst, offerScoreIds));
                if (includeAccounts) copy(offerScoreIds.begin(), offerScoreIds.end(), back_inserter(allTxs));
            }
            if (includeComments) {
                auto commentIds = request.DbConnection()->WebRpcRepoInst->GetContentComments(offerIds);
                result.pushKV("comments", _list_tx_to_uni(*request.DbConnection()->TransactionRepoInst, commentIds));
                if (includeAccounts) copy(commentIds.begin(), commentIds.end(), back_inserter(allTxs));

                if (includeCommentScores) {
                    auto commentScoreIds = request.DbConnection()->WebRpcRepoInst->GetCommentScores(commentIds);
                    result.pushKV("commentScores", _list_tx_to_uni(*request.DbConnection()->TransactionRepoInst, commentScoreIds));
                    if (includeAccounts) copy(commentScoreIds.begin(), commentScoreIds.end(), back_inserter(allTxs));
                }
            }

            if (includeAccounts) {
                auto addresses = request.DbConnection()->WebRpcRepoInst->GetAddresses(allTxs);

                auto barteronAccIds = request.DbConnection()->BarteronRepoInst->GetAccountIds(addresses);
                auto barteronAccTxs = request.DbConnection()->TransactionRepoInst->List(barteronAccIds, true);
                vector<string> bastyonOnlyAddresses;
                copy_if(
                    addresses.begin(),
                    addresses.end(),
                    back_inserter(bastyonOnlyAddresses),
                    [&](const auto& address) {
                        return std::find_if(barteronAccTxs->begin(), barteronAccTxs->end(), [&](const auto& tx) {
                                    return *tx->GetString1() == address;
                                }) == barteronAccTxs->end();
                    }
                );
                // TODO (losty): these are addresses not txs
                auto bastyonAccIds = request.DbConnection()->WebRpcRepoInst->GetAccountsIds(bastyonOnlyAddresses);

                auto txs = request.DbConnection()->TransactionRepoInst->List(bastyonOnlyAddresses, true);
                txs->reserve(txs->size() + barteronAccTxs->size());
                copy(barteronAccTxs->begin(), barteronAccTxs->end(), back_inserter(*txs));

                result.pushKV("accounts", _serialize_account_txs_with_additional_info(*txs, *request.DbConnection()->BarteronRepoInst));
            }

            return result;
        }};
    }

    RPCHelpMan GetBarteronComplexDeals()
    {
        return RPCHelpMan{"getbarteroncomplexdeals",
            "\nGet barteron complex (3-side deals).\n",
            {
                { "request", RPCArg::Type::STR, RPCArg::Optional::NO, "JSON object for filter offers" },
            },
            RPCResult{ RPCResult::Type::ARR, "", "", {
                { RPCResult::Type::STR_HEX, "hash", "Tx hash" },
            }},
            RPCExamples{
                HelpExampleCli("getbarteroncomplexdeals", "request") +
                HelpExampleRpc("getbarteroncomplexdeals", "request")
            },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
        {
            RPCTypeCheck(request.params, { UniValue::VOBJ });

            BarteronOffersComplexDealDto args;
            {
                auto _args = request.params[0].get_obj();
                args.Page = ParsePaginationArgs(_args);

                if (auto arg = _args.At("theirtags", true); arg.isArray())
                    for (int i = 0; i < arg.size(); i++)
                        args.TheirTags.emplace_back(arg[i].get_int());

                if (auto arg = _args.At("mytag", true); arg.isNum())
                        args.MyTag = arg.get_int();

                if (auto arg = _args.At("excludeaddresses", true); arg.isArray())
                    for (int i = 0; i < arg.size(); i++)
                        args.ExcludeAddresses.emplace_back(arg[i].get_str());

                if (auto arg = _args.At("location", true); arg.isArray())
                    for (size_t i = 0; i < arg.size(); i++)
                        if (arg[i].isStr())
                            args.Location.push_back(arg[i].get_str());

                if (args.MyTag == 0 || args.TheirTags.empty()) {
                    // TODO (losty): error
                }
            }

            auto deals = request.DbConnection()->BarteronRepoInst->GetComplexDeal(args);

            vector<string> hashes;
            hashes.reserve(deals.size() * 2); // pseudo optimization
            for (const auto& [k, v]: deals) {
                hashes.reserve(v.size() + 1);
                hashes.emplace_back(k);
                copy(v.begin(), v.end(), back_inserter(hashes));
            }
            auto txs = request.DbConnection()->TransactionRepoInst->List(hashes, true);

            map<string, PTransactionRef> m;
            for (const auto& tx: *txs) {
                m.emplace(*tx->GetHash(), tx);
            }

            // Build result array with original sorting
            UniValue result(UniValue::VARR);
            for (const auto& [target, intermediates]: deals) {
                if (auto tx = m.find(target); tx != m.end()) {
                    UniValue o(UniValue::VOBJ);
                    o.pushKV("target", ConstructTransaction(tx->second));

                    UniValue inters(UniValue::VARR);
                    for (const auto& inter: intermediates) {
                        if (auto tx = m.find(inter); tx != m.end()) {
                            inters.push_back(ConstructTransaction(tx->second));
                        }
                    }
                    o.pushKV("intermediates", inters);

                    result.push_back(o);
                }
            }

            return result;
        }};
    }

}