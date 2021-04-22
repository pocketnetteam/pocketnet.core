// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 Bitcoin developers
// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETTX_USER_HPP
#define POCKETTX_USER_HPP

#include "pocketdb/models/base/Transaction.hpp"

namespace PocketTx
{

    class User : public PocketTx::Transaction
    {
    public:
        ~User() = default;

        User(const UniValue &src) : base(src)
        {
            SetTxType(PocketTxType::USER_ACCOUNT);

            // TODO (brangr): ID должен вычислять при индексировании транзакции
            // SetId(src["id"].get_int64());

            // TODO (brangr): ID должен вычислять при индексировании транзакции
            // SetRegistration(src["regdate"].get_int64());

            if (src.exists("lang"))
                SetLang(src["lang"].get_str());

            if (src.exists("name"))
                SetName(src["name"].get_str());

            if (src.exists("referrer"))
                SetReferrer(src["referrer"].get_str());
        }


        [[nodiscard]] int64_t *GetId() const { return m_int1; }
        void SetId(int64_t id) { m_int1 = new int64_t(id); }

        [[nodiscard]] int64_t *GetRegistration() const { return m_int2; }
        void SetRegistration(int64_t value) { m_int2 = new int64_t(value); }

        [[nodiscard]] std::string *GetLang() const { return m_string1; }
        void SetLang(std::string value) { m_string1 = new std::string(std::move(value)); }

        [[nodiscard]] std::string *GetName() const { return m_string2; }
        void SetName(std::string value) { m_string2 = new std::string(std::move(value)); }

        [[nodiscard]] std::string *GetReferrer() const { return m_string3; }
        void SetReferrer(std::string value) { m_string3 = new std::string(std::move(value)); }
    };

} // namespace PocketTx

#endif // POCKETTX_USER_HPP
