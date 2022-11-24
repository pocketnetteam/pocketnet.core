// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chain.h>
#include <chainparams.h>
#include <core_io.h>
#include <httpserver.h>
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

#include "pocketdb/services/ChainPostProcessing.h"
#include "pocketdb/consensus/Helper.h"
#include "pocketdb/services/Accessor.h"
#include "pocketdb/web/PocketFrontend.h"

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

static bool RESTERR(HTTPRequest* req, enum HTTPStatusCode status, std::string message)
{
    req->WriteHeader("Content-Type", "text/plain");
    req->WriteReply(status, message + "\r\n");
    return false;
}

/**
 * Get the node context.
 *
 * @param[in]  req  The HTTP request, whose status code will be set if node
 *                  context is not found.
 * @returns         Pointer to the node context or nullptr if not found.
 */
static NodeContext* GetNodeContext(const util::Ref& context, HTTPRequest* req)
{
    NodeContext* node = context.Has<NodeContext>() ? &context.Get<NodeContext>() : nullptr;
    if (!node) {
        RESTERR(req, HTTP_INTERNAL_SERVER_ERROR,
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
static CTxMemPool* GetMemPool(const util::Ref& context, HTTPRequest* req)
{
    NodeContext* node = context.Has<NodeContext>() ? &context.Get<NodeContext>() : nullptr;
    if (!node || !node->mempool) {
        RESTERR(req, HTTP_NOT_FOUND, "Mempool disabled or instance not found");
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

static bool CheckWarmup(HTTPRequest* req)
{
    std::string statusmessage;
    if (RPCIsInWarmup(&statusmessage))
        return RESTERR(req, HTTP_SERVICE_UNAVAILABLE, "Service temporarily unavailable: " + statusmessage);
    return true;
}

static bool rest_headers(const util::Ref& context,
    HTTPRequest* req,
    const std::string& strURIPart)
{
    if (!CheckWarmup(req))
        return false;
    std::string param;
    const RetFormat rf = ParseDataFormat(param, strURIPart);
    std::vector <std::string> path;
    boost::split(path, param, boost::is_any_of("/"));

    if (path.size() != 2)
        return RESTERR(req, HTTP_BAD_REQUEST, "No header count specified. Use /rest/headers/<count>/<hash>.<ext>.");

    long count = strtol(path[0].c_str(), nullptr, 10);
    if (count < 1 || count > 2000)
        return RESTERR(req, HTTP_BAD_REQUEST, "Header count out of range: " + path[0]);

    std::string hashStr = path[1];
    uint256 hash;
    if (!ParseHashStr(hashStr, hash))
        return RESTERR(req, HTTP_BAD_REQUEST, "Invalid hash: " + hashStr);

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
            req->WriteHeader("Content-Type", "application/octet-stream");
            req->WriteReply(HTTP_OK, binaryHeader);
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
            req->WriteHeader("Content-Type", "text/plain");
            req->WriteReply(HTTP_OK, strHex);
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
            req->WriteHeader("Content-Type", "application/json");
            req->WriteReply(HTTP_OK, strJSON);
            return true;
        }
        default:
        {
            return RESTERR(req, HTTP_NOT_FOUND, "output format not found (available: .bin, .hex, .json)");
        }
    }
}

static bool rest_block(HTTPRequest* req,
    const std::string& strURIPart,
    bool showTxDetails)
{
    if (!CheckWarmup(req))
        return false;
    std::string hashStr;
    const RetFormat rf = ParseDataFormat(hashStr, strURIPart);

    uint256 hash;
    if (!ParseHashStr(hashStr, hash))
        return RESTERR(req, HTTP_BAD_REQUEST, "Invalid hash: " + hashStr);

    CBlock block;
    CBlockIndex* pblockindex = nullptr;
    CBlockIndex* tip = nullptr;
    {
        LOCK(cs_main);
        tip = ::ChainActive().Tip();
        pblockindex = LookupBlockIndex(hash);
        if (!pblockindex) {
            return RESTERR(req, HTTP_NOT_FOUND, hashStr + " not found");
        }

        if (IsBlockPruned(pblockindex))
            return RESTERR(req, HTTP_NOT_FOUND, hashStr + " not available (pruned data)");

        if (!ReadBlockFromDisk(block, pblockindex, Params().GetConsensus()))
            return RESTERR(req, HTTP_NOT_FOUND, hashStr + " not found");
    }

    switch (rf)
    {
        case RetFormat::BINARY:
        {
            CDataStream ssBlock(SER_NETWORK, PROTOCOL_VERSION | RPCSerializationFlags());
            ssBlock << block;
            std::string binaryBlock = ssBlock.str();
            req->WriteHeader("Content-Type", "application/octet-stream");
            req->WriteReply(HTTP_OK, binaryBlock);
            return true;
        }

        case RetFormat::HEX:
        {
            CDataStream ssBlock(SER_NETWORK, PROTOCOL_VERSION | RPCSerializationFlags());
            ssBlock << block;
            std::string strHex = HexStr(ssBlock) + "\n";
            req->WriteHeader("Content-Type", "text/plain");
            req->WriteReply(HTTP_OK, strHex);
            return true;
        }

        case RetFormat::JSON:
        {
            UniValue objBlock = blockToJSON(block, tip, pblockindex, showTxDetails);
            std::string strJSON = objBlock.write() + "\n";
            req->WriteHeader("Content-Type", "application/json");
            req->WriteReply(HTTP_OK, strJSON);
            return true;
        }

        default:
        {
            return RESTERR(req, HTTP_NOT_FOUND,
                "output format not found (available: " + AvailableDataFormatsString() + ")");
        }
    }
}

static bool rest_blockhash(const util::Ref& context, HTTPRequest* req, const std::string& strURIPart)
{
    if (!CheckWarmup(req))
        return false;

    auto[rf, uriParts] = ParseParams(strURIPart);

    int height = ChainActive().Height();
    if (auto[ok, value] = TryGetParamInt(uriParts, 0); ok)
        height = value;

    if (height < 0 || height > ChainActive().Height())
        return RESTERR(req, HTTP_BAD_REQUEST, "Block height out of range");

    CBlockIndex* pblockindex = ChainActive()[height];
    std::string blockHash = pblockindex->GetBlockHash().GetHex();

    switch (rf)
    {
        case RetFormat::JSON:
        {
            UniValue result(UniValue::VOBJ);
            result.pushKV("height", height);
            result.pushKV("blockhash", blockHash);

            req->WriteHeader("Content-Type", "application/json");
            req->WriteReply(HTTP_OK, result.write() + "\n");
            return true;
        }
        default:
        {
            req->WriteHeader("Content-Type", "text/plain");
            req->WriteReply(HTTP_OK, blockHash + "\n");
            return true;
        }
    }
}

static bool rest_block_extended(const util::Ref& context, HTTPRequest* req, const std::string& strURIPart)
{
    return rest_block(req, strURIPart, true);
}

static bool rest_block_notxdetails(const util::Ref& context, HTTPRequest* req, const std::string& strURIPart)
{
    return rest_block(req, strURIPart, false);
}

// A bit of a hack - dependency on a function defined in rpc/blockchain.cpp
RPCHelpMan getblockchaininfo();

static bool rest_chaininfo(const util::Ref& context, HTTPRequest* req, const std::string& strURIPart)
{
    if (!CheckWarmup(req))
        return false;
    std::string param;
    const RetFormat rf = ParseDataFormat(param, strURIPart);

    switch (rf)
    {
        case RetFormat::JSON:
        {
            JSONRPCRequest jsonRequest(context);
            jsonRequest.params = UniValue(UniValue::VARR);
            UniValue chainInfoObject = getblockchaininfo().HandleRequest(jsonRequest);
            std::string strJSON = chainInfoObject.write() + "\n";
            req->WriteHeader("Content-Type", "application/json");
            req->WriteReply(HTTP_OK, strJSON);
            return true;
        }
        default:
        {
            return RESTERR(req, HTTP_NOT_FOUND, "output format not found (available: json)");
        }
    }
}

static bool rest_mempool_info(const util::Ref& context, HTTPRequest* req, const std::string& strURIPart)
{
    if (!CheckWarmup(req))
        return false;
    const CTxMemPool* mempool = GetMemPool(context, req);
    if (!mempool) return false;
    std::string param;
    const RetFormat rf = ParseDataFormat(param, strURIPart);

    switch (rf)
    {
        case RetFormat::JSON:
        {
            UniValue mempoolInfoObject = mempoolInfoToJSON(*mempool);

            std::string strJSON = mempoolInfoObject.write() + "\n";
            req->WriteHeader("Content-Type", "application/json");
            req->WriteReply(HTTP_OK, strJSON);
            return true;
        }
        default:
        {
            return RESTERR(req, HTTP_NOT_FOUND, "output format not found (available: json)");
        }
    }
}

static bool rest_mempool_contents(const util::Ref& context, HTTPRequest* req, const std::string& strURIPart)
{
    if (!CheckWarmup(req))
        return false;
    const CTxMemPool* mempool = GetMemPool(context, req);
    if (!mempool) return false;
    std::string param;
    const RetFormat rf = ParseDataFormat(param, strURIPart);

    switch (rf)
    {
        case RetFormat::JSON:
        {
            UniValue mempoolObject = MempoolToJSON(*mempool, true);

            std::string strJSON = mempoolObject.write() + "\n";
            req->WriteHeader("Content-Type", "application/json");
            req->WriteReply(HTTP_OK, strJSON);
            return true;
        }
        default:
        {
            return RESTERR(req, HTTP_NOT_FOUND, "output format not found (available: json)");
        }
    }
}

static bool rest_tx(const util::Ref& context, HTTPRequest* req, const std::string& strURIPart)
{
    if (!CheckWarmup(req))
        return false;
    std::string hashStr;
    const RetFormat rf = ParseDataFormat(hashStr, strURIPart);

    uint256 hash;
    if (!ParseHashStr(hashStr, hash))
        return RESTERR(req, HTTP_BAD_REQUEST, "Invalid hash: " + hashStr);

    if (g_txindex)
    {
        g_txindex->BlockUntilSyncedToCurrentChain();
    }

    const NodeContext* const node = GetNodeContext(context, req);
    if (!node) return false;
    uint256 hashBlock = uint256();
    const CTransactionRef tx = GetTransaction(/* block_index */ nullptr, node->mempool.get(), hash, Params().GetConsensus(), hashBlock);
    if (!tx) {
        return RESTERR(req, HTTP_NOT_FOUND, hashStr + " not found");
    }

    switch (rf)
    {
        case RetFormat::BINARY:
        {
            CDataStream ssTx(SER_NETWORK, PROTOCOL_VERSION | RPCSerializationFlags());
            ssTx << tx;

            std::string binaryTx = ssTx.str();
            req->WriteHeader("Content-Type", "application/octet-stream");
            req->WriteReply(HTTP_OK, binaryTx);
            return true;
        }

        case RetFormat::HEX:
        {
            CDataStream ssTx(SER_NETWORK, PROTOCOL_VERSION | RPCSerializationFlags());
            ssTx << tx;

            std::string strHex = HexStr(ssTx) + "\n";
            req->WriteHeader("Content-Type", "text/plain");
            req->WriteReply(HTTP_OK, strHex);
            return true;
        }

        case RetFormat::JSON:
        {
            UniValue objTx(UniValue::VOBJ);
            TxToUniv(*tx, hashBlock, objTx);
            std::string strJSON = objTx.write() + "\n";
            req->WriteHeader("Content-Type", "application/json");
            req->WriteReply(HTTP_OK, strJSON);
            return true;
        }

        default:
        {
            return RESTERR(req, HTTP_NOT_FOUND,
                "output format not found (available: " + AvailableDataFormatsString() + ")");
        }
    }
}

static bool rest_getutxos(const util::Ref& context, HTTPRequest* req, const std::string& strURIPart)
{
    if (!CheckWarmup(req))
        return false;
    std::string param;
    const RetFormat rf = ParseDataFormat(param, strURIPart);

    std::vector <std::string> uriParts;
    if (param.length() > 1)
    {
        std::string strUriParams = param.substr(1);
        boost::split(uriParts, strUriParams, boost::is_any_of("/"));
    }

    // throw exception in case of an empty request
    std::string strRequestMutable = req->ReadBody();
    if (strRequestMutable.length() == 0 && uriParts.size() == 0)
        return RESTERR(req, HTTP_BAD_REQUEST, "Error: empty request");

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
                return RESTERR(req, HTTP_BAD_REQUEST, "Parse error");

            txid.SetHex(strTxid);
            vOutPoints.push_back(COutPoint(txid, (uint32_t) nOutput));
        }

        if (vOutPoints.size() > 0)
            fInputParsed = true;
        else
            return RESTERR(req, HTTP_BAD_REQUEST, "Error: empty request");
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
                        return RESTERR(req, HTTP_BAD_REQUEST,
                            "Combination of URI scheme inputs and raw post data is not allowed");

                    CDataStream oss(SER_NETWORK, PROTOCOL_VERSION);
                    oss << strRequestMutable;
                    oss >> fCheckMemPool;
                    oss >> vOutPoints;
                }
            } catch (const std::ios_base::failure&)
            {
                // abort in case of unreadable binary data
                return RESTERR(req, HTTP_BAD_REQUEST, "Parse error");
            }
            break;
        }

        case RetFormat::JSON:
        {
            if (!fInputParsed)
                return RESTERR(req, HTTP_BAD_REQUEST, "Error: empty request");
            break;
        }
        default:
        {
            return RESTERR(req, HTTP_NOT_FOUND,
                "output format not found (available: " + AvailableDataFormatsString() + ")");
        }
    }

    // limit max outpoints
    if (vOutPoints.size() > MAX_GETUTXOS_OUTPOINTS)
        return RESTERR(req, HTTP_BAD_REQUEST,
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
            const CTxMemPool* mempool = GetMemPool(context, req);
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

            req->WriteHeader("Content-Type", "application/octet-stream");
            req->WriteReply(HTTP_OK, ssGetUTXOResponseString);
            return true;
        }

        case RetFormat::HEX:
        {
            CDataStream ssGetUTXOResponse(SER_NETWORK, PROTOCOL_VERSION);
            ssGetUTXOResponse << ::ChainActive().Height() << ::ChainActive().Tip()->GetBlockHash() << bitmap << outs;
            std::string strHex = HexStr(ssGetUTXOResponse) + "\n";

            req->WriteHeader("Content-Type", "text/plain");
            req->WriteReply(HTTP_OK, strHex);
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
            req->WriteHeader("Content-Type", "application/json");
            req->WriteReply(HTTP_OK, strJSON);
            return true;
        }
        default:
        {
            return RESTERR(req, HTTP_NOT_FOUND,
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

static bool rest_emission(const util::Ref& context, HTTPRequest* req, const std::string& strURIPart)
{
    if (!CheckWarmup(req))
        return false;

    auto[rf, uriParts] = ParseParams(strURIPart);
    
    int height = ChainActive().Height();
    if (auto[ok, value] = TryGetParamInt(uriParts, 0); ok)
        height = std::max(value, 0);

    req->WriteHeader("Content-Type", "text/plain");
    req->WriteReply(HTTP_OK, std::to_string(getEmission(height)));
    return true;
}

static bool get_static_status(HTTPRequest* req, const std::string& strURIPart)
{
    if (!CheckWarmup(req))
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

    req->WriteHeader("Content-Type", "application/json");
    req->WriteReply(HTTP_OK, entry.write() + "\n");
    return true;
}

static bool debug_index_block(const util::Ref& context, HTTPRequest* req, const std::string& strURIPart)
{
    if (!CheckWarmup(req))
        return false;

    auto& node = EnsureNodeContext(context);
    auto[rf, uriParts] = ParseParams(strURIPart);
    int start = 0;
    int height = 1;

    if (auto[ok, result] = TryGetParamInt(uriParts, 0); ok)
        start = result;

    if (auto[ok, result] = TryGetParamInt(uriParts, 1); ok)
        height = result;

    if (start == 0)
    {
        PocketDb::SQLiteDbInst.DropIndexes();
        PocketDb::ChainRepoInst.ClearDatabase();
        PocketDb::SQLiteDbInst.CreateStructure();
    }

    int current = start;
    while (current <= height)
    {
        CBlockIndex* pblockindex = ChainActive()[current];
        if (!pblockindex)
            return RESTERR(req, HTTP_BAD_REQUEST, "Block height out of range");

        CBlock block;
        if (!ReadBlockFromDisk(block, pblockindex, Params().GetConsensus()))
            return RESTERR(req, HTTP_BAD_REQUEST, "Block not found on disk");

        try
        {
            std::shared_ptr <PocketHelpers::PocketBlock> pocketBlock = nullptr;
            if (!PocketServices::Accessor::GetBlock(block, pocketBlock) || !pocketBlock)
                return RESTERR(req, HTTP_BAD_REQUEST, "Block not found on sqlite db");

            PocketServices::ChainPostProcessing::Rollback(pblockindex->nHeight);

            CDataStream hashProofOfStakeSource(SER_GETHASH, 0);
            if (pblockindex->nHeight > 100000 && block.IsProofOfStake())
            {
                arith_uint256 hashProof;
                arith_uint256 targetProofOfStake;
                CHECK_NONFATAL(node.mempool);
                CheckProofOfStake(pblockindex->pprev, block.vtx[1], block.nBits, hashProof, hashProofOfStakeSource,
                    targetProofOfStake, NULL, *node.mempool, false);

                int64_t nReward = GetProofOfStakeReward(pblockindex->nHeight, 0, Params().GetConsensus());
                if (!CheckBlockRatingRewards(block, pblockindex->pprev, nReward, hashProofOfStakeSource))
                {
                    LogPrintf("CheckBlockRatingRewards at height %d failed\n", current);
                    return RESTERR(req, HTTP_BAD_REQUEST, "CheckBlockRatingRewards failed");
                }
            }

            if (auto[ok, result] = PocketConsensus::SocialConsensusHelper::Validate(block, pocketBlock, pblockindex->nHeight); !ok)
            {
                LogPrintf("failed at %d heaihgt with result %d\n", pblockindex->nHeight, (int)result);
                // return RESTERR(req, HTTP_BAD_REQUEST, "Validate failed");
            }

            PocketServices::ChainPostProcessing::Index(block, pblockindex->nHeight);
        }
        catch (std::exception& ex)
        {
            return RESTERR(req, HTTP_BAD_REQUEST, "TransactionPostProcessing::Index ended with result: ");
        }

        LogPrintf("TransactionPostProcessing::Index at height %d\n", current);
        current += 1;
    }

    req->WriteHeader("Content-Type", "text/plain");
    req->WriteReply(HTTP_OK, "Success\n");
    return true;
}

static bool debug_check_block(const util::Ref& context, HTTPRequest* req, const std::string& strURIPart)
{
    if (!CheckWarmup(req))
        return false;

    auto[rf, uriParts] = ParseParams(strURIPart);
    int start = 0;
    int height = 1;

    if (auto[ok, result] = TryGetParamInt(uriParts, 0); ok)
        start = result;

    if (auto[ok, result] = TryGetParamInt(uriParts, 1); ok)
        height = result;

    int current = start;
    while (current <= height)
    {
        CBlockIndex* pblockindex = ::ChainActive()[current];
        if (!pblockindex)
            return RESTERR(req, HTTP_BAD_REQUEST, "Block height out of range");

        CBlock block;
        if (!ReadBlockFromDisk(block, pblockindex, Params().GetConsensus()))
            return RESTERR(req, HTTP_BAD_REQUEST, "Block not found on disk");

        std::shared_ptr<PocketHelpers::PocketBlock> pocketBlock = nullptr;
        if (!PocketServices::Accessor::GetBlock(block, pocketBlock) || !pocketBlock)
            return RESTERR(req, HTTP_BAD_REQUEST, "Block not found on sqlite db");

        if (auto[ok, result] = PocketConsensus::SocialConsensusHelper::Check(block, pocketBlock, ::ChainActive().Height()); ok)
        {
            LogPrintf("SocialConsensusHelper::Check at height %d - SUCCESS\n", current);
            current += 1;
        }
        else
        {
            LogPrintf("SocialConsensusHelper::Check at height %d - FAILED\n", current);
            return false;
        }
    }

    req->WriteHeader("Content-Type", "text/plain");
    req->WriteReply(HTTP_OK, "Success\n");
    return true;
}

static bool get_static_web(const util::Ref& context, HTTPRequest* req, const std::string& strURIPart)
{
    if (!CheckWarmup(req))
        return false;

    if (strURIPart.find("clear") == 0)
    {
        PocketWeb::PocketFrontendInst.ClearCache();
        req->WriteReply(HTTP_OK, "Cache cleared!");
        return true;
    }

    if (strURIPart.find("status") == 0)
        return get_static_status(req, strURIPart);

    if (auto[code, file] = PocketWeb::PocketFrontendInst.GetFile(strURIPart); code == HTTP_OK)
    {
        req->WriteHeader("Content-Type", file->ContentType);
        req->WriteReply(code, file->Content);
        return true;
    }
    else
    {
        return RESTERR(req, code, "");
    }

    return RESTERR(req, HTTP_NOT_FOUND, "");
}



static bool rest_blockhash_by_height(const util::Ref& context, HTTPRequest* req,
                       const std::string& str_uri_part)
{
    if (!CheckWarmup(req)) return false;
    std::string height_str;
    const RetFormat rf = ParseDataFormat(height_str, str_uri_part);

    int32_t blockheight = -1; // Initialization done only to prevent valgrind false positive, see https://github.com/bitcoin/bitcoin/pull/18785
    if (!ParseInt32(height_str, &blockheight) || blockheight < 0) {
        return RESTERR(req, HTTP_BAD_REQUEST, "Invalid height: " + SanitizeString(height_str));
    }

    CBlockIndex* pblockindex = nullptr;
    {
        LOCK(cs_main);
        if (blockheight > ::ChainActive().Height()) {
            return RESTERR(req, HTTP_NOT_FOUND, "Block height out of range");
        }
        pblockindex = ::ChainActive()[blockheight];
    }
    switch (rf) {
    case RetFormat::BINARY: {
        CDataStream ss_blockhash(SER_NETWORK, PROTOCOL_VERSION);
        ss_blockhash << pblockindex->GetBlockHash();
        req->WriteHeader("Content-Type", "application/octet-stream");
        req->WriteReply(HTTP_OK, ss_blockhash.str());
        return true;
    }
    case RetFormat::HEX: {
        req->WriteHeader("Content-Type", "text/plain");
        req->WriteReply(HTTP_OK, pblockindex->GetBlockHash().GetHex() + "\n");
        return true;
    }
    case RetFormat::JSON: {
        req->WriteHeader("Content-Type", "application/json");
        UniValue resp = UniValue(UniValue::VOBJ);
        resp.pushKV("blockhash", pblockindex->GetBlockHash().GetHex());
        req->WriteReply(HTTP_OK, resp.write() + "\n");
        return true;
    }
    default: {
        return RESTERR(req, HTTP_NOT_FOUND, "output format not found (available: " + AvailableDataFormatsString() + ")");
    }
    }
}

static const struct
{
    const char* prefix;
    bool (* handler)(const util::Ref& context, HTTPRequest* req, const std::string& strReq);
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
};

void StartREST(const util::Ref& context)
{
    if(g_restSocket) {
        for (const auto& up : uri_prefixes) {
            auto handler = [&context, up](HTTPRequest* req, const std::string& prefix) { return up.handler(context, req, prefix); };
            g_restSocket->RegisterHTTPHandler(up.prefix, false, handler, g_restSocket->m_workQueue);
        }
    }
}

// Register web content route
void StartSTATIC(const util::Ref& context)
{
    if(g_staticSocket)
    {
        auto handler = [&context](HTTPRequest* req, const std::string& prefix) { return get_static_web(context, req, prefix); };
        g_staticSocket->RegisterHTTPHandler("/", false, handler, g_staticSocket->m_workQueue);
    }
}

void InterruptREST()
{
}

void StopREST()
{
    if (g_restSocket)
        for (auto uri_prefixe: uri_prefixes)
            g_restSocket->UnregisterHTTPHandler(uri_prefixe.prefix, false);    
}

void StopSTATIC()
{
    if (g_staticSocket)
        g_staticSocket->UnregisterHTTPHandler("/", false);
}
