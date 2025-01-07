// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include "pocketdb/web/PocketAccountRpc.h"
#include "rpc/blockchain.h"
#include "rpc/util.h"
#include "validation.h"
#include "util/html.h"

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
                    {"shortForm", /* TODO (rpc): is this really string? */RPCArg::Type::STR, RPCArg::Optional::OMITTED_NAMED_ARG, ""}
                },
                {
                    // TODO (rpc): provide return description
                    
                },
                RPCExamples{
                    // TODO (rpc): provide correct examples
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

        // Count of days for first flags after first content publish
        int firstFlagsDepth = 7;
        if (request.params.size() > 2 && request.params[2].isNum() && request.params[2].get_int() > 0)
            firstFlagsDepth = request.params[2].get_int();

        // Get data
        map<string, UniValue> profiles = request.DbConnection()->WebRpcRepoInst->GetAccountProfiles(addresses, shortForm, firstFlagsDepth);
        for (auto& p : profiles)
            result.push_back(p.second);

        return result;
    },
        };
    }

    RPCHelpMan GetAccountVersions()
    {
        return RPCHelpMan{"getaccountversions",
            "\nGet account versions.\n",
            {
                {"address", RPCArg::Type::STR, RPCArg::Optional::NO, ""},
                {"topHeight", RPCArg::Type::NUM, RPCArg::Optional::OMITTED_NAMED_ARG, "Start height of pagination"},
                {"pageStart", RPCArg::Type::NUM, RPCArg::Optional::OMITTED_NAMED_ARG, "Start page"},
                {"pageSize", RPCArg::Type::NUM, RPCArg::Optional::OMITTED_NAMED_ARG, "Size page"},
            },
            {
                // TODO (rpc): provide return description
            },
            RPCExamples{
                // TODO (rpc): provide correct examples
                HelpExampleCli("getaccountversions", "") +
                HelpExampleRpc("getaccountversions", "")
            },
            [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
            {
                RPCTypeCheck(request.params, {UniValue::VSTR});

                string address = request.params[0].get_str();

                Pagination pagination{ ChainActiveSafeHeight(), 0, 10, "height", true };
                if (request.params[1].isNum())
                    pagination.TopHeight = min(request.params[1].get_int(), pagination.TopHeight);
                if (request.params[2].isNum())
                    pagination.PageStart = request.params[2].get_int();
                if (request.params[3].isNum())
                    pagination.PageSize = request.params[3].get_int();

                return request.DbConnection()->WebRpcRepoInst->GetAccountVersions(address, pagination);
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
                    // TODO (rpc): provide return description
                },
                RPCExamples{
                    // TODO (rpc): provide correct examples
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
                    // TODO (rpc): provide correct examples
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
                    // TODO (rpc): provide return description
                },
                RPCExamples{
                    // TODO (rpc): provide correct examples
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

        auto reputationConsensus = ConsensusFactoryInst_Reputation.Instance(ChainActiveSafeHeight());
        auto windowDepth = reputationConsensus->GetConsensusLimit(ConsensusLimit_depth);

        // Read general account info and current state
        auto result = request.DbConnection()->WebRpcRepoInst->GetAccountState(address, ChainActiveSafeHeight() - windowDepth);
        if (result["address"].isNull())
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Pocketcoin address not found : " + address);

        // Check account permissions
        auto accountsData = request.DbConnection()->ConsensusRepoInst->GetAccountsData({ address });
        auto accountData = accountsData[address];
        
        auto accountMode = reputationConsensus->GetAccountMode(accountData.Reputation, accountData.Balance);
        result.pushKV("mode", accountMode);
        result.pushKV("trial", accountMode == AccountMode_Trial);
        result.pushKV("reputation", accountData.Reputation / 10.0);

        // Extend result data with badges array
        auto badgeSet = reputationConsensus->GetBadges(accountData);
        UniValue badges = badgeSet.ToJson();
        result.pushKV("badges", badges);

        result.pushKV("user_reg_date", accountData.RegistrationTime);
        result.pushKV("user_reg_height", accountData.RegistrationHeight);
        result.pushKV("balance", accountData.Balance);
        result.pushKV("likers", accountData.LikersAll());

        // Spent/Unspent limits
        int64_t postLimit;
        int64_t videoLimit;
        int64_t articleLimit;
        int64_t streamLimit;
        int64_t audioLimit;
        int64_t complainLimit;
        int64_t commentLimit;
        int64_t scoreCommentLimit;
        int64_t scoreLimit;
        switch (accountMode)
        {
            case PocketConsensus::AccountMode_Trial:
                postLimit = reputationConsensus->GetConsensusLimit(ConsensusLimit_trial_post);
                videoLimit = reputationConsensus->GetConsensusLimit(ConsensusLimit_trial_video);
                articleLimit = reputationConsensus->GetConsensusLimit(ConsensusLimit_trial_article);
                streamLimit = reputationConsensus->GetConsensusLimit(ConsensusLimit_trial_stream);
                audioLimit = reputationConsensus->GetConsensusLimit(ConsensusLimit_trial_audio);
                complainLimit = reputationConsensus->GetConsensusLimit(ConsensusLimit_trial_complain);
                commentLimit = reputationConsensus->GetConsensusLimit(ConsensusLimit_trial_comment);
                scoreCommentLimit = reputationConsensus->GetConsensusLimit(ConsensusLimit_trial_comment_score);
                scoreLimit = reputationConsensus->GetConsensusLimit(ConsensusLimit_trial_score);
                break;
            case PocketConsensus::AccountMode_Full:
                postLimit = reputationConsensus->GetConsensusLimit(ConsensusLimit_full_post);
                videoLimit = reputationConsensus->GetConsensusLimit(ConsensusLimit_full_video);
                articleLimit = reputationConsensus->GetConsensusLimit(ConsensusLimit_full_article);
                streamLimit = reputationConsensus->GetConsensusLimit(ConsensusLimit_full_stream);
                audioLimit = reputationConsensus->GetConsensusLimit(ConsensusLimit_full_audio);
                complainLimit = reputationConsensus->GetConsensusLimit(ConsensusLimit_full_complain);
                commentLimit = reputationConsensus->GetConsensusLimit(ConsensusLimit_full_comment);
                scoreCommentLimit = reputationConsensus->GetConsensusLimit(ConsensusLimit_full_comment_score);
                scoreLimit = reputationConsensus->GetConsensusLimit(ConsensusLimit_full_score);
                break;
            case PocketConsensus::AccountMode_Pro:
                postLimit = reputationConsensus->GetConsensusLimit(ConsensusLimit_full_post);
                videoLimit = reputationConsensus->GetConsensusLimit(ConsensusLimit_pro_video);
                articleLimit = reputationConsensus->GetConsensusLimit(ConsensusLimit_full_article);
                streamLimit = reputationConsensus->GetConsensusLimit(ConsensusLimit_full_stream);
                audioLimit = reputationConsensus->GetConsensusLimit(ConsensusLimit_full_audio);
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

        if (!result["article_spent"].isNull())
            result.pushKV("article_unspent", articleLimit - result["article_spent"].get_int());

        if (!result["stream_spent"].isNull())
            result.pushKV("stream_unspent", streamLimit - result["stream_spent"].get_int());

        if (!result["audio_spent"].isNull())
            result.pushKV("audio_unspent", audioLimit - result["audio_spent"].get_int());

        if (!result["complain_spent"].isNull())
            result.pushKV("complain_unspent", complainLimit - result["complain_spent"].get_int());

        if (!result["comment_spent"].isNull())
            result.pushKV("comment_unspent", commentLimit - result["comment_spent"].get_int());

        if (!result["comment_score_spent"].isNull())
            result.pushKV("comment_score_unspent", scoreCommentLimit - result["comment_score_spent"].get_int());

        if (!result["score_spent"].isNull())
            result.pushKV("score_unspent", scoreLimit - result["score_spent"].get_int());

        if (!result["mod_flag_spent"].isNull())
            result.pushKV("mod_flag_unspent", reputationConsensus->GetConsensusLimit(moderation_flag_count) - result["mod_flag_spent"].get_int());

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
                    // TODO (rpc): provide return description
                },
                RPCExamples{
                    // TODO (rpc): provide correct examples
                    HelpExampleCli("txunspent", "") +
                    HelpExampleRpc("txunspent", "")
                },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
    {
        // TODO (aok): add pagination

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
        CHECK_NONFATAL(node.mempool);
        node.mempool->GetAllInputs(mempoolInputs);

        // Get unspents from DB
        int height = ChainActiveSafeHeight();
        return request.DbConnection()->WebRpcRepoInst->GetUnspents(destinations, height, 0, mempoolInputs);
    },
        };
    }

    RPCHelpMan GetAccountEarning()
    {
        return RPCHelpMan{"getaccountearning",
                          "\nReturns account earnings for the period\n",
                          {
                                  {"address", RPCArg::Type::STR, RPCArg::Optional::OMITTED_NAMED_ARG, "A string of pocketcoin addresses",},
                                  {"height", RPCArg::Type::NUM, RPCArg::Optional::OMITTED_NAMED_ARG, "Maximum height for filter. Default is current chain height"},
                                  {"depth", RPCArg::Type::NUM, RPCArg::Optional::OMITTED_NAMED_ARG, "Depth for query. Default is 43200 (about 1 month)"},

                          },
                          {
                                  // TODO (rpc): provide return description
                          },
                          RPCExamples{
                                  // TODO (rpc): provide correct examples
                                  HelpExampleCli("getaccountearning", "\"ab1123afd1231\"") +
                                  HelpExampleRpc("getaccountearning", "\"ab1123afd1231\"")
                          },
                          [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
                          {
                              std::string address;
                              if (request.params.size() > 0)
                              {
                                  if (request.params[0].isStr())
                                  {
                                      address = request.params[0].get_str();
                                      CTxDestination dest = DecodeDestination(address);
                                      if (!IsValidDestination(dest))
                                      {
                                          throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY,
                                                             std::string("Invalid Pocketnet address: ") + address);
                                      }
                                  }
                              }

                              int height = ChainActiveSafeHeight();
                              if (request.params.size() > 1) {
                                  if (request.params[1].isNum()) {
                                      if (request.params[1].get_int() > 0) {
                                          height = std::min(request.params[1].get_int(), height);
                                      }
                                  }
                              }

                              int depth = 43200;
                              if (request.params.size() > 2) {
                                  if (request.params[2].isNum()) {
                                      if (request.params[2].get_int() > 0) {
                                          depth = request.params[2].get_int();
                                      }
                                  }
                              }
                              return request.DbConnection()->WebRpcRepoInst->GetAccountEarning(address, height, depth);
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
                    // TODO (rpc): provide return description
                },
                RPCExamples{
                    // TODO (rpc): provide correct examples
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
                    {"depthR", RPCArg::Type::NUM, RPCArg::Optional::OMITTED_NAMED_ARG, "Depth of referral statistic. Default - whole history"},
                    {"depthC", RPCArg::Type::NUM, RPCArg::Optional::OMITTED_NAMED_ARG, "Depth of commentator statistic. Default - same as depthR"},
                    {"cntC",   RPCArg::Type::NUM, RPCArg::Optional::OMITTED_NAMED_ARG, "Count of minimum comments for commentator statistic. Default - 1"}
                },
                {
                    // TODO (rpc): provide return description
                },
                RPCExamples{
                    // TODO (rpc): provide correct examples
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

        int nHeight = ChainActiveSafeHeight();
        if (request.params.size() > 1) {
            if (request.params[1].isNum()) {
                if (request.params[1].get_int() > 0) {
                    nHeight = request.params[1].get_int();
                }
            }
        }

        int depthR = ChainActiveSafeHeight();
        if (request.params.size() > 2) {
            if (request.params[2].isNum()) {
                if (request.params[2].get_int() > 0) {
                    depthR = request.params[2].get_int();
                }
            }
        }

        int depthC = depthR;
        if (request.params.size() > 3) {
            if (request.params[3].isNum()) {
                if (request.params[3].get_int() > 0) {
                    depthC = request.params[3].get_int();
                }
            }
        }

        int cntC = 1;
        if (request.params.size() > 4) {
            if (request.params[4].isNum()) {
                if (request.params[4].get_int() > 0) {
                    cntC = request.params[4].get_int();
                }
            }
        }

        return request.DbConnection()->WebRpcRepoInst->GetUserStatistic(addresses, nHeight, depthR, depthC, cntC);
    },
        };
    }

    RPCHelpMan GetAccountSubscribes()
    {
        return RPCHelpMan{
            "getusersubscribes",
            "\nReturn subscribes accounts list with pagination\n",
            {
                {"address", RPCArg::Type::STR, RPCArg::Optional::NO, "Address for filter"},
                {"orderby", RPCArg::Type::STR, RPCArg::Optional::OMITTED_NAMED_ARG, "Order by field (reputation|height) (Default: height)"},
                {"orderdesc", RPCArg::Type::BOOL, RPCArg::Optional::OMITTED_NAMED_ARG, "Order by desc (Default: true)"},
                {"offset", RPCArg::Type::NUM, RPCArg::Optional::OMITTED_NAMED_ARG, "Skip first N records (Default: 0)"},
                {"limit", RPCArg::Type::NUM, RPCArg::Optional::OMITTED_NAMED_ARG, "Limit N result records (Default: 10) (0 - not limitted)"},
            },
            {
                // TODO (rpc): provide return description
            },
            RPCExamples{
                // TODO (rpc): provide correct examples
                HelpExampleCli("getusersubscribes", "\"address\"") +
                HelpExampleRpc("getusersubscribes", "\"address\"")
            },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
        {
            RPCTypeCheck(request.params, {UniValue::VSTR});

            string address = request.params[0].get_str();

            string orderBy = "height";
            if (request.params.size() > 1 && request.params[1].isStr())
            {
                orderBy = request.params[1].get_str();
                HtmlUtils::StringToLower(orderBy);
            }

            bool orderDesc = true;
            if (request.params.size() > 2 && request.params[2].isBool())
                orderDesc = request.params[2].get_bool();

            int offset = 0;
            if (request.params.size() > 3 && request.params[3].isNum())
                offset = min(0, request.params[3].get_int());

            int limit = 10;
            if (request.params.size() > 4 && request.params[4].isNum())
                limit = min(0, request.params[4].get_int());

            return request.DbConnection()->WebRpcRepoInst->GetSubscribesAddresses(
                address, { ACTION_SUBSCRIBE, ACTION_SUBSCRIBE_PRIVATE }, orderBy, orderDesc, offset, limit);
        }};
    }

    RPCHelpMan GetAccountSubscribers()
    {
        return RPCHelpMan{
            "getusersubscribers",
            "\nReturn subscribers accounts list with pagination\n",
            {
                {"address", RPCArg::Type::STR, RPCArg::Optional::NO, "Address for filter"},
                {"orderby", RPCArg::Type::STR, RPCArg::Optional::OMITTED_NAMED_ARG, "Order by field (reputation|height) (Default: height)"},
                {"orderdesc", RPCArg::Type::BOOL, RPCArg::Optional::OMITTED_NAMED_ARG, "Order by desc (Default: true)"},
                {"offset", RPCArg::Type::NUM, RPCArg::Optional::OMITTED_NAMED_ARG, "Skip first N records (Default: 0)"},
                {"limit", RPCArg::Type::NUM, RPCArg::Optional::OMITTED_NAMED_ARG, "Limit N result records (Default: 10) (0 - not limitted)"},
            },
            {
                // TODO (rpc): provide return description
            },
            RPCExamples{
                // TODO (rpc): provide correct examples
                HelpExampleCli("getusersubscribers", "\"address\"") +
                HelpExampleRpc("getusersubscribers", "\"address\"")
            },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
        {
            RPCTypeCheck(request.params, {UniValue::VSTR});

            string address = request.params[0].get_str();

            string orderBy = "height";
            if (request.params.size() > 1 && request.params[1].isStr())
            {
                orderBy = request.params[1].get_str();
                HtmlUtils::StringToLower(orderBy);
            }

            bool orderDesc = true;
            if (request.params.size() > 2 && request.params[2].isBool())
                orderDesc = request.params[2].get_bool();

            int offset = 0;
            if (request.params.size() > 3 && request.params[3].isNum())
                offset = min(0, request.params[3].get_int());

            int limit = 10;
            if (request.params.size() > 4 && request.params[4].isNum())
                limit = min(0, request.params[4].get_int());

            return request.DbConnection()->WebRpcRepoInst->GetSubscribersAddresses(
                address, { ACTION_SUBSCRIBE, ACTION_SUBSCRIBE_PRIVATE }, orderBy, orderDesc, offset, limit);
        }};
    }

    RPCHelpMan GetAccountBlockings()
    {
        return RPCHelpMan{"GetAccountBlockings",
                "\nReturn blocked accounts list\n",
                {
                    {"address", RPCArg::Type::STR, RPCArg::Optional::NO, ""},
                    {"useaddresses", RPCArg::Type::BOOL, RPCArg::Optional::OMITTED_NAMED_ARG, "Use addresses instead numbers"},
                },
                {
                    // TODO (rpc): provide return description
                },
                RPCExamples{
                    // TODO (rpc): provide correct examples
                    HelpExampleCli("GetAccountBlockings", "\"address\"") +
                    HelpExampleRpc("GetAccountBlockings", "\"address\"")
                },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
    {
        RPCTypeCheck(request.params, {UniValue::VSTR});

        string address = request.params[0].get_str();

        bool useAddresses = false;
        if (request.params.size() > 1)
        {
            if (request.params[1].isBool())
                useAddresses = request.params[1].get_bool();
            else if (request.params[1].isNum())
                useAddresses = (request.params[1].get_int() == 1);
        }

        return request.DbConnection()->WebRpcRepoInst->GetBlockings(address, useAddresses);
    },
        };
    }

    RPCHelpMan GetAccountBlockers()
    {
        return RPCHelpMan{"GetAccountBlockers",
                "\nReturns a list of accounts that have blocked the specified address\n",
                {
                    {"address", RPCArg::Type::STR, RPCArg::Optional::NO, ""},
                    {"useaddresses", RPCArg::Type::BOOL, RPCArg::Optional::OMITTED_NAMED_ARG, "Use addresses instead numbers"},
                },
                {
                    // TODO (rpc): provide return description
                },
                RPCExamples{
                    // TODO (rpc): provide correct examples
                    HelpExampleCli("GetAccountBlockings", "\"address\"") +
                    HelpExampleRpc("GetAccountBlockings", "\"address\"")
                },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
    {
        RPCTypeCheck(request.params, {UniValue::VSTR});

        string address = request.params[0].get_str();

        bool useAddresses = false;
        if (request.params.size() > 1)
        {
            if (request.params[1].isBool())
                useAddresses = request.params[1].get_bool();
            else if (request.params[1].isNum())
                useAddresses = (request.params[1].get_int() == 1);
        }

        return request.DbConnection()->WebRpcRepoInst->GetBlockers(address, useAddresses);
    },
        };
    }

    RPCHelpMan GetTopAccounts()
    {
        return RPCHelpMan{"GetTopAccounts",
            "\nReturn top accounts based on their content ratings\n",
            {
                    // TODO (rpc): provide inputs description
            },
            {
                    // TODO (rpc): provide return description
            },
            RPCExamples{
                    // TODO (rpc): provide correct examples
                    HelpExampleCli("GetTopAccounts", "\"address\"") +
                    HelpExampleRpc("GetTopAccounts", "\"address\"")
            },
            [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
            {
                int topHeight = ChainActiveSafeHeight();
                if (request.params.size() > 0 && request.params[0].isNum() && request.params[0].get_int() > 0)
                    topHeight = request.params[0].get_int();

                int countOut = 15;
                if (request.params.size() > 1 && request.params[1].isNum())
                    countOut = request.params[1].get_int();

                string lang = "";
                if (request.params.size() > 2 && request.params[2].isStr())
                    lang = request.params[2].get_str();

                vector<string> tags;
                if (request.params.size() > 3)
                    ParseRequestTags(request.params[3], tags);

                vector<int> contentTypes;
                if (request.params.size() > 4)
                    ParseRequestContentTypes(request.params[4], contentTypes);

                vector<string> adrsExcluded;
                if (request.params.size() > 5)
                {
                    if (request.params[5].isStr() && !request.params[5].get_str().empty())
                    {
                        adrsExcluded.push_back(request.params[5].get_str());
                    }
                    else if (request.params[5].isArray())
                    {
                        UniValue adrs = request.params[5].get_array();
                        for (unsigned int idx = 0; idx < adrs.size(); idx++)
                        {
                            string adrEx = boost::trim_copy(adrs[idx].get_str());
                            if (!adrEx.empty())
                                adrsExcluded.push_back(adrEx);

                            if (adrsExcluded.size() > 100)
                                break;
                        }
                    }
                }

                vector<string> tagsExcluded;
                if (request.params.size() > 6)
                    ParseRequestTags(request.params[6], tagsExcluded);

                int depth = 60 * 24 * 30 * 12; // about 1 year
                if (request.params.size() > 7)
                {
                    RPCTypeCheckArgument(request.params[7], UniValue::VNUM);
                    depth = std::min(depth, request.params[7].get_int());
                }

                auto reputationConsensus = ConsensusFactoryInst_Reputation.Instance(ChainActiveSafeHeight());
                auto badReputationLimit = reputationConsensus->GetConsensusLimit(ConsensusLimit_bad_reputation);

                UniValue result(UniValue::VARR);
                auto ids =  request.DbConnection()->WebRpcRepoInst->GetTopAccounts(topHeight, countOut, lang, tags, contentTypes,
                adrsExcluded, tagsExcluded, depth, badReputationLimit);
                if (!ids.empty())
                {
                    auto profiles = request.DbConnection()->WebRpcRepoInst->GetAccountProfiles(ids);
                    for (const auto[id, record] : profiles)
                        result.push_back(record);
                }

                return result;
            },
        };
    }

}