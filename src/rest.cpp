// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "rest.h"

#include <chain.h>
#include <chainparams.h>
#include <core_io.h>
#include <index/txindex.h>
#include <node/context.h>
#include <primitives/block.h>
#include <primitives/transaction.h>
#include <rpc/blockchain.h>
#include <rpc/protocol.h>
#include <rpc/server.h>
#include <streams.h>
#include <sync.h>
#include <txmempool.h>
#include <util/check.h>
#include <util/ref.h>
#include <util/strencodings.h>
#include <validation.h>
#include <version.h>
#include <pos.h>

#include <boost/algorithm/string.hpp>
#include <univalue.h>

#include "pocketdb/SQLiteConnection.h"
#include "pocketdb/services/ChainPostProcessing.h"
#include "pocketdb/consensus/Helper.h"
#include "pocketdb/services/Accessor.h"
#include "pocketdb/web/PocketFrontend.h"
#include "rpcapi/rpcapi.h"
#include "util/system.h"

#include <memory>
#include <vector>

static const size_t MAX_GETUTXOS_OUTPOINTS = 15; //allow a max of 15 outpoints to be queried at once

enum class RetFormat
{
    UNDEF,
    BINARY,
    HEX,
    JSON,
};

static const struct
{
    RetFormat rf;
    const char* name;
}rf_names[] = {
    {RetFormat::UNDEF,  ""},
    {RetFormat::BINARY, "bin"},
    {RetFormat::HEX,    "hex"},
    {RetFormat::JSON,   "json"},
};

struct CCoin {
    uint32_t nHeight;
    CTxOut out;

    CCoin() : nHeight(0) {}
    explicit CCoin(Coin&& in) : nHeight(in.nHeight), out(std::move(in.out)) {}

    SERIALIZE_METHODS(CCoin, obj)
    {
        uint32_t nTxVerDummy = 0;
        READWRITE(nTxVerDummy, obj.nHeight, obj.out);
    }
};

static bool RESTERR(const std::shared_ptr<IReplier>& replier, enum HTTPStatusCode status, std::string message)
{
    replier->WriteHeader("Content-Type", "text/plain");
    replier->WriteReply(status, message + "\r\n");
    return false;
}

/**
 * Get the node context.
 *
 * @param[in]  req  The HTTP request, whose status code will be set if node
 *                  context is not found.
 * @returns         Pointer to the node context or nullptr if not found.
 */
static NodeContext* GetNodeContext(const util::Ref& context, const std::shared_ptr<IReplier>& replier)
{
    NodeContext* node = context.Has<NodeContext>() ? &context.Get<NodeContext>() : nullptr;
    if (!node) {
        RESTERR(replier, HTTP_INTERNAL_SERVER_ERROR,
                strprintf("%s:%d (%s)\n"
                          "Internal bug detected: Node context not found!\n"
                          "You may report this issue here: %s\n",
                          __FILE__, __LINE__, __func__, PACKAGE_BUGREPORT));
        return nullptr;
    }
    return node;
}

/**
 * Get the node context mempool.
 *
 * @param[in]  req The HTTP request, whose status code will be set if node
 *                 context mempool is not found.
 * @returns        Pointer to the mempool or nullptr if no mempool found.
 */
static CTxMemPool* GetMemPool(const util::Ref& context, const std::shared_ptr<IReplier>& replier)
{
    NodeContext* node = context.Has<NodeContext>() ? &context.Get<NodeContext>() : nullptr;
    if (!node || !node->mempool) {
        RESTERR(replier, HTTP_NOT_FOUND, "Mempool disabled or instance not found");
        return nullptr;
    }
    return node->mempool.get();
}

static RetFormat ParseDataFormat(std::string& param, const std::string& strReq)
{
    const std::string::size_type pos = strReq.rfind('.');
    if (pos == std::string::npos)
    {
        param = strReq;
        return rf_names[0].rf;
    }

    param = strReq.substr(0, pos);
    const std::string suff(strReq, pos + 1);

    for (unsigned int i = 0; i < ARRAYLEN(rf_names); i++)
        if (suff == rf_names[i].name)
            return rf_names[i].rf;

    /* If no suffix is found, return original string.  */
    param = strReq;
    return rf_names[0].rf;
}

static std::tuple <RetFormat, std::vector<std::string>> ParseParams(const std::string& strURIPart)
{
    std::string param;
    RetFormat rf = ParseDataFormat(param, strURIPart);

    std::vector <std::string> uriParts;
    if (param.length() > 1)
    {
        std::string strUriParams = param.substr(1);
        boost::split(uriParts, strUriParams, boost::is_any_of("/"));
    }

    return std::make_tuple(rf, uriParts);
};

