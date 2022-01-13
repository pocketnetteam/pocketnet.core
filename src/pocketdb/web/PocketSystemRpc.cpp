// Copyright (c) 2018-2022 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include "pocketdb/web/PocketSystemRpc.h"

namespace PocketWeb::PocketWebRpc
{

    UniValue GetTime(const JSONRPCRequest& request)
    {
        if (request.fHelp)
            throw std::runtime_error(
                "gettime\n"
                "\nReturn node time.\n");

        UniValue entry(UniValue::VOBJ);
        entry.pushKV("time", GetAdjustedTime());

        return entry;
    }

    UniValue GetPeerInfo(const JSONRPCRequest& request)
    {
        if (request.fHelp)
            throw std::runtime_error(
                "getpeerinfo\n"
                "\nReturns data about each connected network node as a json array of objects.\n"
            );

        UniValue ret(UniValue::VARR);

        std::vector<CNodeStats> vstats;
        g_connman->GetNodeStats(vstats);
        for (const CNodeStats& stats : vstats) {
            UniValue obj(UniValue::VOBJ);

            obj.pushKV("addr", stats.addrName);
            obj.pushKV("services", strprintf("%016x", stats.nServices));
            obj.pushKV("relaytxes", stats.fRelayTxes);
            obj.pushKV("lastsend", stats.nLastSend);
            obj.pushKV("lastrecv", stats.nLastRecv);
            obj.pushKV("conntime", stats.nTimeConnected);
            obj.pushKV("timeoffset", stats.nTimeOffset);
            obj.pushKV("pingtime", stats.dPingTime);
            obj.pushKV("protocol", stats.nVersion);
            obj.pushKV("version", stats.cleanSubVer);
            obj.pushKV("inbound", stats.fInbound);
            obj.pushKV("addnode", stats.m_manual_connection);
            obj.pushKV("startingheight", stats.nStartingHeight);
            obj.pushKV("whitelisted", stats.fWhitelisted);

            // Mutex guarded node statistic
            CNodeStateStats nodeState;
            if (GetNodeStateStats(stats.nodeid, nodeState))
            {
                obj.pushKV("banscore", nodeState.nMisbehavior);
                obj.pushKV("synced_headers", nodeState.nSyncHeight);
                obj.pushKV("synced_blocks", nodeState.nCommonHeight);
            }

            ret.push_back(obj);
        }

        return ret;
    }

    UniValue GetNodeInfo(const JSONRPCRequest& request)
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

        CBlockIndex* pindex = chainActive.Tip();
        UniValue oblock(UniValue::VOBJ);
        oblock.pushKV("height", pindex->nHeight);
        oblock.pushKV("hash", pindex->GetBlockHash().GetHex());
        oblock.pushKV("time", (int64_t)pindex->nTime);
        oblock.pushKV("ntx", (int)pindex->nTx);
        entry.pushKV("lastblock", oblock);

        UniValue proxies(UniValue::VARR);
        if (!WSConnections.empty()) {
            boost::lock_guard<boost::mutex> guard(WSMutex);
            for (auto& it : WSConnections) {
                if (it.second.Service) {
                    UniValue proxy(UniValue::VOBJ);
                    proxy.pushKV("address", it.second.Address);
                    proxy.pushKV("ip", it.second.Ip);
                    proxy.pushKV("port", it.second.MainPort);
                    proxy.pushKV("portWss", it.second.WssPort);
                    proxies.push_back(proxy);
                }
            }
        }
        entry.pushKV("proxies", proxies);

        // Ports information
        int64_t nodePort = gArgs.GetArg("-port", Params().GetDefaultPort());
        int64_t publicPort = gArgs.GetArg("-publicrpcport", BaseParams().PublicRPCPort());
        int64_t staticPort = gArgs.GetArg("-staticrpcport", BaseParams().StaticRPCPort());
        int64_t restPort = gArgs.GetArg("-restport", BaseParams().RestPort());
        int64_t wssPort = gArgs.GetArg("-wsport", 8087);

        UniValue ports(UniValue::VOBJ);
        ports.pushKV("node", nodePort);
        ports.pushKV("api", publicPort);
        ports.pushKV("rest", restPort);
        ports.pushKV("wss", wssPort);
        ports.pushKV("http", staticPort);
        ports.pushKV("https", staticPort);
        entry.pushKV("ports", ports);

        return entry;
    }
    
    UniValue GetCoinInfo(const JSONRPCRequest& request)
    {
        if (request.fHelp)
            throw std::runtime_error(
                "getcoininfo height\n"
                "\n Returns current Pocketcoin emission for any given block\n"
                "\nArguments:\n"
                "1. height (integer optional) to calculate emission as of that block number\n"
                "   if arguments are empty or non-numeric, then returns as of the current block number");

        int height = chainActive.Height();
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
    }

}