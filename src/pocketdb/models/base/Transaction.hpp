// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 Bitcoin developers
// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETTX_TRANSACTION_H
#define POCKETTX_TRANSACTION_H

#include <string>
#include <univalue.h>
#include <utility>
#include <utilstrencodings.h>
#include <crypto/sha256.h>

#include "pocketdb/models/base/Base.hpp"
#include "pocketdb/models/base/Payload.hpp"

namespace PocketTx
{
    class Transaction : public Base
    {
    public:

        Transaction() = default;

        virtual void Deserialize(const UniValue &src)
        {
            if (auto[ok, val] = TryGetStr(src, "hash"); ok) SetHash(val);
            if (auto[ok, val] = TryGetInt64(src, "time"); ok) SetTime(val);
        }


        [[nodiscard]] shared_ptr<int64_t> GetId() const { return m_id; }
        void SetId(int64_t value) { m_id = make_shared<int64_t>(value); }

        [[nodiscard]] shared_ptr<PocketTxType> GetType() const { return m_type; }
        [[nodiscard]] shared_ptr<int> GetTypeInt() const { return make_shared<int>((int) *m_type); }
        void SetType(PocketTxType value) { m_type = make_shared<PocketTxType>(value); }

        [[nodiscard]] shared_ptr<string> GetHash() const { return m_hash; }
        void SetHash(string value) { m_hash = make_shared<string>(value); }

        [[nodiscard]] shared_ptr<int64_t> GetTime() const { return m_time; }
        void SetTime(int64_t value) { m_time = make_shared<int64_t>(value); }

        [[nodiscard]] shared_ptr<int64_t> GetHeight() const { return m_height; }
        void SetHeight(int64_t value) { m_height = make_shared<int64_t>(value); }

        [[nodiscard]] shared_ptr<int64_t> GetNumber() const { return m_number; }
        void SetNumber(int64_t value) { m_number = make_shared<int64_t>(value); }

        [[nodiscard]] shared_ptr<int64_t> GetInt1() const { return m_int1; }
        void SetInt1(int64_t value) { m_int1 = make_shared<int64_t>(value); }

        [[nodiscard]] shared_ptr<int64_t> GetInt2() const { return m_int2; }
        void SetInt2(int64_t value) { m_int2 = make_shared<int64_t>(value); }

        [[nodiscard]] shared_ptr<int64_t> GetInt3() const { return m_int3; }
        void SetInt3(int64_t value) { m_int3 = make_shared<int64_t>(value); }

        [[nodiscard]] shared_ptr<int64_t> GetInt4() const { return m_int4; }
        void SetInt4(int64_t value) { m_int4 = make_shared<int64_t>(value); }


        [[nodiscard]] shared_ptr<Payload> GetPayload() const { return m_payload; }
        void SetPayload(Payload value) { m_payload = make_shared<Payload>(value); }
        [[nodiscard]] bool HasPayload() const { return m_payload != nullptr; };

        virtual void BuildPayload(const UniValue &src) = 0;
        virtual void BuildHash(const UniValue &src) = 0;

    protected:
        shared_ptr<int64_t> m_id = nullptr;
        shared_ptr<PocketTxType> m_type = nullptr;
        shared_ptr<string> m_hash = nullptr;
        shared_ptr<int64_t> m_height = nullptr;
        shared_ptr<int64_t> m_number = nullptr;
        shared_ptr<int64_t> m_time = nullptr;

        shared_ptr<int64_t> m_int1 = nullptr;
        shared_ptr<int64_t> m_int2 = nullptr;
        shared_ptr<int64_t> m_int3 = nullptr;
        shared_ptr<int64_t> m_int4 = nullptr;

        shared_ptr<Payload> m_payload = nullptr;

        void GenerateHash(string &dataSrc)
        {
            unsigned char hash[32] = {};
            CSHA256().Write((const unsigned char *) dataSrc.data(), dataSrc.size()).Finalize(hash);
            CSHA256().Write(hash, 32).Finalize(hash);
            std::vector<unsigned char> vec(hash, hash + sizeof(hash));
            m_hash = make_shared<string>(HexStr(vec));
        }

    private:

    };

} // namespace PocketTx

#endif // POCKETTX_TRANSACTION_H