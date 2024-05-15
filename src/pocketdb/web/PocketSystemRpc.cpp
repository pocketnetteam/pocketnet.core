// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include "pocketdb/web/PocketSystemRpc.h"
#include "rpc/blockchain.h"
#include "rpc/util.h"
#include "init.h"

namespace PocketWeb::PocketWebRpc
{

    RPCHelpMan GetTime()
    {
        return RPCHelpMan{"gettime",
                "\nReturn node time.\n",
                {},
                RPCResult{
                    RPCResult::Type::OBJ, "", "", 
                    {
                        {RPCResult::Type::NUM_TIME, "time", ""},
                    },
                },
                RPCExamples{
                    HelpExampleCli("gettime", "") +
                    HelpExampleRpc("gettime", "")
                },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
    {
        UniValue entry(UniValue::VOBJ);
        entry.pushKV("time", GetAdjustedTime());

        return entry;
    },
        };
    }

    RPCHelpMan GetPeerInfo()
    {
        return RPCHelpMan{"getpeerinfo",
                "\nReturns data about each connected network node as a json array of objects..\n",
                {},
                RPCResult{
                    RPCResult::Type::ARR, "", "",
                    {
                        {
                            RPCResult::Type::OBJ, "", "", 
                            {
                                {RPCResult::Type::STR, "addr", ""},
                                {RPCResult::Type::STR, "services", ""},
                                {RPCResult::Type::BOOL, "relaytxes", ""},
                                {RPCResult::Type::NUM, "lastsend", ""},
                                {RPCResult::Type::NUM, "lastrecv", ""},
                                {RPCResult::Type::NUM, "conntime", ""},
                                {RPCResult::Type::NUM, "timeoffset", ""},
                                {RPCResult::Type::NUM, "protocol", ""},
                                {RPCResult::Type::STR, "version", ""},
                                {RPCResult::Type::BOOL, "inbound", ""},
                                {RPCResult::Type::BOOL, "addnode", ""},
                                {RPCResult::Type::NUM, "startingheight", ""},
                                {RPCResult::Type::BOOL, "whitelisted", ""},
                            },
                        },
                    },
                },
                RPCExamples{
                    HelpExampleCli("getpeerinfo", "") +
                    HelpExampleRpc("getpeerinfo", "")
                },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
    {
        const auto& node = EnsureNodeContext(request.context);
        UniValue ret(UniValue::VARR);

        std::vector<CNodeStats> vstats;
        node.connman->GetNodeStats(vstats);
        for (const CNodeStats& stats : vstats) {
            UniValue obj(UniValue::VOBJ);

            obj.pushKV("addr", stats.addrName);
            obj.pushKV("services", strprintf("%016x", stats.nServices));
            obj.pushKV("relaytxes", stats.fRelayTxes);
            obj.pushKV("lastsend", stats.nLastSend);
            obj.pushKV("lastrecv", stats.nLastRecv);
            obj.pushKV("conntime", count_seconds(stats.m_connected));
            obj.pushKV("timeoffset", stats.nTimeOffset);
            obj.pushKV("pingtime", stats.m_ping_usec); // TODO (losty-fur): ping changed from double to usec
            obj.pushKV("protocol", stats.nVersion);
            obj.pushKV("version", stats.cleanSubVer);
            obj.pushKV("inbound", stats.fInbound);
            obj.pushKV("addnode", stats.m_manual_connection);
            obj.pushKV("startingheight", stats.nStartingHeight);
            obj.pushKV("whitelisted", stats.m_legacyWhitelisted); // TODO (losty-fur): probably remove this

            CNodeStateStats nodeState;
            if (GetNodeStateStatsView(stats.nodeid, nodeState))
            {
                obj.pushKV("banscore", nodeState.m_misbehavior_score);
                obj.pushKV("synced_headers", nodeState.nSyncHeight);
                obj.pushKV("synced_blocks", nodeState.nCommonHeight);
            }

            ret.push_back(obj);
        }

        return ret;
    },
        };
    }

