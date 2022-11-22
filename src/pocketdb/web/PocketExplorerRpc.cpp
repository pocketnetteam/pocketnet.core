// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include "pocketdb/web/PocketExplorerRpc.h"

namespace PocketWeb::PocketWebRpc
{
    map<string, UniValue> TransactionsStatisticCache;

    UniValue GetStatisticTransactions(const JSONRPCRequest& request)
    {
        if (request.fHelp)
            throw runtime_error(
                "getstatistictransactions (topTime, depth, period)\n"
                "\nGet statistics\n"
                "\nArguments:\n"
                "1. \"topTime\"     (int64, optional) Top time in unixtime format (Default: current adjusted time)\n"
                "2. \"depth\"       (int32, optional) Depth in hours (Default: 24)\n"
                "2. \"period\"      (int32, optional) Grouping time in seconds (Default: 3600)\n");

        int64_t topTime = GetAdjustedTime();
        if (request.params[0].isNum())
            topTime = min(request.params[0].get_int64(), topTime);

        int depth = 24;
        if (request.params[1].isNum())
            depth = request.params[1].get_int();

        int period = 3600;
        if (request.params[2].isNum())
            period = request.params[2].get_int();

        return request.DbConnection()->ExplorerRepoInst->GetTransactionsStatistic(topTime, depth, period);
    }

    UniValue GetStatisticByHours(const JSONRPCRequest& request)
    {
        if (request.fHelp)
            throw runtime_error(
                "getstatisticbyhours (topHeight, depth)\n"
                "\nGet statistics\n"
                "\nArguments:\n"
                "1. \"topheight\"   (int32, optional) Top height (Default: chain height)\n"
                "2. \"depth\"       (int32, optional) Depth hours (Maximum: 24 hours)\n");

        int topHeight = chainActive.Height() / 10 * 10;
        if (request.params[0].isNum())
            topHeight = min(request.params[0].get_int(), topHeight);

        int depth = 24;
        if (request.params[1].isNum())
            depth = min(request.params[1].get_int(), depth);
        depth = depth * 60;

        return request.DbConnection()->ExplorerRepoInst->GetTransactionsStatisticByHours(topHeight, depth);
    }

    UniValue GetStatisticByDays(const JSONRPCRequest& request)
    {
        if (request.fHelp)
            throw runtime_error(
                "getstatisticbydays (topHeight, depth)\n"
                "\nGet statistics\n"
                "\nArguments:\n"
                "1. \"topheight\"   (int32, optional) Top height (Default: chain height)\n"
                "2. \"depth\"       (int32, optional) Depth days (Maximum: 30 days)\n");

        int topHeight = chainActive.Height() / 10 * 10;
        if (request.params[0].isNum())
            topHeight = min(request.params[0].get_int(), topHeight);

        int depth = 30;
        if (request.params[1].isNum())
            depth = min(request.params[1].get_int(), depth);
        depth = depth * 24 * 60;

        return request.DbConnection()->ExplorerRepoInst->GetTransactionsStatisticByDays(topHeight, depth);
    }

    UniValue GetStatisticContentByHours(const JSONRPCRequest& request)
    {
        if (request.fHelp)
            throw runtime_error(
                "getstatisticcontentbyhours\n"
                "\nGet statistics for content transactions grouped by hours\n"
            );

        int topHeight = chainActive.Height() / 10 * 10;
        if (request.params[0].isNum())
            topHeight = min(request.params[0].get_int(), topHeight);

        int depth = 24;
        if (request.params[1].isNum())
            depth = min(request.params[1].get_int(), depth);
        depth = depth * 60;

        return request.DbConnection()->ExplorerRepoInst->GetContentStatisticByHours(topHeight, depth);
    }

