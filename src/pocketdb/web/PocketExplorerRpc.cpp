// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include "pocketdb/web/PocketExplorerRpc.h"

namespace PocketWeb::PocketWebRpc
{
    map<string, UniValue> TransactionsStatisticCache;

    RPCHelpMan GetStatisticByHours()
    {
        return RPCHelpMan{"getstatisticbyhours",
                "\nGet statistics.\n",
                {
                    {"topheight", RPCArg::Type::NUM, RPCArg::Optional::OMITTED_NAMED_ARG, "Top height (Default: chain height)"},
                    {"depth", RPCArg::Type::NUM, RPCArg::Optional::OMITTED_NAMED_ARG, "Depth hours (Maximum: 24 hours)"},
                },
                {
                    // TODO (losty-fur): provide return description
                },
                RPCExamples{
                    // TODO (losty-fur): provide correct examples
                    // HelpExampleCli("getstatisticbyhours", "") +
                    // HelpExampleRpc("getstatisticbyhours", "")
                    ""
                },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
    {
        int topHeight = ChainActive().Height() / 10 * 10;
        if (request.params[0].isNum())
            topHeight = std::min(request.params[0].get_int(), topHeight);

        int depth = 24;
        if (request.params[1].isNum())
            depth = std::min(request.params[1].get_int(), depth);
        depth = depth * 60;

        return request.DbConnection()->ExplorerRepoInst->GetTransactionsStatisticByHours(topHeight, depth);
    },
        };
    }

    RPCHelpMan GetStatisticByDays()
    {
        return RPCHelpMan{"getstatisticbydays",
                "\nGet statistics.\n",
                {
                    {"topheight", RPCArg::Type::NUM, RPCArg::Optional::OMITTED_NAMED_ARG, "Top height (Default: chain height)"},
                    {"depth", RPCArg::Type::NUM, RPCArg::Optional::OMITTED_NAMED_ARG, "Depth days (Maximum: 30 days)"},
                },
                {
                    // TODO (losty-fur): provide return description
                },
                RPCExamples{
                    // TODO (losty-fur): provide correct examples
                    // HelpExampleCli("getstatisticbydays", "") +
                    // HelpExampleRpc("getstatisticbydays", "")
                    ""
                },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
    {

        int topHeight = ChainActive().Height() / 10 * 10;
        if (request.params[0].isNum())
            topHeight = std::min(request.params[0].get_int(), topHeight);

        int depth = 30;
        if (request.params[1].isNum())
            depth = std::min(request.params[1].get_int(), depth);
        depth = depth * 24 * 60;

        return request.DbConnection()->ExplorerRepoInst->GetTransactionsStatisticByDays(topHeight, depth);
    },
        };
    }

    RPCHelpMan GetStatisticContent()
    {
        return RPCHelpMan{"getstatisticcontent",
                "\nGet statistics for content transactions\n",
                {},
                {
                    // TODO (losty-fur): provide return description
                },
                RPCExamples{
                    HelpExampleCli("getstatisticcontent", "") +
                    HelpExampleRpc("getstatisticcontent", "")
                },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
    {
        return request.DbConnection()->ExplorerRepoInst->GetContentStatistic();
    },
        };
    }

