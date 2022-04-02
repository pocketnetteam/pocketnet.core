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
            if (TransactionHelper::TxIntType(value.get_str()) != TxType::TX_EMPTY)
                types.push_back(TransactionHelper::TxIntType(value.get_str()));
        }
        else if (value.isArray())
        {
            const UniValue& cntntTps = value.get_array();
            for (unsigned int idx = 0; idx < cntntTps.size(); idx++)
                ParseRequestContentTypes(cntntTps[idx], types);
        }

        if (types.empty())
            types = {CONTENT_POST, CONTENT_VIDEO, CONTENT_ARTICLE};
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

}