static std::tuple<bool, int> TryGetParamInt(std::vector<std::string>& uriParts, int index)
{
    try
    {
        if ((int) uriParts.size() > index)
        {
            auto val = std::stoi(uriParts[index]);
            return std::make_tuple(true, val);
        }

        return std::make_tuple(false, 0);
    }
    catch (...)
    {
        return std::make_tuple(false, 0);
    }
}

static std::tuple<bool, std::string> TryGetParamStr(std::vector<std::string>& uriParts, int index)
{
    try
    {
        if ((int) uriParts.size() > index)
        {
            auto val = uriParts[index];
            return std::make_tuple(true, val);
        }

        return std::make_tuple(false, "");
    }
    catch (...)
    {
        return std::make_tuple(false, "");
    }
}

static std::string AvailableDataFormatsString()
{
    std::string formats;
    for (unsigned int i = 0; i < ARRAYLEN(rf_names); i++)
        if (strlen(rf_names[i].name) > 0)
        {
            formats.append(".");
            formats.append(rf_names[i].name);
            formats.append(", ");
        }

    if (formats.length() > 0)
        return formats.substr(0, formats.length() - 2);

    return formats;
}

static bool CheckWarmup(const std::shared_ptr<IReplier>& replier)
{
    std::string statusmessage;
    if (RPCIsInWarmup(&statusmessage))
        return RESTERR(replier, HTTP_SERVICE_UNAVAILABLE, "Service temporarily unavailable: " + statusmessage);
    return true;
}

static bool rest_headers(const RequestContext& reqContext, const DbConnectionRef& sqliteConnection)
{
    const auto& replier = reqContext.replier;
    if (!CheckWarmup(replier))
        return false;
    std::string param;
    const RetFormat rf = ParseDataFormat(param, reqContext.path);
    std::vector <std::string> path;
    boost::split(path, param, boost::is_any_of("/"));

    if (path.size() != 2)
        return RESTERR(replier, HTTP_BAD_REQUEST, "No header count specified. Use /rest/headers/<count>/<hash>.<ext>.");

    long count = strtol(path[0].c_str(), nullptr, 10);
    if (count < 1 || count > 2000)
        return RESTERR(replier, HTTP_BAD_REQUEST, "Header count out of range: " + path[0]);

    std::string hashStr = path[1];
    uint256 hash;
    if (!ParseHashStr(hashStr, hash))
        return RESTERR(replier, HTTP_BAD_REQUEST, "Invalid hash: " + hashStr);

    const CBlockIndex* tip = nullptr;
    std::vector<const CBlockIndex*> headers;
    headers.reserve(count);
    {
        LOCK(cs_main);
        tip = ::ChainActive().Tip();
        const CBlockIndex* pindex = LookupBlockIndex(hash);
        while (pindex != nullptr && ::ChainActive().Contains(pindex))
        {
            headers.push_back(pindex);
            if (headers.size() == (unsigned long) count)
                break;
            pindex = ::ChainActive().Next(pindex);
        }
    }

    switch (rf)
    {
        case RetFormat::BINARY:
        {
            CDataStream ssHeader(SER_NETWORK, PROTOCOL_VERSION);
            for (const CBlockIndex *pindex : headers) {
                ssHeader << pindex->GetBlockHeader();
            }

            std::string binaryHeader = ssHeader.str();
            replier->WriteHeader("Content-Type", "application/octet-stream");
            replier->WriteReply(HTTP_OK, binaryHeader);
            return true;
        }

        case RetFormat::HEX:
        {
            CDataStream ssHeader(SER_NETWORK, PROTOCOL_VERSION);
            for (const CBlockIndex* pindex: headers)
            {
                ssHeader << pindex->GetBlockHeader();
            }

            std::string strHex = HexStr(ssHeader) + "\n";
            replier->WriteHeader("Content-Type", "text/plain");
            replier->WriteReply(HTTP_OK, strHex);
            return true;
        }
        case RetFormat::JSON:
        {
            UniValue jsonHeaders(UniValue::VARR);
            for (const CBlockIndex *pindex: headers)
            {
                jsonHeaders.push_back(blockheaderToJSON(tip, pindex));
            }
            std::string strJSON = jsonHeaders.write() + "\n";
            replier->WriteHeader("Content-Type", "application/json");
            replier->WriteReply(HTTP_OK, strJSON);
            return true;
        }
        default:
        {
            return RESTERR(replier, HTTP_NOT_FOUND, "output format not found (available: .bin, .hex, .json)");
        }
    }
}

