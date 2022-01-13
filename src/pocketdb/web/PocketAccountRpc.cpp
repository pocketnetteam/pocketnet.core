// Copyright (c) 2018-2022 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include "pocketdb/web/PocketAccountRpc.h"
#include "rpc/blockchain.h"
#include "rpc/util.h"
#include "validation.h"

namespace PocketWeb::PocketWebRpc
{
    RPCHelpMan GetAccountId()
    {
        return RPCHelpMan{"getaddressid",
                "\nGet id and address.\n",
                {
                    {"address", RPCArg::Type::STR, RPCArg::Optional::OMITTED_NAMED_ARG, ""},
                    {"id", RPCArg::Type::NUM, RPCArg::Optional::OMITTED_NAMED_ARG, ""}
                },
                RPCResult{
                    RPCResult::Type::OBJ, "", /* optional (means array may be empty) */ true, "",
                    {
                        {RPCResult::Type::STR, "address", ""},
                        {RPCResult::Type::STR, "id", ""}
                    }
                },
                RPCExamples{
                    HelpExampleCli("getaddressid", "123") +
                    HelpExampleRpc("getaddressid", "123") +
                    HelpExampleCli("getaddressid", "\"ab1123afd1231\"") +
                    HelpExampleRpc("getaddressid", "\"ab1123afd1231\"")
                },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
    {
        if (request.params.empty())
            throw JSONRPCError(RPC_INVALID_PARAMETER, "There is no arguments");

        if (request.params[0].isNum())
            return request.DbConnection()->WebRpcRepoInst->GetAddressId(request.params[0].get_int64());
        else if (request.params[0].isStr())
            return request.DbConnection()->WebRpcRepoInst->GetAddressId(request.params[0].get_str());

        throw JSONRPCError(RPC_INVALID_PARAMETER, "There is no arguments");
    },
        };
    }

    RPCHelpMan GetAccountProfiles()
    {
        return RPCHelpMan{"getuserprofile",
                "\nReturn Pocketnet user profile.\n",
                {
                    {"address", RPCArg::Type::STR, RPCArg::Optional::NO, ""},
                    {"shortForm", /* TODO (losty-fur): is this really string? */RPCArg::Type::STR, RPCArg::Optional::OMITTED_NAMED_ARG, ""}
                },
                {
                    // TODO (losty-fur): provide return description
                    
                },
                RPCExamples{
                    // TODO (losty-fur): provide correct examples
                    HelpExampleCli("getuserprofile", "123123abd123 1") +
                    HelpExampleRpc("getuserprofile", "123123abd123 1") +
                    HelpExampleCli("getuserprofile", "123123abd123") +
                    HelpExampleRpc("getuserprofile", "123123abd123")
                },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
    {
        UniValue result(UniValue::VARR);

        if (request.params.empty())
            return result;

        vector<string> addresses;
        if (request.params[0].isStr())
            addresses.push_back(request.params[0].get_str());
        else if (request.params[0].isArray())
        {
            UniValue addr = request.params[0].get_array();
            for (unsigned int idx = 0; idx < addr.size(); idx++)
                addresses.push_back(addr[idx].get_str());
        }

        if (addresses.empty())
            return result;

        // Short profile form is: address, b, i, name
        bool shortForm = false;
        if (request.params.size() > 1)
            shortForm = request.params[1].get_str() == "1";

        // Get data
        map<string, UniValue> profiles = request.DbConnection()->WebRpcRepoInst->GetAccountProfiles(addresses, shortForm);
        for (auto& p : profiles)
            result.push_back(p.second);

        return result;
    },
        };
    }

    RPCHelpMan GetAccountAddress()
    {
        return RPCHelpMan{"getuseraddress",
                "\nGet list addresses of user.\n",
                {
                    {"user_name", RPCArg::Type::STR, RPCArg::Optional::NO, ""},
                    {"count", RPCArg::Type::NUM, RPCArg::Optional::OMITTED_NAMED_ARG, ""}
                },
                {
                    // TODO (losty-fur): provide return description
                },
                RPCExamples{
                    // TODO (losty-fur): provide correct examples
                    HelpExampleCli("getuseraddress", "") +
                    HelpExampleRpc("getuseraddress", "")
                },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
    {
        RPCTypeCheck(request.params, {UniValue::VSTR});

        string userName = request.params[0].get_str();
        return request.DbConnection()->WebRpcRepoInst->GetUserAddress(userName);
    },
        };
    }

