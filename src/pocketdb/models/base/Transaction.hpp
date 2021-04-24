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

namespace PocketTx
{
    class Transaction : public Base
    {
    public:

        Transaction() = default;

        virtual void Deserialize(const UniValue &src)
        {
            BuildPayload(src);
            BuildHash(src);

            if (auto[ok, val] = TryGetStr(src, "txid"); ok) SetTxId(val);
            if (auto[ok, val] = TryGetInt64(src, "time"); ok) SetTxTime(val);
            if (auto[ok, val] = TryGetStr(src, "address"); ok) SetAddress(val);
        }

        string Serialize(const PocketTxType &txType)
        {
            string data;
            // TODO (brangr): implement

            return data;
        }

        bool IsValid() const
        {
            return m_txId &&
                   m_txTime &&
                   m_address;
        }


        shared_ptr<PocketTxType> GetTxType() const { return m_txType; }
        shared_ptr<int> GetTxTypeInt() const { return make_shared<int>((int) *m_txType); }
        void SetTxType(PocketTxType value) { m_txType = make_shared<PocketTxType>(value); }

        shared_ptr<string> GetTxId() const { return m_txId; }
        void SetTxId(string value) { m_txId = make_shared<string>(value); }

        shared_ptr<int64_t> GetTxTime() const { return m_txTime; }
        void SetTxTime(int64_t value) { m_txTime = make_shared<int64_t>(value); }

        shared_ptr<int64_t> GetBlock() const { return m_block; }
        void SetBlock(int64_t value) { m_block = make_shared<int64_t>(value); }

        shared_ptr<int64_t> GetTxOut() const { return m_block; }
        void SetTxOut(int64_t value) { m_txOut = make_shared<int64_t>(value); }

        shared_ptr<string> GetAddress() const { return m_address; }
        void SetAddress(string value) { m_address = make_shared<string>(value); }

        shared_ptr<int64_t> GetInt1() const { return m_int1; }
        void SetInt1(int64_t value) { m_int1 = make_shared<int64_t>(value); }

        shared_ptr<int64_t> GetInt2() const { return m_int2; }
        void SetInt2(int64_t value) { m_int2 = make_shared<int64_t>(value); }

        shared_ptr<int64_t> GetInt3() const { return m_int3; }
        void SetInt3(int64_t value) { m_int3 = make_shared<int64_t>(value); }

        shared_ptr<int64_t> GetInt4() const { return m_int4; }
        void SetInt4(int64_t value) { m_int4 = make_shared<int64_t>(value); }

        shared_ptr<int64_t> GetInt5() const { return m_int5; }
        void SetInt5(int64_t value) { m_int5 = make_shared<int64_t>(value); }


        shared_ptr<string> GetString1() const { return m_string1; }
        void SetString1(string value) { m_string1 = make_shared<string>(value); }

        shared_ptr<string> GetString2() const { return m_string2; }
        void SetString2(string value) { m_string2 = make_shared<string>(value); }

        shared_ptr<string> GetString3() const { return m_string3; }
        void SetString3(string value) { m_string3 = make_shared<string>(value); }

        shared_ptr<string> GetString4() const { return m_string4; }
        void SetString4(string value) { m_string4 = make_shared<string>(value); }

        shared_ptr<string> GetString5() const { return m_string5; }
        void SetString5(string value) { m_string5 = make_shared<string>(value); }


        shared_ptr<string> GetPayload() const { return m_payload; }
        bool HasPayload() const { return m_payload && true; };
        void SetPayload(string value) { m_payload = make_shared<string>(value); }

    protected:
        shared_ptr<string> m_hash = nullptr;

        shared_ptr<PocketTxType> m_txType = nullptr;
        shared_ptr<string> m_txId = nullptr;
        shared_ptr<int64_t> m_block = nullptr;
        shared_ptr<int64_t> m_txOut = nullptr;
        shared_ptr<int64_t> m_txTime = nullptr;
        shared_ptr<string> m_address = nullptr;

        shared_ptr<int64_t> m_int1 = nullptr;
        shared_ptr<int64_t> m_int2 = nullptr;
        shared_ptr<int64_t> m_int3 = nullptr;
        shared_ptr<int64_t> m_int4 = nullptr;
        shared_ptr<int64_t> m_int5 = nullptr;

        shared_ptr<string> m_string1 = nullptr;
        shared_ptr<string> m_string2 = nullptr;
        shared_ptr<string> m_string3 = nullptr;
        shared_ptr<string> m_string4 = nullptr;
        shared_ptr<string> m_string5;

        shared_ptr<string> m_payload = nullptr;

        virtual void BuildPayload(const UniValue &src) = 0;
        virtual void BuildHash(const UniValue &src) = 0;

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