static bool rest_block(const std::shared_ptr<IReplier>& replier,
    const std::string& strURIPart,
    bool showTxDetails)
{
    if (!CheckWarmup(replier))
        return false;
    std::string hashStr;
    const RetFormat rf = ParseDataFormat(hashStr, strURIPart);

    uint256 hash;
    if (!ParseHashStr(hashStr, hash))
        return RESTERR(replier, HTTP_BAD_REQUEST, "Invalid hash: " + hashStr);

    CBlock block;
    CBlockIndex* pblockindex = nullptr;
    CBlockIndex* tip = nullptr;
    {
        LOCK(cs_main);
        tip = ::ChainActive().Tip();
        pblockindex = LookupBlockIndex(hash);
        if (!pblockindex) {
            return RESTERR(replier, HTTP_NOT_FOUND, hashStr + " not found");
        }

        if (IsBlockPruned(pblockindex))
            return RESTERR(replier, HTTP_NOT_FOUND, hashStr + " not available (pruned data)");

        if (!ReadBlockFromDisk(block, pblockindex, Params().GetConsensus()))
            return RESTERR(replier, HTTP_NOT_FOUND, hashStr + " not found");
    }

    switch (rf)
    {
        case RetFormat::BINARY:
        {
            CDataStream ssBlock(SER_NETWORK, PROTOCOL_VERSION | RPCSerializationFlags());
            ssBlock << block;
            std::string binaryBlock = ssBlock.str();
            replier->WriteHeader("Content-Type", "application/octet-stream");
            replier->WriteReply(HTTP_OK, binaryBlock);
            return true;
        }

        case RetFormat::HEX:
        {
            CDataStream ssBlock(SER_NETWORK, PROTOCOL_VERSION | RPCSerializationFlags());
            ssBlock << block;
            std::string strHex = HexStr(ssBlock) + "\n";
            replier->WriteHeader("Content-Type", "text/plain");
            replier->WriteReply(HTTP_OK, strHex);
            return true;
        }

        case RetFormat::JSON:
        {
            UniValue objBlock = blockToJSON(block, tip, pblockindex, showTxDetails);
            std::string strJSON = objBlock.write() + "\n";
            replier->WriteHeader("Content-Type", "application/json");
            replier->WriteReply(HTTP_OK, strJSON);
            return true;
        }

        default:
        {
            return RESTERR(replier, HTTP_NOT_FOUND,
                "output format not found (available: " + AvailableDataFormatsString() + ")");
        }
    }
}

static bool rest_blockhash(const RequestContext& reqContext, const DbConnectionRef& sqliteConnection)
{
    const auto& replier = reqContext.replier;
    if (!CheckWarmup(replier))
        return false;

    auto[rf, uriParts] = ParseParams(reqContext.path);

    int height = ChainActive().Height();
    if (auto[ok, value] = TryGetParamInt(uriParts, 0); ok)
        height = value;

    if (height < 0 || height > ChainActive().Height())
        return RESTERR(replier, HTTP_BAD_REQUEST, "Block height out of range");

    CBlockIndex* pblockindex = ChainActive()[height];
    std::string blockHash = pblockindex->GetBlockHash().GetHex();

    switch (rf)
    {
        case RetFormat::JSON:
        {
            UniValue result(UniValue::VOBJ);
            result.pushKV("height", height);
            result.pushKV("blockhash", blockHash);

            replier->WriteHeader("Content-Type", "application/json");
            replier->WriteReply(HTTP_OK, result.write() + "\n");
            return true;
        }
        default:
        {
            replier->WriteHeader("Content-Type", "text/plain");
            replier->WriteReply(HTTP_OK, blockHash + "\n");
            return true;
        }
    }
}

static bool rest_block_extended(const RequestContext& reqContext, const DbConnectionRef& sqliteConnection)
{
    return rest_block(reqContext.replier, reqContext.path, true);
}

static bool rest_block_notxdetails(const RequestContext& reqContext, const DbConnectionRef& sqliteConnection)
{
    return rest_block(reqContext.replier, reqContext.path, false);
}

// A bit of a hack - dependency on a function defined in rpc/blockchain.cpp
RPCHelpMan getblockchaininfo();

static bool rest_chaininfo(const RequestContext& reqContext, const DbConnectionRef& sqliteConnection)
{
    const auto& replier = reqContext.replier;
    const auto& context = reqContext.context;
    const auto& path = reqContext.path;

    if (!CheckWarmup(replier))
        return false;
    std::string param;
    const RetFormat rf = ParseDataFormat(param, path);

    switch (rf)
    {
        case RetFormat::JSON:
        {
            JSONRPCRequest jsonRequest(context);
            jsonRequest.params = UniValue(UniValue::VARR);
            UniValue chainInfoObject = getblockchaininfo().HandleRequest(jsonRequest);
            std::string strJSON = chainInfoObject.write() + "\n";
            replier->WriteHeader("Content-Type", "application/json");
            replier->WriteReply(HTTP_OK, strJSON);
            return true;
        }
        default:
        {
            return RESTERR(replier, HTTP_NOT_FOUND, "output format not found (available: json)");
        }
    }
}

