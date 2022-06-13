// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include "pocketdb/web/PocketAccountRpc.h"

namespace PocketWeb::PocketWebRpc
{
    UniValue GetAccountId(const JSONRPCRequest& request)
    {
        if (request.fHelp)
            throw runtime_error(
                "getaddressid \"address\" or \"id\"\n"
                "\nGet id and address.\n");

        if (request.params.empty())
            throw JSONRPCError(RPC_INVALID_PARAMETER, "There is no arguments");

        if (request.params[0].isNum())
            return request.DbConnection()->WebRpcRepoInst->GetAddressId(request.params[0].get_int64());
        else if (request.params[0].isStr())
            return request.DbConnection()->WebRpcRepoInst->GetAddressId(request.params[0].get_str());

        throw JSONRPCError(RPC_INVALID_PARAMETER, "There is no arguments");
    }

    UniValue GetAccountProfiles(const JSONRPCRequest& request)
    {
        if (request.fHelp)
            throw runtime_error(
                "getuserprofile \"address\" ( shortForm )\n"
                "\nReturn Pocketnet user profile.\n");

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
    }

    UniValue GetAccountAddress(const JSONRPCRequest& request)
    {
        if (request.fHelp)
            throw runtime_error(
                "getuseraddress \"user_name\" ( count )\n"
                "\nGet list addresses of user.\n");

        RPCTypeCheck(request.params, {UniValue::VSTR});

        string userName = request.params[0].get_str();
        return request.DbConnection()->WebRpcRepoInst->GetUserAddress(userName);
    }

    UniValue GetAccountRegistration(const JSONRPCRequest& request)
    {
        if (request.fHelp)
        {
            throw runtime_error(
                "getaddressregistration [\"addresses\",...]\n"
                "\nReturns array of registration dates.\n"
                "\nArguments:\n"
                "1. \"addresses\"      (string) A json array of pocketcoin addresses to filter\n"
                "    [\n"
                "      \"address\"     (string) pocketcoin address\n"
                "      ,...\n"
                "    ]\n"
                "\nResult\n"
                "[                             (array of json objects)\n"
                "  {\n"
                "    \"address\" : \"1PGFqEzfmQch1gKD3ra4k18PNj3tTUUSqg\",     (string) the pocketcoin address\n"
                "    \"date\" : \"1544596205\",                                (int64) date in Unix time format\n"
                "    \"date\" : \"2378659...\"                                 (string) id of first transaction with this address\n"
                "  },\n"
                "  ,...\n"
                "]");
        }

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
    }

    UniValue GetAccountState(const JSONRPCRequest& request)
    {
        if (request.fHelp)
            throw runtime_error(
                "getuserstate \"address\"\n"
                "\nReturns account limits and rating information\n"
            );

        if (request.params.empty())
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, string("Invalid or empty Pocketcoin address"));

