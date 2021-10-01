// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include "pocketdb/web/PocketAccountRpc.h"

namespace PocketWeb::PocketWebRpc
{
    UniValue GetAddressId(const JSONRPCRequest& request)
    {
        if (request.fHelp)
            throw runtime_error(
                "getaddressid \"address\" or \"id\"\n"
                "\nGet id and address.\n");

        if (request.params.empty())
            throw JSONRPCError(RPC_INVALID_PARAMETER, "There is no arguments");

        if (request.params[0].isNum())
            return request.DbConnection()->WebRepoInst->GetAddressId(request.params[0].get_int64());
        else if (request.params[0].isStr())
            return request.DbConnection()->WebRepoInst->GetAddressId(request.params[0].get_str());

        throw JSONRPCError(RPC_INVALID_PARAMETER, "There is no arguments");
    }

    map<string, UniValue> GetUsersProfiles(const DbConnectionRef& dbCon, vector<string> addresses, bool shortForm, int option)
    {
        auto result = dbCon->WebRepoInst->GetUserProfile(addresses, shortForm, option);

        if (shortForm)
            return result;

        auto subscribes = dbCon->WebRepoInst->GetSubscribesAddresses(addresses);
        auto subscribers = dbCon->WebRepoInst->GetSubscribersAddresses(addresses);
        auto blocking = dbCon->WebRepoInst->GetBlockingToAddresses(addresses);

        for (auto& i : result)
        {
            if (subscribes.find(i.first) != subscribes.end())
                i.second.pushKV("subscribes", subscribes[i.first]);

            if (subscribers.find(i.first) != subscribers.end())
                i.second.pushKV("subscribers", subscribers[i.first]);

            if (blocking.find(i.first) != blocking.end())
                i.second.pushKV("blocking", blocking[i.first]);
        }

        return result;
    }

    UniValue GetUserProfile(const JSONRPCRequest& request)
    {
        if (request.fHelp)
            throw runtime_error(
                "getuserprofile \"address\" ( shortForm )\n"
                "\nReturn Pocketnet user profile.\n");

        if (request.params.empty())
            throw JSONRPCError(RPC_INVALID_PARAMETER, "There is no arguments");

        vector<string> addresses;
        if (request.params[0].isStr())
            addresses.push_back(request.params[0].get_str());
        else if (request.params[0].isArray())
        {
            UniValue addr = request.params[0].get_array();
            for (unsigned int idx = 0; idx < addr.size(); idx++)
            {
                addresses.push_back(addr[idx].get_str());
            }
        }

        if (addresses.empty())
            throw JSONRPCError(RPC_INVALID_PARAMETER, "There is no address");

        // Short profile form is: address, b, i, name
        bool shortForm = false;
        if (request.params.size() > 1)
            shortForm = request.params[1].get_str() == "1";

        UniValue aResult(UniValue::VARR);

        map<string, UniValue> profiles = GetUsersProfiles(request.DbConnection(), addresses, shortForm);
        for (auto& p : profiles)
            aResult.push_back(p.second);

        return aResult;
    }

    UniValue GetUserAddress(const JSONRPCRequest& request)
    {
        if (request.fHelp)
            throw runtime_error(
                "getuseraddress \"user_name\" ( count )\n"
                "\nGet list addresses of user.\n");

        RPCTypeCheck(request.params, {UniValue::VSTR});

        string userName = request.params[0].get_str();
        return request.DbConnection()->WebRepoInst->GetUserAddress(userName);
    }

    UniValue GetAddressRegistration(const JSONRPCRequest& request)
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

        return request.DbConnection()->WebRepoInst->GetAddressesRegistrationDates(addresses);
    }

    UniValue GetUserState(const JSONRPCRequest& request)
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
        auto result = request.DbConnection()->WebRepoInst->GetAccountState(address, chainActive.Height() - windowDepth);
        if (result["address"].isNull())
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, string("Pocketcoin address not found"));

        // Calculate additional fields
        auto accountMode = reputationConsensus->GetAccountMode(result["reputation"].get_int(), result["balance"].get_int64());

        result.pushKV("mode", accountMode);
        result.pushKV("trial", accountMode == AccountMode_Trial);

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
    }

    UniValue GetUnspents(const JSONRPCRequest& request)
    {
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

        // Check exists TX in mempool
        // // T_O_D_O: LOCK(mempool.cs);
        // for (const auto& e : mempool.mapTx) {
        //     const CTransaction& tx = e.GetTx();
        //     for (const CTxIn& txin : tx.vin) {
        //         AddressUnspentTransactionItem mempoolItm = {"", txin.prevout.hash.ToString(), (int)txin.prevout.n};

        //         if (find_if(
        //             unspentTransactions.begin(),
        //             unspentTransactions.end(),
        //             [&](const AddressUnspentTransactionItem& itm) { return itm == mempoolItm; }) != unspentTransactions.end()) {
        //             unspentTransactions.erase(
        //                 remove(unspentTransactions.begin(), unspentTransactions.end(), mempoolItm),
        //                 unspentTransactions.end());
        //         }
        //     }
        // }

        return request.DbConnection()->WebRepoInst->GetUnspents(destinations, chainActive.Height());
    }

}