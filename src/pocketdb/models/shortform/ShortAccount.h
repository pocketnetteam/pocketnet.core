// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETDB_SHORTACCOUNT_H
#define POCKETDB_SHORTACCOUNT_H

#include <univalue.h>

#include <string>
#include <optional>

namespace PocketDb
{
    class ShortAccount
    {
    public:
        ShortAccount(std::string name, std::string avatar, std::string badge, int64_t reputation);
        ShortAccount() = default;
        UniValue Serialize() const;

        const std::string& GetName() const;
        void SetName(const std::string& name);
        const std::string& GetAvatar() const;
        void SetAvatar(const std::string& name);
        const std::string& GetBadge() const;
        void SetBadge(const std::string& name);
        const int64_t& GetReputation() const;
        void SetReputation(const int64_t& reputation);
        const std::optional<std::string>& GetLang() const;
        void SetLang(const std::optional<std::string>& lang);
    private:
        std::string m_avatar;
        std::string m_name;
        std::string m_badge;
        std::optional<std::string> m_lang;
        int64_t m_reputation {0};
    };
}

#endif // POCKETDB_SHORTACCOUNT_H