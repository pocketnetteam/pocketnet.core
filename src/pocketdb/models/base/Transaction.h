// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETTX_TRANSACTION_H
#define POCKETTX_TRANSACTION_H

#include <string>
#include <univalue/include/univalue.h>
#include <utility>
#include <utilstrencodings.h>
#include <crypto/sha256.h>
#include <primitives/transaction.h>

#include "pocketdb/models/base/Payload.h"
#include "pocketdb/models/base/TransactionInput.h"
#include "pocketdb/models/base/TransactionOutput.h"

namespace PocketTx
{
    using namespace std;

    class Transaction : public Base
    {
    public:
        Transaction();
        Transaction(const CTransactionRef& tx);

        virtual shared_ptr<UniValue> Serialize() const;

        virtual void Deserialize(const UniValue& src);
        virtual void DeserializeRpc(const UniValue& src);
        virtual void DeserializePayload(const UniValue& src);

        virtual string BuildHash() = 0;

        shared_ptr<string> GetHash() const;
        void SetHash(string value);
        bool operator==(const string& hash) const;

        shared_ptr<TxType> GetType() const;
        void SetType(TxType value);

        shared_ptr<int64_t> GetTime() const;
        void SetTime(int64_t value);

        shared_ptr<int64_t> GetHeight() const;
        void SetHeight(int64_t value);

        shared_ptr<string> GetBlockHash() const;
        void SetBlockHash(string value);

        shared_ptr<bool> GetLast() const;
        void SetLast(bool value);

        shared_ptr<int64_t> GetId() const;
        void SetId(int64_t value);

        shared_ptr<string> GetString1() const;
        void SetString1(string value);

        shared_ptr<string> GetString2() const;
        void SetString2(string value);

        shared_ptr<string> GetString3() const;
        void SetString3(string value);

        shared_ptr<string> GetString4() const;
        void SetString4(string value);

        shared_ptr<string> GetString5() const;
        void SetString5(string value);

        shared_ptr<int64_t> GetInt1() const;
        void SetInt1(int64_t value);

        vector<shared_ptr<TransactionInput>>& Inputs();
        vector<shared_ptr<TransactionOutput>>& Outputs();
        const vector<shared_ptr<TransactionOutput>>& OutputsConst() const;

        shared_ptr<Payload> GetPayload() const;
        void SetPayload(Payload value);
        bool HasPayload() const;
        
        void GeneratePayload();
        void ClearPayload();

    protected:
        shared_ptr<TxType> m_type = nullptr;
        shared_ptr<string> m_hash = nullptr;
        shared_ptr<int64_t> m_time = nullptr;
        shared_ptr<int64_t> m_height = nullptr;
        shared_ptr<string> m_blockhash = nullptr;
        shared_ptr<bool> m_last = nullptr;
        shared_ptr<int64_t> m_id = nullptr;
        shared_ptr<string> m_string1 = nullptr;
        shared_ptr<string> m_string2 = nullptr;
        shared_ptr<string> m_string3 = nullptr;
        shared_ptr<string> m_string4 = nullptr;
        shared_ptr<string> m_string5 = nullptr;
        shared_ptr<int64_t> m_int1 = nullptr;
        shared_ptr<Payload> m_payload = nullptr;
        vector<shared_ptr<TransactionInput>> m_inputs;
        vector<shared_ptr<TransactionOutput>> m_outputs;

        string GenerateHash(const string& data) const;
    };

} // namespace PocketTx

#endif // POCKETTX_TRANSACTION_H