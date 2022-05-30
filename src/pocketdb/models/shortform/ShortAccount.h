// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETDB_SHORTACCOUNT_H
#define POCKETDB_SHORTACCOUNT_H

#include <univalue.h>

#include <string>


namespace PocketDb
{
    class ShortAccount
    {
    public:
        ShortAccount(std::string name, std::string avatar, std::string badge, int64_t reputation);
        UniValue Serialize() const;
    private:
        std::string m_avatar;
        std::string m_name;
        std::string m_badge;
        int64_t m_reputation;
    };
}

#endif // POCKETDB_SHORTACCOUNT_H