static bool rest_mempool_info(const RequestContext& reqContext, const DbConnectionRef& sqliteConnection)
{
    const auto& replier = reqContext.replier;
    const auto& context = reqContext.context;
    const auto& path = reqContext.path;

    if (!CheckWarmup(replier))
        return false;
    const CTxMemPool* mempool = GetMemPool(context, replier);
    if (!mempool) return false;
    std::string param;
    const RetFormat rf = ParseDataFormat(param, path);

    switch (rf)
    {
        case RetFormat::JSON:
        {
            UniValue mempoolInfoObject = mempoolInfoToJSON(*mempool);

            std::string strJSON = mempoolInfoObject.write() + "\n";
            replier->WriteHeader("Content-Type", "application/json");
            replier->WriteReply(HTTP_OK, strJSON);
            return true;
        }
        default:
        {
            return RESTERR(replier, HTTP_NOT_FOUND, "output format not found (available: json)");
        }
    }
}

static bool rest_mempool_contents(const RequestContext& reqContext, const DbConnectionRef& sqliteConnection)
{
    const auto& replier = reqContext.replier;
    const auto& context = reqContext.context;
    const auto& path = reqContext.path;

    if (!CheckWarmup(replier))
        return false;
    const CTxMemPool* mempool = GetMemPool(context, replier);
    if (!mempool) return false;
    std::string param;
    const RetFormat rf = ParseDataFormat(param, path);

    switch (rf)
    {
        case RetFormat::JSON:
        {
            UniValue mempoolObject = MempoolToJSON(*mempool, true);

            std::string strJSON = mempoolObject.write() + "\n";
            replier->WriteHeader("Content-Type", "application/json");
            replier->WriteReply(HTTP_OK, strJSON);
            return true;
        }
        default:
        {
            return RESTERR(replier, HTTP_NOT_FOUND, "output format not found (available: json)");
        }
    }
}

static bool rest_tx(const RequestContext& reqContext, const DbConnectionRef& sqliteConnection)
{
    const auto& replier = reqContext.replier;
    const auto& context = reqContext.context;
    const auto& path = reqContext.path;

    if (!CheckWarmup(replier))
        return false;
    std::string hashStr;
    const RetFormat rf = ParseDataFormat(hashStr, path);

    uint256 hash;
    if (!ParseHashStr(hashStr, hash))
        return RESTERR(replier, HTTP_BAD_REQUEST, "Invalid hash: " + hashStr);

    if (g_txindex)
    {
        g_txindex->BlockUntilSyncedToCurrentChain();
    }

    const NodeContext* const node = GetNodeContext(context, replier);
    if (!node) return false;
    uint256 hashBlock = uint256();
    const CTransactionRef tx = GetTransaction(/* block_index */ nullptr, node->mempool.get(), hash, Params().GetConsensus(), hashBlock);
    if (!tx) {
        return RESTERR(replier, HTTP_NOT_FOUND, hashStr + " not found");
    }

    switch (rf)
    {
        case RetFormat::BINARY:
        {
            CDataStream ssTx(SER_NETWORK, PROTOCOL_VERSION | RPCSerializationFlags());
            ssTx << tx;

            std::string binaryTx = ssTx.str();
            replier->WriteHeader("Content-Type", "application/octet-stream");
            replier->WriteReply(HTTP_OK, binaryTx);
            return true;
        }

        case RetFormat::HEX:
        {
            CDataStream ssTx(SER_NETWORK, PROTOCOL_VERSION | RPCSerializationFlags());
            ssTx << tx;

            std::string strHex = HexStr(ssTx) + "\n";
            replier->WriteHeader("Content-Type", "text/plain");
            replier->WriteReply(HTTP_OK, strHex);
            return true;
        }

        case RetFormat::JSON:
        {
            UniValue objTx(UniValue::VOBJ);
            TxToUniv(*tx, hashBlock, objTx);
            std::string strJSON = objTx.write() + "\n";
            replier->WriteHeader("Content-Type", "application/json");
            replier->WriteReply(HTTP_OK, strJSON);
            return true;
        }

        default:
        {
            return RESTERR(replier, HTTP_NOT_FOUND,
                "output format not found (available: " + AvailableDataFormatsString() + ")");
        }
    }
}