        RPCTypeCheckArgument(request.params[0], UniValue::VSTR);
        CTxDestination dest = DecodeDestination(request.params[0].get_str());
        if (!IsValidDestination(dest))
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, string("Invalid Pocketcoin address: ") + request.params[0].get_str());
        auto address = request.params[0].get_str();

        auto reputationConsensus = ReputationConsensusFactoryInst.Instance(chainActive.Height());
        auto windowDepth = reputationConsensus->GetConsensusLimit(ConsensusLimit_depth);

        // Read general account info and current state
        auto result = request.DbConnection()->WebRpcRepoInst->GetAccountState(address, chainActive.Height() - windowDepth);
        if (result["address"].isNull())
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Pocketcoin address not found : " + address);

        // Check account permissions
        auto accountMode = reputationConsensus->GetAccountMode(accountData.Reputation, accountData.Balance);
        result.pushKV("mode", accountMode);
        result.pushKV("trial", accountMode == AccountMode_Trial);
        result.pushKV("reputation", accountData.Reputation / 10.0);

        // Extend result data with badges array
        const AccountData accountData = request.DbConnection()->ConsensusRepoInst->GetAccountData(address);
        auto badgeSet = reputationConsensus->GetBadges(accountData);
        UniValue badges = badgeSet.ToJson();
        result.pushKV("badges", badges);

        result.pushKV("user_reg_date", accountData.RegistrationTime);
        result.pushKV("user_reg_height", accountData.RegistrationHeight);
        result.pushKV("reputation", accountData.Reputation);
        result.pushKV("balance", accountData.Balance);
        result.pushKV("likers", accountData.LikersAll());

        // Spent/Unspent limits
        int64_t postLimit;
        int64_t videoLimit;
        int64_t articleLimit;
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
                complainLimit = reputationConsensus->GetConsensusLimit(ConsensusLimit_trial_complain);
                commentLimit = reputationConsensus->GetConsensusLimit(ConsensusLimit_trial_comment);
                scoreCommentLimit = reputationConsensus->GetConsensusLimit(ConsensusLimit_trial_comment_score);
                scoreLimit = reputationConsensus->GetConsensusLimit(ConsensusLimit_trial_score);
                break;
            case PocketConsensus::AccountMode_Full:
                postLimit = reputationConsensus->GetConsensusLimit(ConsensusLimit_full_post);
                videoLimit = reputationConsensus->GetConsensusLimit(ConsensusLimit_full_video);
                articleLimit = reputationConsensus->GetConsensusLimit(ConsensusLimit_full_article);
                complainLimit = reputationConsensus->GetConsensusLimit(ConsensusLimit_full_complain);
                commentLimit = reputationConsensus->GetConsensusLimit(ConsensusLimit_full_comment);
                scoreCommentLimit = reputationConsensus->GetConsensusLimit(ConsensusLimit_full_comment_score);
                scoreLimit = reputationConsensus->GetConsensusLimit(ConsensusLimit_full_score);
                break;
            case PocketConsensus::AccountMode_Pro:
                postLimit = reputationConsensus->GetConsensusLimit(ConsensusLimit_full_post);
                videoLimit = reputationConsensus->GetConsensusLimit(ConsensusLimit_pro_video);
                articleLimit = reputationConsensus->GetConsensusLimit(ConsensusLimit_full_article);
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

        if (!result["complain_spent"].isNull())
            result.pushKV("complain_unspent", complainLimit - result["complain_spent"].get_int());

        if (!result["comment_spent"].isNull())
            result.pushKV("comment_unspent", commentLimit - result["comment_spent"].get_int());

        if (!result["comment_score_spent"].isNull())
            result.pushKV("comment_score_unspent", scoreCommentLimit - result["comment_score_spent"].get_int());

        if (!result["score_spent"].isNull())
            result.pushKV("score_unspent", scoreLimit - result["score_spent"].get_int());

        if (!result["mod_flag_spent"].isNull())
            result.pushKV("mod_flag_unspent", reputationConsensus->GetConsensusLimit(ConsensusLimit_moderation_flag_count) - result["mod_flag_spent"].get_int());

        return result;
    }

    UniValue GetAccountUnspents(const JSONRPCRequest& request)
    {
        // TODO (brangr): add pagination

        if (request.fHelp)
            throw runtime_error(
                "txunspent ( minconf maxconf  [\"addresses\",...] [include_unsafe] [query_options])\n"
                "\nReturns array of unspent transaction outputs\n"
                "with between minconf and maxconf (inclusive) confirmations.\n"
                "Optionally filter to only include txouts paid to specified addresses.\n"
                "\nArguments:\n"
                "1. \"addresses\"      (string) A json array of pocketcoin addresses to filter\n"
                "    [\n"
                "      \"address\"     (string) pocketcoin address\n"
                "      ,...\n"
                "    ]\n"
                "2. minconf          (numeric, optional, default=1) The minimum confirmations to filter\n"
                "3. maxconf          (numeric, optional, default=9999999) The maximum confirmations to filter\n");

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

        // Get exclude inputs already used in mempool
        vector<pair<string, uint32_t>> mempoolInputs;
        mempool.GetAllInputs(mempoolInputs);

        // Get unspents from DB
        return request.DbConnection()->WebRpcRepoInst->GetUnspents(destinations, chainActive.Height(), mempoolInputs);
    }

    UniValue GetAccountSetting(const JSONRPCRequest& request)
    {
        if (request.fHelp)
            throw std::runtime_error(
                    "getaccountsetting \"address\"\n"
                    "\nReturn account settings object.\n");

        RPCTypeCheck(request.params, {UniValue::VSTR});

        string address = request.params[0].get_str();

        return request.DbConnection()->WebRpcRepoInst->GetAccountSetting(address);
    }

    UniValue GetAccountStatistic(const JSONRPCRequest& request)
    {
        if (request.fHelp)
            throw std::runtime_error(
                "getuserstatistic [\"addresses\", ...], height, depth\n"
                "\nGet user statistic.\n"
                "\nArguments:\n"
                "1. \"addresses\" (Array of strings) Addresses for statistic\n"
                "2. \"height\"  (int, optional) Maximum search height. Default is current chain height\n"
                "3. \"depth\" (int, optional) Depth of statistic. Default - whole history\n"
            );

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

        int nHeight = chainActive.Height();
        if (request.params.size() > 1) {
            if (request.params[1].isNum()) {
                if (request.params[1].get_int() > 0) {
                    nHeight = request.params[1].get_int();
                }
            }
        }

        int depth = chainActive.Height();
        if (request.params.size() > 2) {
            if (request.params[2].isNum()) {
                if (request.params[2].get_int() > 0) {
                    depth = request.params[2].get_int();
                }
            }
        }

        return request.DbConnection()->WebRpcRepoInst->GetUserStatistic(addresses, nHeight, depth);
    }

    UniValue GetAccountSubscribes(const JSONRPCRequest& request)
    {
        if (request.fHelp)
            throw std::runtime_error(
                "GetAccountSubscribes \"address\"\n"
                "\nReturn subscribes accounts list with pagination - NOT IMPLEMENTED\n");

        RPCTypeCheck(request.params, {UniValue::VSTR});

        string address = request.params[0].get_str();

        return request.DbConnection()->WebRpcRepoInst->GetSubscribesAddresses(address);
    }

    UniValue GetAccountSubscribers(const JSONRPCRequest& request)
    {
        if (request.fHelp)
            throw std::runtime_error(
                "GetAccountSubscribers \"address\"\n"
                "\nReturn subscribers accounts list with pagination - NOT IMPLEMENTED\n");

        RPCTypeCheck(request.params, {UniValue::VSTR});

        string address = request.params[0].get_str();

        return request.DbConnection()->WebRpcRepoInst->GetSubscribersAddresses(address);
    }

    UniValue GetAccountBlockings(const JSONRPCRequest& request)
    {
        if (request.fHelp)
            throw std::runtime_error(
                "GetAccountBlockings \"address\"\n"
                "\nReturn blocked accounts list with pagination - NOT IMPLEMENTED\n");

        RPCTypeCheck(request.params, {UniValue::VSTR});

        string address = request.params[0].get_str();

        return request.DbConnection()->WebRpcRepoInst->GetBlockingToAddresses(address);
    }

    UniValue GetTopAccounts(const JSONRPCRequest& request)
    {
        if (request.fHelp)
            throw std::runtime_error(
                "GetTopAccounts \"address\"\n"
                "\nReturn top accounts based on their content ratings\n");

        int topHeight = chainActive.Height();
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

        auto reputationConsensus = ReputationConsensusFactoryInst.Instance(chainActive.Height());
        auto badReputationLimit = reputationConsensus->GetConsensusLimit(ConsensusLimit_bad_reputation);

        UniValue result(UniValue::VARR);
        auto ids =  request.DbConnection()->WebRpcRepoInst->GetTopAccounts(topHeight, countOut, lang, tags, contentTypes,
            adrsExcluded, tagsExcluded, depth, badReputationLimit);
        if (!ids.empty())
        {
            auto profiles = request.DbConnection()->WebRpcRepoInst->GetAccountProfiles(ids, true);
            for (const auto[id, record] : profiles)
                result.push_back(record);
        }

        return result;
    }

}