    RPCHelpMan GetNodeInfo()
    {
        return RPCHelpMan{"getnodeinfo",
                "\nReturns data about node.\n",
                {},
                RPCResult{
                    RPCResult::Type::OBJ, "", "", 
                    {
                        {RPCResult::Type::STR, "version", ""},
                        {RPCResult::Type::NUM_TIME, "time", ""},
                        {RPCResult::Type::STR, "chain", ""},
                        {RPCResult::Type::BOOL, "proxy", ""},
                        {RPCResult::Type::NUM, "netstakeweight", ""},
                        {
                            RPCResult::Type::OBJ, "lastblock", "",
                            {
                                {RPCResult::Type::NUM, "height", ""},
                                {RPCResult::Type::STR, "hash", ""},
                                {RPCResult::Type::NUM_TIME, "time", ""},
                                {RPCResult::Type::STR, "ntx", ""},
                            }
                        },
                        {
                            RPCResult::Type::ARR, "", "",
                            {
                                {
                                    RPCResult::Type::OBJ, "", "",
                                    {
                                        {RPCResult::Type::STR, "address", ""},
                                        {RPCResult::Type::STR, "ip", ""},
                                        {RPCResult::Type::NUM, "port", ""},
                                        {RPCResult::Type::NUM, "portWss", ""},
                                    }
                                }
                            }
                        },
                        {
                            RPCResult::Type::OBJ, "ports", "",
                            {
                                {RPCResult::Type::NUM, "node", ""},
                                {RPCResult::Type::NUM, "api", ""},
                                {RPCResult::Type::NUM, "rest", ""},
                                {RPCResult::Type::NUM, "wss", ""},
                                {RPCResult::Type::NUM, "http", ""},
                                {RPCResult::Type::NUM, "https", ""},
                            }
                        }
                    },
                },
                RPCExamples{
                    HelpExampleCli("getnodeinfo", "") +
                    HelpExampleRpc("getnodeinfo", "")
                },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
    {
        if (request.fHelp)
            throw std::runtime_error(
                "getnodeinfo\n"
                "\nReturns data about node.\n"
            );

        UniValue entry(UniValue::VOBJ);
        entry.pushKV("version", FormatVersion(CLIENT_VERSION));
        entry.pushKV("time", GetAdjustedTime());
        entry.pushKV("chain", Params().NetworkIDString());
        entry.pushKV("proxy", true);

        uint64_t nNetworkWeight = GetPoSKernelPS();
        entry.pushKV("netstakeweight", (uint64_t)nNetworkWeight);

        CBlockIndex* pindex = ChainActive().Tip();
        UniValue oblock(UniValue::VOBJ);
        oblock.pushKV("height", pindex->nHeight);
        oblock.pushKV("hash", pindex->GetBlockHash().GetHex());
        oblock.pushKV("time", (int64_t)pindex->nTime);
        oblock.pushKV("ntx", (int)pindex->nTx);
        entry.pushKV("lastblock", oblock);

        UniValue proxies(UniValue::VARR);
        if (WSConnections) {
            auto fillProxy = [&proxies](const std::pair<const std::string, WSUser>& it) {
                if (it.second.Service) {
                    UniValue proxy(UniValue::VOBJ);
                    proxy.pushKV("address", it.second.Address);
                    proxy.pushKV("ip", it.second.Ip);
                    proxy.pushKV("port", it.second.MainPort);
                    proxy.pushKV("portWss", it.second.WssPort);
                    proxies.push_back(proxy);
                }
            };
            WSConnections->Iterate(fillProxy);
        }
        entry.pushKV("proxies", proxies);

        // Ports information
        int64_t nodePort = gArgs.GetArg("-port", Params().GetDefaultPort());
        int64_t publicPort = gArgs.GetArg("-publicrpcport", BaseParams().PublicRPCPort());
        int64_t staticPort = gArgs.GetArg("-staticrpcport", BaseParams().StaticRPCPort());
        int64_t restPort = gArgs.GetArg("-restport", BaseParams().RestPort());
        int64_t wssPort = gArgs.GetArg("-wsport", BaseParams().WsPort());

        UniValue ports(UniValue::VOBJ);
        ports.pushKV("node", nodePort);
        ports.pushKV("api", publicPort);
        ports.pushKV("rest", restPort);
        ports.pushKV("wss", wssPort);
        ports.pushKV("http", staticPort);
        ports.pushKV("https", staticPort);
        entry.pushKV("ports", ports);

        return entry;
    },
        };
    }
    