static bool rest_getutxos(const RequestContext& reqContext, const DbConnectionRef& sqliteConnection)
{
    const auto& replier = reqContext.replier;
    const auto& context = reqContext.context;
    const auto& path = reqContext.path;
    const auto& body = reqContext.body;

    if (!CheckWarmup(replier))
        return false;
    std::string param;
    const RetFormat rf = ParseDataFormat(param, path);

    std::vector <std::string> uriParts;
    if (param.length() > 1)
    {
        std::string strUriParams = param.substr(1);
        boost::split(uriParts, strUriParams, boost::is_any_of("/"));
    }

    // throw exception in case of an empty request
    std::string strRequestMutable = body;
    if (strRequestMutable.length() == 0 && uriParts.size() == 0)
        return RESTERR(replier, HTTP_BAD_REQUEST, "Error: empty request");

    bool fInputParsed = false;
    bool fCheckMemPool = false;
    std::vector <COutPoint> vOutPoints;

    // parse/deserialize input
    // input-format = output-format, rest/getutxos/bin requires binary input, gives binary output, ...

    if (uriParts.size() > 0)
    {
        //inputs is sent over URI scheme (/rest/getutxos/checkmempool/txid1-n/txid2-n/...)
        if (uriParts[0] == "checkmempool") fCheckMemPool = true;

        for (size_t i = (fCheckMemPool) ? 1 : 0; i < uriParts.size(); i++)
        {
            uint256 txid;
            int32_t nOutput;
            std::string strTxid = uriParts[i].substr(0, uriParts[i].find('-'));
            std::string strOutput = uriParts[i].substr(uriParts[i].find('-') + 1);

            if (!ParseInt32(strOutput, &nOutput) || !IsHex(strTxid))
                return RESTERR(replier, HTTP_BAD_REQUEST, "Parse error");

            txid.SetHex(strTxid);
            vOutPoints.push_back(COutPoint(txid, (uint32_t) nOutput));
        }

        if (vOutPoints.size() > 0)
            fInputParsed = true;
        else
            return RESTERR(replier, HTTP_BAD_REQUEST, "Error: empty request");
    }

    switch (rf)
    {
        case RetFormat::HEX:
        {
            // convert hex to bin, continue then with bin part
            std::vector<unsigned char> strRequestV = ParseHex(strRequestMutable);
            strRequestMutable.assign(strRequestV.begin(), strRequestV.end());
        }

        case RetFormat::BINARY:
        {
            try
            {
                //deserialize only if user sent a request
                if (strRequestMutable.size() > 0)
                {
                    if (fInputParsed) //don't allow sending input over URI and HTTP RAW DATA
                        return RESTERR(replier, HTTP_BAD_REQUEST,
                            "Combination of URI scheme inputs and raw post data is not allowed");

                    CDataStream oss(SER_NETWORK, PROTOCOL_VERSION);
                    oss << strRequestMutable;
                    oss >> fCheckMemPool;
                    oss >> vOutPoints;
                }
            } catch (const std::ios_base::failure&)
            {
                // abort in case of unreadable binary data
                return RESTERR(replier, HTTP_BAD_REQUEST, "Parse error");
            }
            break;
        }

        case RetFormat::JSON:
        {
            if (!fInputParsed)
                return RESTERR(replier, HTTP_BAD_REQUEST, "Error: empty request");
            break;
        }
        default:
        {
            return RESTERR(replier, HTTP_NOT_FOUND,
                "output format not found (available: " + AvailableDataFormatsString() + ")");
        }
    }

    // limit max outpoints
    if (vOutPoints.size() > MAX_GETUTXOS_OUTPOINTS)
        return RESTERR(replier, HTTP_BAD_REQUEST,
            strprintf("Error: max outpoints exceeded (max: %d, tried: %d)", MAX_GETUTXOS_OUTPOINTS, vOutPoints.size()));

    // check spentness and form a bitmap (as well as a JSON capable human-readable string representation)
    std::vector<unsigned char> bitmap;
    std::vector <CCoin> outs;
    std::string bitmapStringRepresentation;
    std::vector<bool> hits;
    bitmap.resize((vOutPoints.size() + 7) / 8);
    {
        auto process_utxos = [&vOutPoints, &outs, &hits](const CCoinsView& view, const CTxMemPool& mempool)
        {
            for (const COutPoint& vOutPoint: vOutPoints)
            {
                Coin coin;
                bool hit = !mempool.isSpent(vOutPoint) && view.GetCoin(vOutPoint, coin);
                hits.push_back(hit);
                if (hit) outs.emplace_back(std::move(coin));
            }
        };

        if (fCheckMemPool)
        {
            const CTxMemPool* mempool = GetMemPool(context, replier);
            if (!mempool) return false;
            // use db+mempool as cache backend in case user likes to query mempool
            LOCK2(cs_main, mempool->cs);
            CCoinsViewCache& viewChain = ::ChainstateActive().CoinsTip();
            CCoinsViewMemPool viewMempool(&viewChain, *mempool);
            process_utxos(viewMempool, *mempool);
        }
        else
        {
            LOCK(cs_main);  // no need to lock mempool!
            process_utxos(::ChainstateActive().CoinsTip(), CTxMemPool());
        }

        for (size_t i = 0; i < hits.size(); ++i)
        {
            const bool hit = hits[i];
            bitmapStringRepresentation.append(
                hit ? "1" : "0"); // form a binary string representation (human-readable for json output)
            bitmap[i / 8] |= ((uint8_t) hit) << (i % 8);
        }
    }

    switch (rf)
    {
        case RetFormat::BINARY:
        {
            // serialize data
            // use exact same output as mentioned in Bip64
            CDataStream ssGetUTXOResponse(SER_NETWORK, PROTOCOL_VERSION);
            ssGetUTXOResponse << ::ChainActive().Height() << ::ChainActive().Tip()->GetBlockHash() << bitmap << outs;
            std::string ssGetUTXOResponseString = ssGetUTXOResponse.str();

            replier->WriteHeader("Content-Type", "application/octet-stream");
            replier->WriteReply(HTTP_OK, ssGetUTXOResponseString);
            return true;
        }

        case RetFormat::HEX:
        {
            CDataStream ssGetUTXOResponse(SER_NETWORK, PROTOCOL_VERSION);
            ssGetUTXOResponse << ::ChainActive().Height() << ::ChainActive().Tip()->GetBlockHash() << bitmap << outs;
            std::string strHex = HexStr(ssGetUTXOResponse) + "\n";

            replier->WriteHeader("Content-Type", "text/plain");
            replier->WriteReply(HTTP_OK, strHex);
            return true;
        }

        case RetFormat::JSON:
        {
            UniValue objGetUTXOResponse(UniValue::VOBJ);

            // pack in some essentials
            // use more or less the same output as mentioned in Bip64
            objGetUTXOResponse.pushKV("chainHeight", ::ChainActive().Height());
            objGetUTXOResponse.pushKV("chaintipHash", ::ChainActive().Tip()->GetBlockHash().GetHex());
            objGetUTXOResponse.pushKV("bitmap", bitmapStringRepresentation);

            UniValue utxos(UniValue::VARR);
            for (const CCoin& coin: outs)
            {
                UniValue utxo(UniValue::VOBJ);
                utxo.pushKV("height", (int32_t) coin.nHeight);
                utxo.pushKV("value", ValueFromAmount(coin.out.nValue));

                // include the script in a json output
                UniValue o(UniValue::VOBJ);
                ScriptPubKeyToUniv(coin.out.scriptPubKey, o, true);
                utxo.pushKV("scriptPubKey", o);
                utxos.push_back(utxo);
            }
            objGetUTXOResponse.pushKV("utxos", utxos);

            // return json string
            std::string strJSON = objGetUTXOResponse.write() + "\n";
            replier->WriteHeader("Content-Type", "application/json");
            replier->WriteReply(HTTP_OK, strJSON);
            return true;
        }
        default:
        {
            return RESTERR(replier, HTTP_NOT_FOUND,
                "output format not found (available: " + AvailableDataFormatsString() + ")");
        }
    }
}

