// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include "pocketdb/helpers/ShortFormHelper.h"

#include <map>
#include <algorithm>


static const std::map<PocketDb::ShortTxType, std::string> typesMap = {
    { PocketDb::ShortTxType::PocketnetTeam, "pocketnetteam" },
    { PocketDb::ShortTxType::Money, "money" },
    { PocketDb::ShortTxType::Referal, "referal" },
    { PocketDb::ShortTxType::Answer, "answer" },
    { PocketDb::ShortTxType::Comment, "comment" },
    { PocketDb::ShortTxType::Subscriber, "subscriber" },
    { PocketDb::ShortTxType::CommentScore, "commentscore" },
    { PocketDb::ShortTxType::ContentScore, "contentscore" },
    { PocketDb::ShortTxType::PrivateContent, "privatecontent" },
    { PocketDb::ShortTxType::Boost, "boost" },
    { PocketDb::ShortTxType::Repost, "repost" },
};

std::string PocketHelpers::ShortTxTypeConvertor::toString(PocketDb::ShortTxType type)
{
    auto str = typesMap.find(type);
    if (str != typesMap.end()) {
        return str->second;
    }
    return "";
}

PocketDb::ShortTxType PocketHelpers::ShortTxTypeConvertor::strToType(const std::string& typeStr)
{
    auto type = std::find_if(typesMap.begin(), typesMap.end(), [&](const auto& elem) { return elem.second == typeStr; });
    if (type != typesMap.end()) {
        return type->first;
    }
    return PocketDb::ShortTxType::NotSet;
}