    UniValue GetStatisticContentByDays(const JSONRPCRequest& request)
    {
        if (request.fHelp)
            throw runtime_error(
                "getstatisticcontentbydays\n"
                "\nGet statistics for content transactions grouped by days\n"
            );

        int topHeight = chainActive.Height() / 10 * 10;
        if (request.params[0].isNum())
            topHeight = min(request.params[0].get_int(), topHeight);

        int depth = 30;
        if (request.params[1].isNum())
            depth = min(request.params[1].get_int(), depth);
        depth = depth * 24 * 60;

        return request.DbConnection()->ExplorerRepoInst->GetContentStatisticByDays(topHeight, depth);
    }

    UniValue GetLastBlocks(const JSONRPCRequest& request)
    {
        if (request.fHelp)
            throw runtime_error(
                "getlastblocks ( count, last_height, verbosity )\n"
                "\nGet N last blocks.\n"
                "\nArguments:\n"
                "1. \"count\"         (int, optional) Count of blocks. Maximum 100 blocks.\n"
                "2. \"last_height\"   (int, optional) Height of last block, including.\n"
                "3. \"verbosity\"     (int, optional) Verbosity output.\n");

        int count = 10;
        if (!request.params.empty() && request.params[0].isNum())
        {
            count = request.params[0].get_int();
            if (count > 100) count = 100;
        }

        int last_height = chainActive.Height();
        if (request.params.size() > 1 && request.params[1].isNum())
        {
            last_height = request.params[1].get_int();
            if (last_height < 0) last_height = chainActive.Height();
        }

        bool verbose = false;
        if (request.params.size() > 2)
        {
            RPCTypeCheckArgument(request.params[2], UniValue::VBOOL);
            verbose = request.params[2].get_bool();
        }

        // Collect general block information
        CBlockIndex* pindex = chainActive[last_height];
        int i = count;
        map<int, UniValue> blocks;
        while (pindex && i-- > 0)
        {
            UniValue oblock(UniValue::VOBJ);
            oblock.pushKV("height", pindex->nHeight);
            oblock.pushKV("hash", pindex->GetBlockHash().GetHex());
            oblock.pushKV("time", (int64_t) pindex->nTime);
            oblock.pushKV("ntx", (int) pindex->nTx - 1);

            blocks.emplace(pindex->nHeight, oblock);
            pindex = pindex->pprev;
        }

        // Extend with transaction statistic data
        if (verbose)
        {
            auto data = request.DbConnection()->ExplorerRepoInst->GetBlocksStatistic(last_height - count, last_height);

            for (auto& s : data)
            {
                if (blocks.find(s.first) == blocks.end())
                    continue;

                if (blocks[s.first].At("types").isNull())
                    blocks[s.first].pushKV("types", UniValue(UniValue::VOBJ));

                for (auto& d : s.second)
                    blocks[s.first].At("types").pushKV(to_string(d.first), d.second);
            }
        }

        UniValue result(UniValue::VARR);
        for (auto& block : blocks)
            result.push_back(block.second);

        return result;
    }

    UniValue GetCompactBlock(const JSONRPCRequest& request)
    {
        if (request.fHelp)
            throw runtime_error(
                "getcompactblock \"blockhash\" or \"blocknumber\" \n"
                "\nArguments:\n"
                "1. \"blockhash\"          (string, optional) The block by hash\n"
                "2. \"blocknumber\"        (number, optional) The block by number\n");

        if (request.params.empty())
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Missing parameters");

        string blockHash;
        if (request.params[0].isStr())
            blockHash = request.params[0].get_str();

        int blockNumber = -1;
        if (request.params[1].isNum())
            blockNumber = request.params[1].get_int();

        const CBlockIndex* pindex = nullptr;

        if (!blockHash.empty())
            pindex = LookupBlockIndexWithoutLock(uint256S(blockHash));

        if (blockNumber >= 0 && blockNumber <= chainActive.Height())
            pindex = chainActive[blockNumber];

        if (!pindex)
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Block not found");

        UniValue result(UniValue::VOBJ);

        result.pushKV("height", pindex->nHeight);
        result.pushKV("hash", pindex->GetBlockHash().GetHex());
        result.pushKV("time", (int64_t) pindex->nTime);
        result.pushKV("nTx", (int64_t) pindex->nTx);
        result.pushKV("difficulty", GetDifficulty(pindex));
        result.pushKV("merkleroot", pindex->hashMerkleRoot.GetHex());
        result.pushKV("bits", strprintf("%08x", pindex->nBits));

        if (pindex->pprev)
            result.pushKV("prevhash", pindex->pprev->GetBlockHash().GetHex());

        auto pnext = chainActive.Next(pindex);
        if (pnext)
            result.pushKV("nexthash", pnext->GetBlockHash().GetHex());

        return result;
    }

