// Copyright (c) 2018-2021 Pocketnet developers
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
        int64_t wssPort = gArgs.GetArg("-wsport", 8087);

        UniValue ports(UniValue::VOBJ);
        ports.pushKV("node", nodePort);
        ports.pushKV("api", publicPort);
        ports.pushKV("wss", wssPort);
        ports.pushKV("http", staticPort);
        ports.pushKV("https", staticPort);
        entry.pushKV("ports", ports);

        return entry;
    }

    UniValue AddTransaction(const JSONRPCRequest& request)
    {
        if (request.fHelp)
            throw std::runtime_error(
                "addtransaction\n"
                "\nAdd new pocketnet transaction.\n"
            );

        RPCTypeCheck(request.params, {UniValue::VSTR, UniValue::VOBJ, UniValue::VSTR});

        CMutableTransaction mTx;
        if (!DecodeHexTx(mTx, request.params[0].get_str()))
            throw JSONRPCError(RPC_DESERIALIZATION_ERROR, "TX decode failed");

        const auto tx = MakeTransactionRef(std::move(mTx));
        PocketTxType txType;
        if (!PocketHelpers::IsPocketTransaction(tx, txType))
            throw JSONRPCError(RPC_DESERIALIZATION_ERROR, "Bad pocketnet transaction type");

        string address;
        // TODO (brangr): get address from TxOuputs by TxHash and Number
        // if (!GetInputAddress(tx->vin[0].prevout.hash, tx->vin[0].prevout.n, address)) {
        //     throw JSONRPCError(RPC_INVALID_PARAMS, "Invalid address");
        // }

        auto ptx = PocketHelpers::CreateInstance(txType, tx->GetHash().GetHex(), tx->nTime);

        // TODO (brangr): implement deserialize pocket transaction
        // TODO (brangr): implement check & validate with consensus
        // TODO (brangr): implement insert into mempool
        
        return *ptx->GetHash();
    }

}