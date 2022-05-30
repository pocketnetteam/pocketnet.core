// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0


#include "pocketdb/models/shortform/ShortAccount.h"


PocketDb::ShortAccount::ShortAccount(std::string name, std::string avatar, std::string badge, int64_t reputation)
    : m_name(std::move(name)),
      m_avatar(std::move(name)),
      m_badge(std::move(badge)),
      m_reputation(reputation)
{}

UniValue PocketDb::ShortAccount::Serialize() const
{
    UniValue data(UniValue::VOBJ);

    data.pushKV("name", m_name);
    data.pushKV("avatar", m_avatar);
    data.pushKV("badge", m_badge);
    data.pushKV("reputation", m_reputation);

    return data;
}