    UniValue GetAddressInfo(const JSONRPCRequest& request)
    {
        if (request.fHelp)
            throw runtime_error(
                "getaddressinfo \"address\"\n"
                "\nGet address summary information\n"
                "\nArguments:\n"
                "1. \"address\"    (string) Address\n");

        string address;
        if (request.params.empty() || !request.params[0].isStr())
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid address argument");

        auto dest = DecodeDestination(request.params[0].get_str());
        if (!IsValidDestination(dest))
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY,
                string("Invalid address: ") + request.params[0].get_str());
        address = request.params[0].get_str();

        UniValue addressInfo(UniValue::VOBJ);
        addressInfo.pushKV("lastChange", 0);
        addressInfo.pushKV("balance", 0);

        auto info = request.DbConnection()->ExplorerRepoInst->GetAddressesInfo({ address });
        if (info.find(address) != info.end())
        {
            auto[height, balance] = info[address];
            addressInfo.pushKV("lastChange", height);
            addressInfo.pushKV("balance", balance / 100000000.0);
        }

        return addressInfo;
    }

    UniValue GetBalanceHistory(const JSONRPCRequest& request)
    {
        if (request.fHelp)
            throw runtime_error(
                "getbalancehistory [\"address\", ...] topHeight count\n"
                "\nGet balance changes history for addresses\n"
                "\nArguments:\n"
                "1. addresses     (array of strings) Addresses for calculate total balance\n"
                "2. topHeight     (int32) Top block height (Inclusive)\n"
                "3. count         (int32) Count of records\n\n"
                "Return:\n"
                "[ [height, amount], [1000, 500], [999,495], ... ]");

        vector<string> addresses;
        if (!request.params[0].isArray() && !request.params[0].isStr())
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid address argument");

        if (request.params[0].isStr())
        {
            auto dest = DecodeDestination(request.params[0].get_str());
            if (!IsValidDestination(dest))
                throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY,
                    string("Invalid address: ") + request.params[0].get_str());

            addresses.push_back(request.params[0].get_str());
        }

        if (request.params[0].isArray())
        {
            UniValue addrs = request.params[0].get_array();
            for (unsigned int idx = 0; idx < addrs.size(); idx++)
            {
                auto addr = addrs[idx].get_str();
                auto dest = DecodeDestination(addr);
                if (!IsValidDestination(dest))
                    throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY,
                        string("Invalid address: ") + addr);

                addresses.push_back(addr);

                if (addresses.size() > 100)
                    break;
            }
        }

        int topHeight = chainActive.Height();
        if (request.params[1].isNum())
            topHeight = request.params[1].get_int();

        int count = 10;
        if (request.params[2].isNum())
            count = min(25, request.params[2].get_int());

        return request.DbConnection()->ExplorerRepoInst->GetBalanceHistory(addresses, topHeight, count);
    }

    UniValue SearchByHash(const JSONRPCRequest& request)
    {
        if (request.fHelp)
            throw runtime_error(
                "checkstringtype \"string\"\n"
                "\nCheck type of input string - address, block or tx id.\n"
                "\nArguments:\n"
                "1. \"string\"   (string) Input string\n");

        if (request.params.empty())
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Missing parameter");

        string value = request.params[0].get_str();

        UniValue result(UniValue::VOBJ);

        if (value.size() == 34)
        {
            if (IsValidDestination(DecodeDestination(value)))
            {
                result.pushKV("type", "address");
                return result;
            }
        }
        else if (value.size() == 64)
        {
            const CBlockIndex* pblockindex = LookupBlockIndexWithoutLock(uint256S(value));
            if (pblockindex)
            {
                result.pushKV("type", "block");
                return result;
            }

            result.pushKV("type", "transaction");
            return result;
        }

        result.pushKV("type", "notfound");
        return result;
    }

    UniValue GetAddressTransactions(const JSONRPCRequest& request)
    {
        if (request.fHelp)
        {
            throw runtime_error(
                "getaddresstransactions [address, pageInitBlock, pageStart, pageSize]\n"
                "\nGet transactions info.\n"
                "\nArguments:\n"
                "1. \"address\"       (string, required) Address hash\n"
                "2. \"pageInitBlock\" (number) Max block height for filter pagination window\n"
                "3. \"pageStart\"     (number) Row number for start page\n"
                "4. \"pageSize\"      (number) Page size\n"
            );
        }

        if (request.params.empty() || !request.params[0].isStr())
            throw JSONRPCError(RPC_INVALID_PARAMS, "Invalid argument 1 (address)");
        string address = request.params[0].get_str();

        int pageInitBlock = chainActive.Height();
        if (request.params.size() > 1 && request.params[1].isNum())
            pageInitBlock = request.params[1].get_int();

        int pageStart = 1;
        if (request.params.size() > 2 && request.params[2].isNum())
            pageStart = request.params[2].get_int();

        int pageSize = 10;
        if (request.params.size() > 3 && request.params[3].isNum())
            pageSize = request.params[3].get_int();

        auto txHashesOrdered = request.DbConnection()->ExplorerRepoInst->GetAddressTransactions(
            address,
            pageInitBlock,
            pageStart,
            pageSize
        );

        vector<string> txHashes;
        for(const auto& hashOrdered : txHashesOrdered)
            txHashes.push_back(hashOrdered.first);

        auto pBlock = request.DbConnection()->TransactionRepoInst->List(txHashes, false, true, true);

        UniValue result(UniValue::VARR);
        for (const auto& ptx : *pBlock)
        {
            UniValue utx = _constructTransaction(ptx);
            utx.pushKV("rowNumber", txHashesOrdered[*ptx->GetHash()]);
            result.push_back(utx);
        }

        return result;
    }

    UniValue GetBlockTransactions(const JSONRPCRequest& request)
    {
        if (request.fHelp)
        {
            throw runtime_error(
                "getblocktransactions [blockHash, pageStart, pageSize]\n"
                "\nGet transactions info.\n"
                "\nArguments:\n"
                "1. \"blockHash\"     (string, required) Block hash\n"
                "2. \"pageStart\"     (number) Row number for start page\n"
                "3. \"pageSize\"      (number) Page size\n"
            );
        }

        if (request.params.empty() || !request.params[0].isStr())
            throw JSONRPCError(RPC_INVALID_PARAMS, "Invalid argument 1 (blockHash)");
        string blockHash = request.params[0].get_str();

        int pageStart = 1;
        if (request.params.size() > 1 && request.params[1].isNum())
            pageStart = request.params[1].get_int();

        int pageSize = 10;
        if (request.params.size() > 2 && request.params[2].isNum())
            pageSize = request.params[2].get_int();

        auto txHashesOrdered = request.DbConnection()->ExplorerRepoInst->GetBlockTransactions(
            blockHash,
            pageStart,
            pageSize
        );

        vector<string> txHashes;
        for(const auto& hashOrdered : txHashesOrdered)
            txHashes.push_back(hashOrdered.first);

        auto pBlock = request.DbConnection()->TransactionRepoInst->List(txHashes, false, true, true);

        UniValue result(UniValue::VARR);
        for (const auto& ptx : *pBlock)
        {
            UniValue utx = _constructTransaction(ptx);
            utx.pushKV("rowNumber", txHashesOrdered[*ptx->GetHash()]);
            result.push_back(utx);
        }

        return result;
    }
    
    UniValue GetTransaction(const JSONRPCRequest& request)
    {
        if (request.fHelp)
            throw runtime_error(
                "getrawtransaction\n"
                "\nGet transaction data.\n"
            );

        RPCTypeCheck(request.params, {UniValue::VSTR});
        string txHash = request.params[0].get_str();

        auto pBlock = request.DbConnection()->TransactionRepoInst->List({ txHash }, false, true, true);
        if (pBlock->empty())
            return UniValue(UniValue::VOBJ);

        UniValue result(UniValue::VARR);
        const auto& ptx = (*pBlock)[0];

        return _constructTransaction(ptx);
    }

    UniValue GetTransactions(const JSONRPCRequest& request)
    {
        if (request.fHelp)
        {
            throw runtime_error(
                "gettransactions [transactions[], pageStart, pageSize]\n"
                "\nGet transactions info.\n"
                "\nArguments:\n"
                "1. \"transactions\"  (array, required) Transaction hashes\n"
            );
        }

        vector<string> transactions;
        if (request.params[0].isStr())
        {
            transactions.push_back(request.params[0].get_str());
        }
        else if (request.params[0].isArray())
        {
            UniValue atransactions = request.params[0].get_array();
            for (unsigned int idx = 0; idx < atransactions.size(); idx++)
                transactions.push_back(atransactions[idx].get_str());
        }
        else
        {
            throw JSONRPCError(RPC_INVALID_PARAMS, "Invalid inputs params");
        }

        auto pBlock = request.DbConnection()->TransactionRepoInst->List(transactions, false, true, true);

        UniValue result(UniValue::VARR);
        for (const auto& ptx : *pBlock)
        {
            UniValue utx = _constructTransaction(ptx);
            result.push_back(utx);
        }

        return result;
    }

    UniValue _constructTransaction(const PTransactionRef& ptx)
    {
        // General TX information
        UniValue utx(UniValue::VOBJ);

        utx.pushKV("txid", *ptx->GetHash());
        utx.pushKV("type", *ptx->GetType());
        if (ptx->GetHeight()) utx.pushKV("height", *ptx->GetHeight());
        if (ptx->GetBlockHash()) utx.pushKV("blockHash", *ptx->GetBlockHash());
        utx.pushKV("nTime", *ptx->GetTime());

        // Inputs
        utx.pushKV("vin", UniValue(UniValue::VARR));
        for (const auto& inp : ptx->Inputs())
        {
            UniValue uinp(UniValue::VOBJ);

            uinp.pushKV("txid", *inp->GetTxHash());
            uinp.pushKV("vout", *inp->GetNumber());
            if (inp->GetAddressHash()) uinp.pushKV("address", *inp->GetAddressHash());
            if (inp->GetValue()) uinp.pushKV("value", *inp->GetValue() / 100000000.0);

            utx.At("vin").push_back(uinp);
        }

        // Inputs
        utx.pushKV("vout", UniValue(UniValue::VARR));
        for (const auto& out : ptx->Outputs())
        {
            UniValue uout(UniValue::VOBJ);
            uout.pushKV("n", *out->GetNumber());
            uout.pushKV("value", *out->GetValue() / 100000000.0);

            UniValue scriptPubKey(UniValue::VOBJ);
            UniValue addresses(UniValue::VARR);
            addresses.push_back(*out->GetAddressHash());
            scriptPubKey.pushKV("addresses", addresses);
            scriptPubKey.pushKV("hex", *out->GetScriptPubKey());
            uout.pushKV("scriptPubKey", scriptPubKey);

            if (out->GetSpentHeight()) uout.pushKV("spent", *out->GetSpentHeight());

            utx.At("vout").push_back(uout);
        }

        return utx;
    }
}