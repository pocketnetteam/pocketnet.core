// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include "pocketdb/web/PocketContentRpc.h"

namespace PocketWeb::PocketWebRpc
{
    // std::map<std::string, UniValue> GetContentsData(const DbConnectionRef& dbCon, std::vector<std::string> txids)
    // {
    //     auto result = dbCon->WebRepoInst->GetContentsData(txids);
    //
    //     return result;
    // }
    //
    // UniValue GetContentsData(const JSONRPCRequest& request)
    // {
    //     if (request.fHelp)
    //         throw std::runtime_error(
    //             "GetContentsData\n"
    //             "\n.\n");
    //
    //     std::vector<std::string> txids;
    //     if (request.params.size() > 0)
    //     {
    //         if (request.params[0].isStr())
    //         {
    //             txids.emplace_back(request.params[0].get_str());
    //         }
    //         else if (request.params[0].isArray())
    //         {
    //             UniValue ids = request.params[0].get_array();
    //             for (unsigned int idx = 0; idx < ids.size(); idx++)
    //             {
    //                 if (ids[idx].isStr())
    //                 {
    //                     txids.emplace_back(ids[idx].get_str());
    //                 }
    //             }
    //         }
    //     }
    //
    //     std::map<std::string, UniValue> contentsdata = GetContentsData(request.DbConnection(), txids);
    //
    //     UniValue aResult(UniValue::VARR);
    //     for (auto& cd : contentsdata)
    //     {
    //         aResult.push_back(cd.second);
    //     }
    //     return aResult;
    // }

