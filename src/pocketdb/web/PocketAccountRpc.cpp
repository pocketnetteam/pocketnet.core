// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include "pocketdb/web/PocketAccountRpc.h"

namespace PocketWeb::PocketWebRpc
{

    map<string, UniValue> GetUsersProfiles(const DbConnectionRef& dbCon, vector<string> addresses, bool shortForm, int option)
    {
        auto result = dbCon->WebRepoInst->GetUserProfile(addresses, shortForm, option);

        if (shortForm)
            return result;

        auto subscribes = dbCon->WebRepoInst->GetSubscribesAddresses(addresses);
        auto subscribers = dbCon->WebRepoInst->GetSubscribersAddresses(addresses);
        auto blocking = dbCon->WebRepoInst->GetBlockingToAddresses(addresses);

        for (auto & i : result)
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

    UniValue GetReputations(const JSONRPCRequest& request)
    {
        // TODO (team): implement
        return UniValue();
    }

    UniValue GetUserProfile(const JSONRPCRequest& request)
    {
        if (request.fHelp)
            throw runtime_error(
                "getuserprofile \"address\" ( shortForm )\n"
                "\nReturn Pocketnet user profile.\n");

        if (request.params.size() < 1)
            throw JSONRPCError(RPC_INVALID_PARAMETER, "There is no address");

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
        else
            throw JSONRPCError(RPC_INVALID_PARAMS, "Invalid inputs params");

        if (addresses.size() < 1)
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

        // TODO (team): check argument exists for 0 item in params

        string userName;
        if (!request.params[0].isNull())
        {
            RPCTypeCheckArgument(request.params[0], UniValue::VSTR);
            userName = request.params[0].get_str();
        }

        auto result = request.DbConnection()->WebRepoInst->GetUserAddress(userName);

        return result;
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

        // TODO (team): check argument exists for 0 item in params

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
                {
                    throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, string("Invalid Pocketcoin address: ") + input.get_str());
                }

                if (find(addresses.begin(), addresses.end(), input.get_str()) == addresses.end())
                {
                    addresses.push_back(input.get_str());
                }
            }
        }

        auto result = request.DbConnection()->WebRepoInst->GetAddressesRegistrationDates(addresses);

        return result;
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

        return request.DbConnection()->WebRepoInst->GetAccountState(address);
    }

}