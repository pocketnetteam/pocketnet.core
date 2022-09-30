// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0


#include "pocketdb/models/shortform/ShortAccount.h"


PocketDb::ShortAccount::ShortAccount(std::string name, std::string avatar, int64_t reputation, std::optional<std::string> lang)
    : m_name(std::move(name)),
      m_avatar(std::move(avatar)),
      m_lang(std::move(lang)),
      m_reputation(reputation)
{}

UniValue PocketDb::ShortAccount::Serialize() const
{
    UniValue data(UniValue::VOBJ);
    data.reserveKVSize(5);
    data.pushKV("n", m_name, false);
    data.pushKV("a", m_avatar, false);
    data.pushKV("b", m_badge, false);
    data.pushKV("r", m_reputation, false);
    if (m_lang) data.pushKV("l", *m_lang, false);

    return data;
}

const std::string& PocketDb::ShortAccount::GetName() const
{
    return m_name;
}

void PocketDb::ShortAccount::SetName(const std::string& name)
{
    m_name = name;
}

const std::string& PocketDb::ShortAccount::GetAvatar() const
{
    return m_avatar;
}

void PocketDb::ShortAccount::SetAvatar(const std::string& avatar)
{
    m_avatar = avatar;
}

const std::string& PocketDb::ShortAccount::GetBadge() const
{
    return m_badge;
}

void PocketDb::ShortAccount::SetBadge(const std::string& badge)
{
    m_badge = badge;
}

const int64_t& PocketDb::ShortAccount::GetReputation() const
{
    return m_reputation;
}

void PocketDb::ShortAccount::SetReputation(const int64_t& reputation)
{
    m_reputation = reputation;
}

const std::optional<std::string>& PocketDb::ShortAccount::GetLang() const
{
    return m_lang;
}

void PocketDb::ShortAccount::SetLang(const std::optional<std::string>& lang)
{
    m_lang = lang;
}
