// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include "pocketdb/web/WebRpcUtils.h"

namespace PocketWeb::PocketWebRpc
{
    void ParseRequestContentTypes(const UniValue& value, vector<int>& types)
    {
        if (value.isNum())
        {
            types.push_back(value.get_int());
        }
        else if (value.isStr())
        {
            if (TransactionHelper::TxIntType(value.get_str()) != TxType::NOT_SUPPORTED)
                types.push_back(TransactionHelper::TxIntType(value.get_str()));
        }
        else if (value.isArray())
        {
            const UniValue& cntntTps = value.get_array();
            for (unsigned int idx = 0; idx < cntntTps.size(); idx++)
                ParseRequestContentTypes(cntntTps[idx], types);
        }

        if (types.empty())
            types = {CONTENT_POST, CONTENT_VIDEO, CONTENT_ARTICLE, CONTENT_STREAM, CONTENT_AUDIO, BARTERON_OFFER};
    }

    void ParseRequestTags(const UniValue& value, vector<string>& tags)
    {
        if (value.isStr())
        {
            string tag = boost::trim_copy(value.get_str());
            if (!tag.empty())
            {
                auto _tag = HtmlUtils::UrlDecode(tag);
                HtmlUtils::StringToLower(_tag);
                tags.push_back(_tag);
            }
        }
        else if (value.isArray())
        {
            const UniValue& tgs = value.get_array();
            for (unsigned int idx = 0; idx < tgs.size(); idx++)
            {
                string tag = boost::trim_copy(tgs[idx].get_str());
                if (!tag.empty())
                {
                    auto _tag = HtmlUtils::UrlDecode(tag);
                    HtmlUtils::StringToLower(_tag);
                    tags.push_back(_tag);
                }

                if (tags.size() >= 10)
                    break;
            }
        }
    }

    vector<string> ParseArrayAddresses(const UniValue& value)
    {
        vector<string> addresses;

        for (unsigned int idx = 0; idx < value.size(); idx++)
        {
            const UniValue& input = value[idx];
            CTxDestination dest = DecodeDestination(input.get_str());

            if (!IsValidDestination(dest))
                throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, string("Invalid Pocketcoin address: ") + input.get_str());

            if (find(addresses.begin(), addresses.end(), input.get_str()) == addresses.end())
                addresses.push_back(input.get_str());
        }

        return addresses;
    }

    vector<string> ParseArrayHashes(const UniValue& value)
    {
        vector<string> hashes;

        for (unsigned int idx = 0; idx < value.size(); idx++)
        {
            const UniValue& input = value[idx];
            if (find(hashes.begin(), hashes.end(), input.get_str()) == hashes.end())
                hashes.push_back(input.get_str());
        }

        return hashes;
    }

    UniValue ConstructTransaction(const PTransactionRef& ptx)
    {
        // General TX information
        UniValue utx(UniValue::VOBJ);
        utx.pushKV("hash", *ptx->GetHash());
        utx.pushKV("type", *ptx->GetType());
        if (ptx->GetHeight()) utx.pushKV("height", *ptx->GetHeight());
        if (ptx->GetBlockHash()) utx.pushKV("blockHash", *ptx->GetBlockHash());
        utx.pushKV("time", *ptx->GetTime());
        
        if (ptx->GetString1()) utx.pushKV("s1", *ptx->GetString1());
        if (ptx->GetString2()) utx.pushKV("s2", *ptx->GetString2());
        if (ptx->GetString3()) utx.pushKV("s3", *ptx->GetString3());
        if (ptx->GetString4()) utx.pushKV("s4", *ptx->GetString4());
        if (ptx->GetString5()) utx.pushKV("s5", *ptx->GetString5());
        if (ptx->GetInt1()) utx.pushKV("i1", *ptx->GetInt1());

        // Inputs
        if (ptx->Inputs().size() > 0)
        {
            UniValue vin(UniValue::VARR);
            for (const auto& inp : ptx->Inputs())
            {
                UniValue uinp(UniValue::VOBJ);
                uinp.pushKV("hash", *inp.GetTxHash());
                uinp.pushKV("n", *inp.GetNumber());
                if (inp.GetAddressHash()) uinp.pushKV("addr", *inp.GetAddressHash());
                if (inp.GetValue()) uinp.pushKV("amount", *inp.GetValue());
                vin.push_back(uinp);
            }
            utx.pushKV("vin", vin);
        }

        // Outputs
        if (ptx->Outputs().size() > 0)
        {
            UniValue vout(UniValue::VARR);
            for (const auto& out : ptx->Outputs())
            {
                UniValue uout(UniValue::VOBJ);
                uout.pushKV("n", *out.GetNumber());
                uout.pushKV("amount", *out.GetValue());
                UniValue scriptPubKey(UniValue::VOBJ);
                UniValue addresses(UniValue::VARR);
                addresses.push_back(*out.GetAddressHash());
                scriptPubKey.pushKV("addrs", addresses);
                scriptPubKey.pushKV("hex", *out.GetScriptPubKey());
                uout.pushKV("script", scriptPubKey);
                if (out.GetSpentHeight()) uout.pushKV("spent", *out.GetSpentHeight());
                vout.push_back(uout);
            }

            utx.pushKV("vout", vout);
        }

        if (ptx->HasPayload())
        {
            UniValue p(UniValue::VOBJ);
            if (ptx->GetPayload()->GetString1()) p.pushKV("s1", *ptx->GetPayload()->GetString1());
            if (ptx->GetPayload()->GetString2()) p.pushKV("s2", *ptx->GetPayload()->GetString2());
            if (ptx->GetPayload()->GetString3()) p.pushKV("s3", *ptx->GetPayload()->GetString3());
            if (ptx->GetPayload()->GetString4()) p.pushKV("s4", *ptx->GetPayload()->GetString4());
            if (ptx->GetPayload()->GetString5()) p.pushKV("s5", *ptx->GetPayload()->GetString5());
            if (ptx->GetPayload()->GetString6()) p.pushKV("s6", *ptx->GetPayload()->GetString6());
            if (ptx->GetPayload()->GetString7()) p.pushKV("s7", *ptx->GetPayload()->GetString7());
            if (ptx->GetPayload()->GetInt1()) p.pushKV("i1", *ptx->GetPayload()->GetInt1());
            utx.pushKV("p", p);
        }

        return utx;
    }

    Pagination ParsePaginationArgs(UniValue& args)
    {
        Pagination pagination{ ChainActiveSafeHeight(), 0, 10, "height", true };

        if (auto arg = args.At("topHeight", true); arg.isNum())
            pagination.TopHeight = min(arg.get_int(), pagination.TopHeight);
        
        if (auto arg = args.At("pageStart", true); arg.isNum())
            pagination.PageStart = arg.get_int();

        if (auto arg = args.At("pageSize", true); arg.isNum())
            pagination.PageSize = arg.get_int();

        if (auto arg = args.At("orderBy", true); arg.isStr())
        {
            pagination.OrderBy = arg.get_str();
            HtmlUtils::StringToLower(pagination.OrderBy);
        }

        if (auto arg = args.At("orderDesc", true); arg.isBool())
            pagination.OrderDesc = arg.get_bool();

        return pagination;
    }

}