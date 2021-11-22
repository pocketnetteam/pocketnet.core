// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include "pocketdb/web/PocketExplorerRpc.h"

namespace PocketWeb::PocketWebRpc
{

    UniValue GetStatistic(const JSONRPCRequest& request)
    {
        if (request.fHelp)
            throw std::runtime_error(
                "getstatistic (endTime, depth)\n"
                "\nGet statistics.\n"
                "\nArguments:\n"
                "1. \"endTime\"   (int64, optional) End time of period\n"
                "2. \"depth\"     (int32, optional) Day = 1, Month = 2, Year = 3\n");

        auto end_time = (int64_t) ::ChainActive().Tip()->nTime;
        if (!request.params.empty() && request.params[0].isNum())
            end_time = request.params[0].get_int64();

        StatisticDepth depth = StatisticDepth_Month;
        if (request.params.size() > 1 && request.params[1].isNum() &&
            request.params[1].get_int() >= 1 && request.params[1].get_int() <= 3
            )
            depth = (StatisticDepth) request.params[1].get_int();

        int64_t start_time;
        switch (depth)
        {
            case StatisticDepth_Day:
                start_time = end_time - (60 * 60 * 24);
                break;
            default:
            case StatisticDepth_Month:
                start_time = end_time - (60 * 60 * 24 * 30);
                break;
            case StatisticDepth_Year:
                start_time = end_time - (60 * 60 * 24 * 365);
                break;
        }

        return request.DbConnection()->ExplorerRepoInst->GetStatistic(start_time, end_time, depth);
    }

    UniValue GetLastBlocks(const JSONRPCRequest& request)
    {
        if (request.fHelp)
            throw std::runtime_error(
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

        int last_height = ::ChainActive().Height();
        if (request.params.size() > 1 && request.params[1].isNum())
        {
            last_height = request.params[1].get_int();
            if (last_height < 0) last_height = ::ChainActive().Height();
        }

        bool verbose = false;
        if (request.params.size() > 2)
        {
            RPCTypeCheckArgument(request.params[2], UniValue::VBOOL);
            verbose = request.params[2].get_bool();
        }

        // Collect general block information
        CBlockIndex* pindex = ::ChainActive()[last_height];
        int i = count;
        std::map<int, UniValue> blocks;
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
            auto data = request.DbConnection()->ExplorerRepoInst->GetStatistic(last_height - count, last_height);

            for (auto& s : data)
            {
                if (blocks.find(s.first) == blocks.end())
                    continue;

                if (blocks[s.first].At("types").isNull())
                    blocks[s.first].pushKV("types", UniValue(UniValue::VOBJ));

                for (auto& d : s.second)
                    blocks[s.first].At("types").pushKV(std::to_string(d.first), d.second);
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
            throw std::runtime_error(
                "getcompactblock \"blockhash\" or \"blocknumber\" \n"
                "\nArguments:\n"
                "1. \"blockhash\"          (string, required) The block hash\n"
                "1. \"blocknumber\"        (number, required) The block number\n");

        if (request.params.empty())
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Missing parameters");

        string blockHash;
        if (request.params[0].isStr())
            blockHash = request.params[0].get_str();

        int blockNumber = -1;
        if (request.params[0].isNum())
            blockNumber = request.params[0].get_int();


        const CBlockIndex* pindex = nullptr;

        if (!blockHash.empty())
            pindex = LookupBlockIndexWithoutLock(uint256S(blockHash));

        if (blockNumber >= 0 && blockNumber <= ::ChainActive().Height())
            pindex = ::ChainActive()[blockNumber];

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

        auto pnext = ::ChainActive().Next(pindex);
        if (pnext)
            result.pushKV("nexthash", pnext->GetBlockHash().GetHex());

        return result;
    }

    UniValue GetAddressSpent(const JSONRPCRequest& request)
    {
        if (request.fHelp)
            throw std::runtime_error(
                "getaddressspent \"address\"\n"
                "\nGet address spent & unspent amounts.\n"
                "\nArguments:\n"
                "1. \"address\"    (string) Address\n");

        std::string address;
        if (request.params.empty() || !request.params[0].isStr())
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid address argument");

        auto dest = DecodeDestination(request.params[0].get_str());
        if (!IsValidDestination(dest))
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY,
                std::string("Invalid address: ") + request.params[0].get_str());
        address = request.params[0].get_str();

        auto[spent, unspent] = request.DbConnection()->ExplorerRepoInst->GetAddressSpent(address);

        UniValue addressInfo(UniValue::VOBJ);
        addressInfo.pushKV("spent", spent);
        addressInfo.pushKV("unspent", unspent);

        return addressInfo;
    }

    UniValue SearchByHash(const JSONRPCRequest& request)
    {
        if (request.fHelp)
            throw std::runtime_error(
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
            throw std::runtime_error(
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

        int pageInitBlock = ::ChainActive().Height();
        if (request.params.size() > 1 && request.params[1].isNum())
            pageInitBlock = request.params[1].get_int();

        int pageStart = 1;
        if (request.params.size() > 2 && request.params[2].isNum())
            pageStart = request.params[2].get_int();

        int pageSize = 10;
        if (request.params.size() > 3 && request.params[3].isNum())
            pageSize = request.params[3].get_int();

        return request.DbConnection()->ExplorerRepoInst->GetAddressTransactions(
            address,
            pageInitBlock,
            pageStart,
            pageSize
        );
    }

    UniValue GetBlockTransactions(const JSONRPCRequest& request)
    {
        if (request.fHelp)
        {
            throw std::runtime_error(
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

        return request.DbConnection()->ExplorerRepoInst->GetBlockTransactions(
            blockHash,
            pageStart,
            pageSize
        );
    }

    UniValue GetTransactions(const JSONRPCRequest& request)
    {
        if (request.fHelp)
        {
            throw std::runtime_error(
                "gettransactions [transactions[], pageStart, pageSize]\n"
                "\nGet transactions info.\n"
                "\nArguments:\n"
                "1. \"transactions\"  (array, required) Transaction hashes\n"
                "2. \"pageStart\"     (number) Row number for start page\n"
                "3. \"pageSize\"      (number) Page size\n"
            );
        }

        std::vector<std::string> transactions;
        if (request.params[0].isStr())
            transactions.push_back(request.params[0].get_str());
        else if (request.params[0].isArray())
        {
            UniValue atransactions = request.params[0].get_array();
            for (unsigned int idx = 0; idx < atransactions.size(); idx++)
            {
                transactions.push_back(atransactions[idx].get_str());
            }
        }
        else
        {
            throw JSONRPCError(RPC_INVALID_PARAMS, "Invalid inputs params");
        }

        int pageStart = 1;
        if (request.params.size() > 1 && request.params[1].isNum())
            pageStart = request.params[1].get_int();

        int pageSize = 10;
        if (request.params.size() > 2 && request.params[2].isNum())
            pageSize = request.params[2].get_int();

        return request.DbConnection()->ExplorerRepoInst->GetTransactions(
            transactions,
            pageStart,
            pageSize
        );
    }

}