    RPCHelpMan GetAccountRegistration()
    {
        return RPCHelpMan{"getaddressregistration",
                "\nReturns array of registration dates.\n",
                {
                    {"addresses", RPCArg::Type::ARR, RPCArg::Optional::NO, "A json array of pocketcoin addresses to filter",
                        {
                            {"address", RPCArg::Type::STR, RPCArg::Optional::NO, ""}   
                        }
                    }
                },
                RPCResult{
                    RPCResult::Type::ARR, "", "", 
                    {
                        {
                            RPCResult::Type::OBJ, "", /* optional (means array may be empty) */ true, "",
                            {
                                {RPCResult::Type::STR, "address", "the pocketcoin address"},
                                {RPCResult::Type::NUM_TIME, "date", "date in Unix time format"},
                                // TODO (team): provide name for this arg
                                {RPCResult::Type::STR, "?", "id of first transaction with this address"}
                            }
                        }
                    },
                },
                RPCExamples{
                    // TODO (losty-fur): provide correct examples
                    HelpExampleCli("getaddressregistration", "[\"addresses\",...]") +
                    HelpExampleRpc("getaddressregistration", "[\"addresses\",...]")
                },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
    {
        if (request.params.empty())
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, string("Invalid or empty arguments"));

        vector<string> addresses;
        if (!request.params[0].isNull())
        {
            RPCTypeCheckArgument(request.params[0], UniValue::VARR);
            UniValue inputs = request.params[0].get_array();
            for (unsigned int idx = 0; idx < inputs.size(); idx++)
            {
                const UniValue& input = inputs[idx];
                CTxDestination dest = DecodeDestination(input.get_str());

                if (!IsValidDestination(dest))
                    throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, string("Invalid Pocketcoin address: ") + input.get_str());

                if (find(addresses.begin(), addresses.end(), input.get_str()) == addresses.end())
                    addresses.push_back(input.get_str());
            }
        }

        return request.DbConnection()->WebRpcRepoInst->GetAddressesRegistrationDates(addresses);
    },
        };
    }

    RPCHelpMan GetAccountState()
    {
        return RPCHelpMan{"getuserstate",
                "\nReturns account limits and rating information\n",
                {
                    {"address", RPCArg::Type::STR, RPCArg::Optional::NO, ""}
                },
                {
                    // TODO (losty-fur): provide return description
                },
                RPCExamples{
                    // TODO (losty-fur): provide correct examples
                    HelpExampleCli("getuserstate", "ad123ab123fd") +
                    HelpExampleRpc("getuserstate", "ad123ab123fd")
                },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
    {
        if (request.params.empty())
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, string("Invalid or empty Pocketcoin address"));

        RPCTypeCheckArgument(request.params[0], UniValue::VSTR);
        CTxDestination dest = DecodeDestination(request.params[0].get_str());
        if (!IsValidDestination(dest))
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, string("Invalid Pocketcoin address: ") + request.params[0].get_str());
        auto address = request.params[0].get_str();

        auto reputationConsensus = ReputationConsensusFactoryInst.Instance(::ChainActive().Height());
        auto windowDepth = reputationConsensus->GetConsensusLimit(ConsensusLimit_depth);

        // Read general account info and current state
        auto result = request.DbConnection()->WebRpcRepoInst->GetAccountState(address, ::ChainActive().Height() - windowDepth);
        if (result["address"].isNull())
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Pocketcoin address not found : " + address);

        // Calculate additional fields
        auto accountMode = reputationConsensus->GetAccountMode(result["reputation"].get_int(), result["balance"].get_int64());

        result.pushKV("mode", accountMode);
        result.pushKV("trial", accountMode == AccountMode_Trial);
        result.pushKV("reputation", result["reputation"].get_int() / 10.0);

        int64_t postLimit;
        int64_t videoLimit;
        int64_t complainLimit;
        int64_t commentLimit;
        int64_t scoreCommentLimit;
        int64_t scoreLimit;
        switch (accountMode)
        {
            case PocketConsensus::AccountMode_Trial:
                postLimit = reputationConsensus->GetConsensusLimit(ConsensusLimit_trial_post);
                videoLimit = reputationConsensus->GetConsensusLimit(ConsensusLimit_trial_video);
                complainLimit = reputationConsensus->GetConsensusLimit(ConsensusLimit_trial_complain);
                commentLimit = reputationConsensus->GetConsensusLimit(ConsensusLimit_trial_comment);
                scoreCommentLimit = reputationConsensus->GetConsensusLimit(ConsensusLimit_trial_comment_score);
                scoreLimit = reputationConsensus->GetConsensusLimit(ConsensusLimit_trial_score);
                break;
            case PocketConsensus::AccountMode_Full:
                postLimit = reputationConsensus->GetConsensusLimit(ConsensusLimit_full_post);
                videoLimit = reputationConsensus->GetConsensusLimit(ConsensusLimit_full_video);
                complainLimit = reputationConsensus->GetConsensusLimit(ConsensusLimit_full_complain);
                commentLimit = reputationConsensus->GetConsensusLimit(ConsensusLimit_full_comment);
                scoreCommentLimit = reputationConsensus->GetConsensusLimit(ConsensusLimit_full_comment_score);
                scoreLimit = reputationConsensus->GetConsensusLimit(ConsensusLimit_full_score);
                break;
            case PocketConsensus::AccountMode_Pro:
                postLimit = reputationConsensus->GetConsensusLimit(ConsensusLimit_full_post);
                videoLimit = reputationConsensus->GetConsensusLimit(ConsensusLimit_pro_video);
                complainLimit = reputationConsensus->GetConsensusLimit(ConsensusLimit_full_complain);
                commentLimit = reputationConsensus->GetConsensusLimit(ConsensusLimit_full_comment);
                scoreCommentLimit = reputationConsensus->GetConsensusLimit(ConsensusLimit_full_comment_score);
                scoreLimit = reputationConsensus->GetConsensusLimit(ConsensusLimit_full_score);
                break;
        }

        if (!result["post_spent"].isNull())
            result.pushKV("post_unspent", postLimit - result["post_spent"].get_int());

        if (!result["video_spent"].isNull())
            result.pushKV("video_unspent", videoLimit - result["video_spent"].get_int());

        if (!result["complain_spent"].isNull())
            result.pushKV("complain_unspent", complainLimit - result["complain_spent"].get_int());

        if (!result["comment_spent"].isNull())
            result.pushKV("comment_unspent", commentLimit - result["comment_spent"].get_int());

        if (!result["comment_score_spent"].isNull())
            result.pushKV("comment_score_unspent", scoreCommentLimit - result["comment_score_spent"].get_int());

        if (!result["score_spent"].isNull())
            result.pushKV("score_unspent", scoreLimit - result["score_spent"].get_int());

        return result;
    },
        };
    }

    RPCHelpMan GetAccountUnspents()
    {
        return RPCHelpMan{"txunspent",
                "\nReturns array of unspent transaction outputs\n"
                "with between minconf and maxconf (inclusive) confirmations.\n"
                "Optionally filter to only include txouts paid to specified addresses.\n",
                {
                    {"addresses", RPCArg::Type::ARR, RPCArg::Optional::OMITTED_NAMED_ARG, "A json array of pocketcoin addresses to filter",
                        {
                            {"address", RPCArg::Type::STR, RPCArg::Optional::NO, ""}
                        }
                    },
                    {"minconf", RPCArg::Type::NUM, RPCArg::Optional::OMITTED_NAMED_ARG, "The minimum confirmations to filter (default=1)"},
                    {"maxconf", RPCArg::Type::NUM, RPCArg::Optional::OMITTED_NAMED_ARG, "The maximum confirmations to filter (default=9999999)"},

                },
                {
                    // TODO (losty-fur): provide return description
                },
                RPCExamples{
                    // TODO (losty-fur): provide correct examples
                    HelpExampleCli("txunspent", "") +
                    HelpExampleRpc("txunspent", "")
                },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
    {
        // TODO (brangr): add pagination

        vector<string> destinations;
        if (request.params.size() > 0)
        {
            RPCTypeCheckArgument(request.params[0], UniValue::VARR);
            UniValue inputs = request.params[0].get_array();
            for (unsigned int idx = 0; idx < inputs.size(); idx++)
            {
                const UniValue& input = inputs[idx];
                CTxDestination dest = DecodeDestination(input.get_str());

                if (!IsValidDestination(dest))
                {
                    throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, string("Invalid Pocketcoin address: ") + input.get_str());
                }

                if (find(destinations.begin(), destinations.end(), input.get_str()) == destinations.end())
                {
                    destinations.push_back(input.get_str());
                }
            }
        }
        if (destinations.empty())
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, string("Invalid Pocketcoin addresses"));

        // int minConf = 1;
        // if (request.params.size() > 1) {
        //     RPCTypeCheckArgument(request.params[1], UniValue::VNUM);
        //     minConf = request.params[1].get_int();
        // }

        // int maxConf = 9999999;
        // if (request.params.size() > 2) {
        //     RPCTypeCheckArgument(request.params[2], UniValue::VNUM);
        //     maxConf = request.params[2].get_int();
        // }

        // TODO: filter by amount
        // TODO: filter by depth
        // bool include_unsafe = true;
        // if (request.params.size() > 3) {
        //     RPCTypeCheckArgument(request.params[3], UniValue::VBOOL);
        //     include_unsafe = request.params[3].get_bool();
        // }

        // CAmount nMinimumAmount = 0;
        // CAmount nMaximumAmount = MAX_MONEY;
        // CAmount nMinimumSumAmount = MAX_MONEY;
        // uint64_t nMaximumCount = UINTMAX_MAX;

        // if (request.params.size() > 4) {
        //     const UniValue& options = request.params[4].get_obj();

        //     if (options.exists("minimumAmount"))
        //         nMinimumAmount = AmountFromValue(options["minimumAmount"]);

        //     if (options.exists("maximumAmount"))
        //         nMaximumAmount = AmountFromValue(options["maximumAmount"]);

        //     if (options.exists("minimumSumAmount"))
        //         nMinimumSumAmount = AmountFromValue(options["minimumSumAmount"]);

        //     if (options.exists("maximumCount"))
        //         nMaximumCount = options["maximumCount"].get_int64();
        // }

        const auto& node = EnsureNodeContext(request.context);
        // Get exclude inputs already used in mempool
        vector<pair<string, uint32_t>> mempoolInputs;
        // TODO (losty-fur): possible null mempool
        node.mempool->GetAllInputs(mempoolInputs);

        // Get unspents from DB
        return request.DbConnection()->WebRpcRepoInst->GetUnspents(destinations, ::ChainActive().Height(), mempoolInputs);
    },
        };
    }

    RPCHelpMan GetAccountSetting()
    {
        return RPCHelpMan{"getaccountsetting",
                "\nReturn account settings object.\n",
                {
                    {"address", RPCArg::Type::STR, RPCArg::Optional::NO, ""}
                },
                {
                    // TODO (losty-fur): provide return description
                },
                RPCExamples{
                    // TODO (losty-fur): provide correct examples
                    HelpExampleCli("getaccountsetting", "123abacf12") +
                    HelpExampleRpc("getaccountsetting", "123abacf12")
                },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
    {
        RPCTypeCheck(request.params, {UniValue::VSTR});

        string address = request.params[0].get_str();

        return request.DbConnection()->WebRpcRepoInst->GetAccountSetting(address);
    },
        };
    }

    RPCHelpMan GetAccountStatistic()
    {
        return RPCHelpMan{"getuserstatistic",
                "\nGet user statistic.\n",
                {
                    {"addresses", RPCArg::Type::ARR, RPCArg::Optional::NO, "Addresses for statistic",
                        {
                            {"address", RPCArg::Type::STR, RPCArg::Optional::NO, ""}   
                        }
                    },
                    {"height", RPCArg::Type::NUM, RPCArg::Optional::OMITTED_NAMED_ARG, "Maximum search height. Default is current chain height"},
                    {"depth", RPCArg::Type::NUM, RPCArg::Optional::OMITTED_NAMED_ARG, "Depth of statistic. Default - whole history"},
                },
                {
                    // TODO (losty-fur): provide return description
                },
                RPCExamples{
                    // TODO (losty-fur): provide correct examples
                    HelpExampleCli("getuserstatistic", "[\"addresses\", ...], height, depth") +
                    HelpExampleRpc("getuserstatistic", "[\"addresses\", ...], height, depth")
                },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
    {
        std::string address;
        std::vector<std::string> addresses;
        if (request.params.size() > 0) {
            if (request.params[0].isStr()) {
                address = request.params[0].get_str();
                CTxDestination dest = DecodeDestination(address);
                if (!IsValidDestination(dest)) {
                    throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, std::string("Invalid Pocketnet address: ") + address);
                }
                addresses.emplace_back(address);
            } else if (request.params[0].isArray()) {
                UniValue addrs = request.params[0].get_array();
                if (addrs.size() > 10) {
                    throw JSONRPCError(RPC_INVALID_PARAMS, "Too large array");
                }
                if(addrs.size() > 0) {
                    for (unsigned int idx = 0; idx < addrs.size(); idx++) {
                        address = addrs[idx].get_str();
                        CTxDestination dest = DecodeDestination(address);
                        if (!IsValidDestination(dest)) {
                            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, std::string("Invalid Pocketnet address: ") + address);
                        }
                        addresses.emplace_back(address);
                    }
                }
            }
        }

        int nHeight = ::ChainActive().Height();
        if (request.params.size() > 1) {
            if (request.params[1].isNum()) {
                if (request.params[1].get_int() > 0) {
                    nHeight = request.params[1].get_int();
                }
            }
        }

        int depth = ::ChainActive().Height();
        if (request.params.size() > 2) {
            if (request.params[2].isNum()) {
                if (request.params[2].get_int() > 0) {
                    depth = request.params[2].get_int();
                }
            }
        }

        return request.DbConnection()->WebRpcRepoInst->GetUserStatistic(addresses, nHeight, depth);
    },
        };
    }

    RPCHelpMan GetAccountSubscribes()
    {
        return RPCHelpMan{"GetAccountSubscribes",
                "\nReturn subscribes accounts list with pagination - NOT IMPLEMENTED\n",
                {
                    {"address", RPCArg::Type::STR, RPCArg::Optional::NO, ""},
                },
                {
                    // TODO (losty-fur): provide return description
                },
                RPCExamples{
                    // TODO (losty-fur): provide correct examples
                    HelpExampleCli("GetAccountSubscribes", "\"address\"") +
                    HelpExampleRpc("GetAccountSubscribes", "\"address\"")
                },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
    {
        RPCTypeCheck(request.params, {UniValue::VSTR});

        string address = request.params[0].get_str();

        return request.DbConnection()->WebRpcRepoInst->GetSubscribesAddresses(address);
    },
        };
    }

    RPCHelpMan GetAccountSubscribers()
    {
        return RPCHelpMan{"GetAccountSubscribers",
                "\nReturn subscribers accounts list with pagination - NOT IMPLEMENTED\n",
                {
                    {"address", RPCArg::Type::STR, RPCArg::Optional::NO, ""},
                },
                {
                    // TODO (losty-fur): provide return description
                },
                RPCExamples{
                    // TODO (losty-fur): provide correct examples
                    HelpExampleCli("GetAccountSubscribers", "\"address\"") +
                    HelpExampleRpc("GetAccountSubscribers", "\"address\"")
                },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
    {
        RPCTypeCheck(request.params, {UniValue::VSTR});

        string address = request.params[0].get_str();

        return request.DbConnection()->WebRpcRepoInst->GetSubscribersAddresses(address);
    },
        };
    }

    RPCHelpMan GetAccountBlockings()
    {
        return RPCHelpMan{"GetAccountBlockings",
                "\nReturn blocked accounts list with pagination - NOT IMPLEMENTED\n",
                {
                    {"address", RPCArg::Type::STR, RPCArg::Optional::NO, ""},
                },
                {
                    // TODO (losty-fur): provide return description
                },
                RPCExamples{
                    // TODO (losty-fur): provide correct examples
                    HelpExampleCli("GetAccountBlockings", "\"address\"") +
                    HelpExampleRpc("GetAccountBlockings", "\"address\"")
                },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
    {
        RPCTypeCheck(request.params, {UniValue::VSTR});

        string address = request.params[0].get_str();

        return request.DbConnection()->WebRpcRepoInst->GetBlockingToAddresses(address);
    },
        };
    }

}