    RPCHelpMan GetCoinInfo()
    {
        return RPCHelpMan{"getcoininfo",
                "\nReturns current Pocketcoin emission for any given block\n",
                {
                    {"height", RPCArg::Type::NUM, RPCArg::Optional::OMITTED_NAMED_ARG, "height (integer optional) to calculate emission as of that block number. If arguments are empty or non-numeric, then returns as of the current block number"}
                },
                RPCResult{
                    RPCResult::Type::OBJ, "", "", 
                    {
                        {RPCResult::Type::STR_AMOUNT, "emission", ""},
                        {RPCResult::Type::NUM, "height", ""},
                    },
                },
                RPCExamples{
                    HelpExampleCli("getcoininfo", "") +
                    HelpExampleRpc("getcoininfo", "") +
                    HelpExampleCli("getcoininfo", "1231") +
                    HelpExampleRpc("getcoininfo", "1231")
                },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
        {
            int height = ChainActiveSafeHeight();
            if (request.params.size() > 0 && request.params[0].isNum())
                height = request.params[0].get_int();

            int first75 = 3750000;
            int halvblocks = 2'100'000;
            double emission = 0;
            int nratio = height / halvblocks;
            double mult;

            for (int i = 0; i <= nratio; ++i) {
                mult = 5. / pow(2., static_cast<double>(i));
                if (i < nratio || nratio == 0) {
                    if (i == 0 && height < 75'000)
                        emission += height * 50;
                    else if (i == 0) {
                        emission += first75 + (std::min(height, halvblocks) - 75000) * 5;
                    } else {
                        emission += 2'100'000 * mult;
                    }
                }

                if (i == nratio && nratio != 0) {
                    emission += (height % halvblocks) * mult;
                }
            }

            UniValue result(UniValue::VOBJ);
            result.pushKV("emission", emission);
            result.pushKV("height", height);
            
            return result;
        },
        };
    }

    RPCHelpMan GetLatestStat()
    {
        return RPCHelpMan{"getlateststat",
                "\nReturns last page of statistic\n",
                { },
                RPCResult{
                    RPCResult::Type::OBJ, "", "", 
                    {
                        {RPCResult::Type::NUM_TIME, "time", ""},
                    },
                },
                RPCExamples{
                    HelpExampleCli("getlateststat", "")
                },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
        {
            return gStatEngineInstance.LatestPage();
        },
        };
    }
    
    RPCHelpMan GetPosDifficulty()
    {
        return RPCHelpMan{
            "getposdifficulty",
            "\nReturns proof of stake difficulty for selected height\n",
            {
                {"height", RPCArg::Type::NUM, RPCArg::Optional::NO, "Heihgt (Default: current height)"},
            },
            RPCResult{
                RPCResult::Type::OBJ, "", "", {
                    { RPCResult::Type::NUM, "Height", ""},
                    { RPCResult::Type::NUM, "Difficulty", ""}
                }
            },
            RPCExamples{
                HelpExampleCli("getposdifficulty", "")
            },
            [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
            {
                int64_t height = ChainActiveSafeHeight();
                if (request.params.size() > 0 && request.params[0].isNum()) {
                    height = request.params[0].get_int();
                }
                
                auto blockIndex = ChainActive().Tip();
                if (blockIndex->nHeight > height)
                    blockIndex = ChainActive()[height];

                UniValue result(UniValue::VOBJ);
                result.pushKV("Height", blockIndex->nHeight);
                result.pushKV("Difficulty", GetPosDifficulty(blockIndex));

                return result;
            }
        };
    }

}