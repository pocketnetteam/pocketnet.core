// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 Bitcoin developers
// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETTX_TRANSACTION_HPP
#define POCKETTX_TRANSACTION_HPP

#include <string>
#include <univalue/include/univalue.h>
#include <utility>
#include <utilstrencodings.h>
#include <crypto/sha256.h>

#include "pocketdb/models/base/Base.hpp"
#include "pocketdb/models/base/Payload.hpp"
#include "pocketdb/models/base/TransactionOutput.hpp"

namespace PocketTx
{
    class Transaction : public Base
    {
    public:

        //TODO mb change to string& hash (reference) and in dto too
        Transaction(string& hash, int64_t time) : Base()
        {
            SetHash(hash);
            SetTime(time);
        }

        virtual shared_ptr<UniValue> Serialize() const
        {
            auto result = make_shared<UniValue>(UniValue(UniValue::VOBJ));
            result->pushKV("time", *GetTime());
            result->pushKV("txid", *GetHash()); //TODO (brangr): check pls

            return result;
        }

        virtual void Deserialize(const UniValue& src)
        {
        }

        shared_ptr<string> GetHash() const { return m_hash; }
        void SetHash(string value) { m_hash = make_shared<string>(value); }

        shared_ptr<PocketTxType> GetType() const { return m_type; }
        shared_ptr<int> GetTypeInt() const { return make_shared<int>((int) *m_type); }
        void SetType(PocketTxType value) { m_type = make_shared<PocketTxType>(value); }

        shared_ptr<int64_t> GetTime() const { return m_time; }
        void SetTime(int64_t value) { m_time = make_shared<int64_t>(value); }

//        shared_ptr<int> GetHeight() const { return m_height; }
//        void SetHeight(int value) { m_height = make_shared<int>(value); }

        shared_ptr<bool> GetLast() const { return m_last; }
        void SetLast(bool value) { m_last = make_shared<bool>(value); }

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

        shared_ptr<int64_t> GetInt1() const { return m_int1; }
        void SetInt1(int64_t value) { m_int1 = make_shared<int64_t>(value); }

        shared_ptr<int64_t> GetId() const { return m_id; }
        void SetId(int64_t value) { m_id = make_shared<int64_t>(value); }

        vector<shared_ptr<TransactionOutput>>& Outputs() { return m_outputs; }

        shared_ptr<Payload> GetPayload() const { return m_payload; }
        void SetPayload(Payload value) { m_payload = make_shared<Payload>(value); }
        bool HasPayload() const { return m_payload != nullptr; };
        virtual void BuildPayload(const UniValue& src)
        {
            m_payload = make_shared<Payload>();
            m_payload->SetTxHash(*GetHash());
        }

        virtual void BuildHash(const UniValue& src) = 0;

    protected:
        shared_ptr<string> m_opreturn_hash = nullptr;

        shared_ptr<PocketTxType> m_type = nullptr;
        shared_ptr<string> m_hash = nullptr;
        shared_ptr<int64_t> m_time = nullptr;
//        shared_ptr<int> m_height = nullptr;
        shared_ptr<int64_t> m_id = nullptr;
        shared_ptr<bool> m_last = nullptr;

        shared_ptr<string> m_string1 = nullptr;
        shared_ptr<string> m_string2 = nullptr;
        shared_ptr<string> m_string3 = nullptr;
        shared_ptr<string> m_string4 = nullptr;
        shared_ptr<string> m_string5 = nullptr;

        shared_ptr<int64_t> m_int1 = nullptr;

        shared_ptr<Payload> m_payload = nullptr;

        vector<shared_ptr<TransactionOutput>> m_outputs;

        void GenerateHash(string& dataSrc)
        {
            unsigned char hash[32] = {};
            CSHA256().Write((const unsigned char*) dataSrc.data(), dataSrc.size()).Finalize(hash);
            CSHA256().Write(hash, 32).Finalize(hash);
            std::vector<unsigned char> vec(hash, hash + sizeof(hash));
            m_opreturn_hash = make_shared<string>(HexStr(vec));
        }

    };

} // namespace PocketTx

#endif // POCKETTX_TRANSACTION_HPP