    RPCHelpMan GetLastBlocks()
    {
        return RPCHelpMan{"getlastblocks",
                "\nGet N last blocks.\n",
                {
                    {"count", RPCArg::Type::NUM, RPCArg::Optional::OMITTED_NAMED_ARG, "Count of blocks. Maximum 100 blocks."},
                    {"last_height", RPCArg::Type::NUM, RPCArg::Optional::OMITTED_NAMED_ARG, "Height of last block, including."},
                    {"verbosity", RPCArg::Type::NUM, RPCArg::Optional::OMITTED_NAMED_ARG, "Verbosity output."},
                },
                {
                    // TODO (losty-fur): provide return description
                },
                RPCExamples{
                    HelpExampleCli("getlastblocks", "") +
                    HelpExampleRpc("getlastblocks", "")
                },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
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
            auto data = request.DbConnection()->ExplorerRepoInst->GetBlocksStatistic(last_height - count, last_height);

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
    },
        };
    }

    RPCHelpMan GetCompactBlock()
    {
        return RPCHelpMan{"getcompactblock",
                // TODO (team): description
                "",
                {
                    {"blockhash", RPCArg::Type::STR, RPCArg::Optional::OMITTED_NAMED_ARG, "The block hash"},
                    {"blocknumber", RPCArg::Type::NUM, RPCArg::Optional::OMITTED_NAMED_ARG, "The block number"},
                },
                {
                    // TODO (losty-fur): provide return description
                },
                RPCExamples{
                    HelpExampleCli("getcompactblock", "123acbadbcca") +
                    HelpExampleRpc("getcompactblock", "123acbadbcca") +
                    HelpExampleCli("getcompactblock", "7546744353") +
                    HelpExampleRpc("getcompactblock", "7546744353")
                },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
    {
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
    },
        };
    }

    RPCHelpMan GetAddressInfo()
    {
        return RPCHelpMan{"getaddressinfo",
                "\nGet contents statistic.\n",
                {
                    {"address", RPCArg::Type::STR, RPCArg::Optional::NO, "Address"},
                },
                {
                    // TODO (losty-fur): provide return description
                },
                RPCExamples{
                    HelpExampleCli("getaddressinfo", "1bd123c12f123a123b") +
                    HelpExampleRpc("getaddressinfo", "1bd123c12f123a123b")
                },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
    {
        std::string address;
        if (request.params.empty() || !request.params[0].isStr())
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid address argument");

        auto dest = DecodeDestination(request.params[0].get_str());
        if (!IsValidDestination(dest))
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY,
                std::string("Invalid address: ") + request.params[0].get_str());
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
    },
        };
    }

    RPCHelpMan SearchByHash()
    {
        return RPCHelpMan{"checkstringtype",
                "\nCheck type of input string - address, block or tx id.\n",
                {
                    {"string", RPCArg::Type::STR, RPCArg::Optional::NO, "Input string"},
                },
                {
                    // TODO (losty-fur): provide return description
                },
                RPCExamples{
                    HelpExampleCli("checkstringtype", "123123123123") +
                    HelpExampleRpc("checkstringtype", "123123123123")
                },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
    {
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
    },
        };
    }

    RPCHelpMan GetAddressTransactions()
    {
        return RPCHelpMan{"getaddresstransactions",
                "\nGet transactions info.\n",
                {
                    {"address", RPCArg::Type::STR, RPCArg::Optional::NO, "Address hash"},
                    {"pageInitBlock", RPCArg::Type::NUM, RPCArg::Optional::OMITTED_NAMED_ARG, "Max block height for filter pagination window"},
                    {"pageStart", RPCArg::Type::NUM, RPCArg::Optional::OMITTED_NAMED_ARG, "Row number for start page"},
                    {"pageSize", RPCArg::Type::NUM, RPCArg::Optional::OMITTED_NAMED_ARG, "Page size"},
                },
                {
                    // TODO (losty-fur): provide return description
                },
                RPCExamples{
                    HelpExampleCli("getaddresstransactions", "abaa1231bca1231") +
                    HelpExampleRpc("getaddresstransactions", "abaa1231bca1231")
                },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
    {
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
    },
        };
    }

    RPCHelpMan GetBlockTransactions()
    {
        return RPCHelpMan{"getblocktransactions",
                "\nGet transactions info.\n",
                {
                    {"blockHash", RPCArg::Type::STR, RPCArg::Optional::NO, "Block hash"},
                    {"pageStart", RPCArg::Type::NUM, RPCArg::Optional::OMITTED_NAMED_ARG, "Row number for start page"},
                    {"pageSize", RPCArg::Type::NUM, RPCArg::Optional::OMITTED_NAMED_ARG, "Page size"},
                },
                {
                    // TODO (losty-fur): provide return description
                },
                RPCExamples{
                    HelpExampleCli("getblocktransactions", "abaa1231bca1231") +
                    HelpExampleRpc("getblocktransactions", "abaa1231bca1231")
                },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
    {
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
    },
        };
    }

    RPCHelpMan GetTransactions()
    {
        return RPCHelpMan{"gettransactions",
                "\nGet transactions info.\n",
                {
                    {"transactions", RPCArg::Type::ARR, RPCArg::Optional::NO, "Transaction hashes",
                        {
                            {"transaction", RPCArg::Type::STR, RPCArg::Optional::NO, ""}   
                        }
                    },
                    {"pageStart", RPCArg::Type::NUM, RPCArg::Optional::OMITTED_NAMED_ARG, "Row number for start page"},
                    {"pageSize", RPCArg::Type::NUM, RPCArg::Optional::OMITTED_NAMED_ARG, "Page size"},
                },
                {
                    // TODO (losty-fur): provide return description
                },
                RPCExamples{
                    // TODO (team): provide examples
                    ""
                },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
    {
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
    },
        };
    }
}