    UniValue GetHistoricalStrip(const JSONRPCRequest& request)
    {
        if (request.fHelp)
            throw std::runtime_error(
                "GetHistoricalStrip\n"
                "\n.\n");

        int nHeight = chainActive.Height();
        if (!request.params.empty())
        {
            if (request.params[0].isNum())
                if (request.params[0].get_int() > 0)
                    nHeight = request.params[0].get_int();
        }

        std::string start_txid;
        if (request.params.size() > 1)
        {
            start_txid = request.params[1].get_str();
        }

        int countOut = 10;
        if (request.params.size() > 2)
            if (request.params[2].isNum())
                countOut = request.params[2].get_int();

        std::string lang;
        if (request.params.size() > 3)
            lang = request.params[3].get_str();

        std::vector<string> tags;
        if (request.params.size() > 4)
        {
            if (request.params[4].isStr())
            {
                std::string tag = boost::trim_copy(request.params[4].get_str());
                if (!tag.empty())
                    tags.push_back(tag);
            }
            else if (request.params[4].isArray())
            {
                UniValue tgs = request.params[4].get_array();
                if (tgs.size() > 1000)
                    throw JSONRPCError(RPC_INVALID_PARAMS, "Too large array tags");

                if (!tgs.empty())
                {
                    for (unsigned int idx = 0; idx < tgs.size(); idx++)
                    {
                        std::string tag = boost::trim_copy(tgs[idx].get_str());
                        if (!tag.empty())
                            tags.push_back(tag);
                    }
                }
            }
        }

        std::vector<int> contentTypes;
        if (request.params.size() > 5)
        {
            if (request.params[5].isNum())
            {
                contentTypes.push_back(request.params[5].get_int());
            }
            else if (request.params[5].isStr())
            {
                auto type = TransactionHelper::ConvertOpReturnToType(request.params[5].get_str());
                if (TransactionHelper::IsIn(type, {CONTENT_POST, CONTENT_VIDEO}))
                    contentTypes.push_back(type);
            }
            else if (request.params[5].isArray())
            {
                UniValue cntntTps = request.params[5].get_array();
                if (cntntTps.size() > 10)
                    throw JSONRPCError(RPC_INVALID_PARAMS, "Too large array content types");

                if (!cntntTps.empty())
                {
                    for (unsigned int idx = 0; idx < cntntTps.size(); idx++)
                    {
                        if (cntntTps[idx].isNum())
                        {
                            contentTypes.push_back(cntntTps[idx].get_int());
                        }
                        else if (cntntTps[idx].isStr())
                        {
                            auto type = TransactionHelper::ConvertOpReturnToType(cntntTps[idx].get_str());
                            if (TransactionHelper::IsIn(type, {CONTENT_POST, CONTENT_VIDEO}))
                                contentTypes.push_back(type);
                        }
                    }
                }
            }
        }

        std::vector<string> txidsExcluded;
        if (request.params.size() > 6)
        {
            if (request.params[6].isStr())
            {
                txidsExcluded.push_back(request.params[6].get_str());
            }
            else if (request.params[6].isArray())
            {
                UniValue txids = request.params[6].get_array();
                if (txids.size() > 1000)
                    throw JSONRPCError(RPC_INVALID_PARAMS, "Too large array txids");

                if (!txids.empty())
                {
                    for (unsigned int idx = 0; idx < txids.size(); idx++)
                    {
                        std::string txidEx = boost::trim_copy(txids[idx].get_str());
                        if (!txidEx.empty())
                            txidsExcluded.push_back(txidEx);
                    }
                }
            }
        }

        std::vector<string> adrsExcluded;
        if (request.params.size() > 7)
        {
            if (request.params[7].isStr())
            {
                adrsExcluded.push_back(request.params[7].get_str());
            }
            else if (request.params[7].isArray())
            {
                UniValue adrs = request.params[7].get_array();
                if (adrs.size() > 1000)
                {
                    throw JSONRPCError(RPC_INVALID_PARAMS, "Too large array addresses");
                }
                if (!adrs.empty())
                {
                    for (unsigned int idx = 0; idx < adrs.size(); idx++)
                    {
                        std::string adrEx = boost::trim_copy(adrs[idx].get_str());
                        if (!adrEx.empty())
                        {
                            adrsExcluded.push_back(adrEx);
                        }
                    }
                }
            }
        }

        std::vector<string> tagsExcluded;
        if (request.params.size() > 8)
        {
            if (request.params[8].isStr())
            {
                tagsExcluded.push_back(request.params[8].get_str());
            }
            else if (request.params[8].isArray())
            {
                UniValue tagsEx = request.params[8].get_array();
                if (tagsEx.size() > 1000)
                {
                    throw JSONRPCError(RPC_INVALID_PARAMS, "Too large array tags");
                }
                if (!tagsEx.empty())
                {
                    for (unsigned int idx = 0; idx < tagsEx.size(); idx++)
                    {
                        std::string tgsEx = boost::trim_copy(tagsEx[idx].get_str());
                        if (!tgsEx.empty())
                        {
                            tagsExcluded.push_back(tgsEx);
                        }
                    }
                }
            }
        }

        std::string address;
        if (request.params.size() > 9)
        {
            RPCTypeCheckArgument(request.params[9], UniValue::VSTR);
            address = request.params[9].get_str();
            CTxDestination dest = DecodeDestination(address);

            if (!IsValidDestination(dest))
                throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, std::string("Invalid Pocketcoin address: ") + address);
        }

        UniValue oResult(UniValue::VOBJ);
        UniValue aContents(UniValue::VARR);

        int totalcount = 0;
        map<string, UniValue> contents = request.DbConnection()->WebRepoInst->GetContents(nHeight, start_txid, countOut, lang, tags, contentTypes,
            txidsExcluded, adrsExcluded, tagsExcluded, address);
        for (auto& c : contents)
        {
            aContents.push_back(c.second);
        }

        oResult.pushKV("height", nHeight);
        oResult.pushKV("contents", aContents);
        oResult.pushKV("contentsTotal", totalcount);
        return oResult;
    }

    UniValue GetHierarchicalStrip(const JSONRPCRequest& request)
    {
        if (request.fHelp)
            throw std::runtime_error(
                "GetHierarchicalStrip\n"
                "\n.\n");

        return GetHistoricalStrip(request);
    }
}