static double getEmission(int height)
{
    int first75 = 3750000;
    int halvblocks = 2'100'000;
    double emission = 0;
    int nratio = height / halvblocks;
    double mult;

    for (int i = 0; i <= nratio; ++i)
    {
        mult = 5. / pow(2., static_cast<double>(i));
        if (i < nratio || nratio == 0)
        {
            if (i == 0 && height < 75'000)
                emission += height * 50;
            else if (i == 0)
            {
                emission += first75 + (std::min(height, halvblocks) - 75000) * 5;
            }
            else
            {
                emission += 2'100'000 * mult;
            }
        }

        if (i == nratio && nratio != 0)
        {
            emission += (height % halvblocks) * mult;
        }
    }

    return emission;
}

static bool rest_emission(const RequestContext& reqContext, const DbConnectionRef& sqliteConnection)
{
    const auto& replier = reqContext.replier;
    const auto& path = reqContext.path;
    if (!CheckWarmup(replier))
        return false;

    auto[rf, uriParts] = ParseParams(path);
    
    int height = ChainActive().Height();
    if (auto[ok, value] = TryGetParamInt(uriParts, 0); ok)
        height = std::max(value, 0);

    replier->WriteHeader("Content-Type", "text/plain");
    replier->WriteReply(HTTP_OK, std::to_string(getEmission(height)));
    return true;
}

static bool get_static_status(const RequestContext& reqContext)
{
    const auto& replier = reqContext.replier;
    if (!CheckWarmup(replier))
        return false;

    // TODO (aok): join with PocketSystemRpc.cpp::GetNodeInfo

    UniValue entry(UniValue::VOBJ);

    // General information about instance
    entry.pushKV("version", FormatVersion(CLIENT_VERSION));
    entry.pushKV("time", GetAdjustedTime());
    entry.pushKV("chain", Params().NetworkIDString());
    entry.pushKV("proxy", true);

    // Network information
    uint64_t nNetworkWeight = GetPoSKernelPS();
    entry.pushKV("netstakeweight", (uint64_t)nNetworkWeight);
    entry.pushKV("emission", getEmission(ChainActive().Height()));
    
    // Last block
    CBlockIndex* pindex = ChainActive().Tip();
    UniValue oblock(UniValue::VOBJ);
    oblock.pushKV("height", pindex->nHeight);
    oblock.pushKV("hash", pindex->GetBlockHash().GetHex());
    oblock.pushKV("time", (int64_t)pindex->nTime);
    oblock.pushKV("ntx", (int)pindex->nTx);
    entry.pushKV("lastblock", oblock);

    UniValue proxies(UniValue::VARR);
    if (notificationProcessor) {
        proxies = notificationProcessor->CollectStats();
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

    replier->WriteHeader("Content-Type", "application/json");
    replier->WriteReply(HTTP_OK, entry.write() + "\n");
    return true;
}

static bool get_static_web(const RequestContext& reqContext, const DbConnectionRef& sqliteConnection)
{
    const auto& replier = reqContext.replier;
    const auto& path = reqContext.path;

    if (!CheckWarmup(replier))
        return false;

    if (path.find("clear") == 0)
    {
        PocketWeb::PocketFrontendInst.ClearCache();
        replier->WriteReply(HTTP_OK, "Cache cleared!");
        return true;
    }

    if (path.find("status") == 0)
        return get_static_status(reqContext);

    if (auto[code, file] = PocketWeb::PocketFrontendInst.GetFile(path); code == HTTP_OK)
    {
        replier->WriteHeader("Content-Type", file->ContentType);
        replier->WriteReply(code, file->Content);
        return true;
    }
    else
    {
        return RESTERR(replier, code, "");
    }

    return RESTERR(replier, HTTP_NOT_FOUND, "");
}

static bool rest_blockhash_by_height(const RequestContext& reqContext, const DbConnectionRef& sqliteConnection)
{
    const auto& replier = reqContext.replier;
    const auto& path = reqContext.path;

    if (!CheckWarmup(replier)) return false;
    std::string height_str;
    const RetFormat rf = ParseDataFormat(height_str, path);

    int32_t blockheight = -1; // Initialization done only to prevent valgrind false positive, see https://github.com/bitcoin/bitcoin/pull/18785
    if (!ParseInt32(height_str, &blockheight) || blockheight < 0) {
        return RESTERR(replier, HTTP_BAD_REQUEST, "Invalid height: " + SanitizeString(height_str));
    }

    CBlockIndex* pblockindex = nullptr;
    {
        LOCK(cs_main);
        if (blockheight > ::ChainActive().Height()) {
            return RESTERR(replier, HTTP_NOT_FOUND, "Block height out of range");
        }
        pblockindex = ::ChainActive()[blockheight];
    }
    switch (rf) {
        case RetFormat::BINARY: {
            CDataStream ss_blockhash(SER_NETWORK, PROTOCOL_VERSION);
            ss_blockhash << pblockindex->GetBlockHash();
            replier->WriteHeader("Content-Type", "application/octet-stream");
            replier->WriteReply(HTTP_OK, ss_blockhash.str());
            return true;
        }
        case RetFormat::HEX: {
            replier->WriteHeader("Content-Type", "text/plain");
            replier->WriteReply(HTTP_OK, pblockindex->GetBlockHash().GetHex() + "\n");
            return true;
        }
        case RetFormat::JSON: {
            replier->WriteHeader("Content-Type", "application/json");
            UniValue resp = UniValue(UniValue::VOBJ);
            resp.pushKV("blockhash", pblockindex->GetBlockHash().GetHex());
            replier->WriteReply(HTTP_OK, resp.write() + "\n");
            return true;
        }
        default: {
            return RESTERR(replier, HTTP_NOT_FOUND, "output format not found (available: " + AvailableDataFormatsString() + ")");
        }
    }
}

static bool rest_topaddresses(const RequestContext& reqContext, const DbConnectionRef& sqliteConnection)
{
    const auto& replier = reqContext.replier;
    const auto& path = reqContext.path;
    if (!CheckWarmup(replier))
        return false;

    std::string count_str;
    const RetFormat rf = ParseDataFormat(count_str, path);
    boost::replace_all(count_str, "/", "");

    int32_t count = -1; // Initialization done only to prevent valgrind false positive, see https://github.com/bitcoin/bitcoin/pull/18785
    if (!ParseInt32(count_str, &count) || count < 0 || count > 1000) {
        return RESTERR(replier, HTTP_BAD_REQUEST, "Invalid count: " + SanitizeString(count_str));
    }

    switch (rf) {
        case RetFormat::JSON: {
            replier->WriteHeader("Content-Type", "application/json");
            auto resp = sqliteConnection->WebRpcRepoInst->GetTopAddresses(count);
            replier->WriteReply(HTTP_OK, resp.write() + "\n");
            return true;
        }
        default: {
            return RESTERR(replier, HTTP_NOT_FOUND, "output format not found (available: " + AvailableDataFormatsString() + ")");
        }
    }
}

static const struct
{
    const char* prefix;
    bool (* handler)(const RequestContext&, const DbConnectionRef& sqliteConnection);
}uri_prefixes[] = {

    {"/rest/tx/",                rest_tx},
    {"/rest/block/notxdetails/", rest_block_notxdetails},
    {"/rest/block/",             rest_block_extended},
    {"/rest/chaininfo",          rest_chaininfo},
    {"/rest/mempool/info",       rest_mempool_info},
    {"/rest/mempool/contents",   rest_mempool_contents},
    {"/rest/headers/",           rest_headers},
    {"/rest/getutxos",           rest_getutxos},
    {"/rest/blockhashbyheight/", rest_blockhash_by_height},
    {"/rest/blockhash",          rest_blockhash},
    {"/rest/emission",           rest_emission},
    {"/rest/topaddresses",       rest_topaddresses},
};

void Rest::StartREST()
{
    if (m_restRequestProcessor) {
        m_restRequestProcessor->Start();
    }
}

void Rest::StartSTATIC()
{
    if (m_staticRequestProcessor) {
        m_staticRequestProcessor->Start();
    }
}

bool Rest::Init(const ArgsManager& args, const util::Ref& context)
{
    // TODO (losty-nat): clean up
    int workQueueStaticDepth = std::max((long) args.GetArg("-rpcstaticworkqueue", DEFAULT_HTTP_STATIC_WORKQUEUE), 1L);
    int workQueueRestDepth = std::max((long) args.GetArg("-rpcrestworkqueue", DEFAULT_HTTP_REST_WORKQUEUE), 1L);
    int staticThreads = std::max((long) gArgs.GetArg("-rpcstaticthreads", DEFAULT_HTTP_STATIC_THREADS), 1L);
    int restThreads = std::max((long) gArgs.GetArg("-rpcrestthreads", DEFAULT_HTTP_REST_THREADS), 1L);

    std::vector<PathRequestHandlerEntry> pathHandlers;
    for (const auto& up : uri_prefixes) {
        auto handler = [up](const RequestContext& reqContext, const DbConnectionRef& sqliteConnection) { return up.handler(reqContext, sqliteConnection); };
        // TODO (losty-nat)
        auto pathHandler = std::make_shared<FunctionalHandler>(std::move(handler));
        pathHandlers.emplace_back(PathRequestHandlerEntry{up.prefix, false, std::move(pathHandler)});
    }

    auto restPod = std::make_shared<RequestHandlerPod>(std::move(pathHandlers), workQueueRestDepth, restThreads);
    auto handler = [](const RequestContext& reqContext, const DbConnectionRef& sqliteConnection) { return get_static_web(reqContext, sqliteConnection); };
    auto pathHandler = std::make_shared<FunctionalHandler>(std::move(handler));
    // g_staticSocket->RegisterHTTPHandler("/", false, handler, g_staticSocket->m_workQueue);
    auto staticPod = std::make_shared<RequestHandlerPod>(std::vector<PathRequestHandlerEntry>{{"/", false, pathHandler}}, workQueueStaticDepth, staticThreads);

    m_restRequestProcessor = std::make_shared<RequestProcessor>(context);
    m_staticRequestProcessor = std::make_shared<RequestProcessor>(context);

    m_restRequestProcessor->RegisterPod(std::move(restPod));
    m_staticRequestProcessor->RegisterPod(std::move(staticPod));

    return true;
}

void Rest::InterruptREST()
{
    // TODO (losty-nat): ?
}

void Rest::StopREST()
{
    if (m_restRequestProcessor) {
        m_restRequestProcessor->Stop();
    }
}

void Rest::StopSTATIC()
{
    if (m_staticRequestProcessor) {
        m_staticRequestProcessor->Stop();
    }
}