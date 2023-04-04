// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETTX_TRANSACTION_H
#define POCKETTX_TRANSACTION_H

#include <string>
#include <univalue/include/univalue.h>
#include <utility>
#include <util/strencodings.h>
#include <crypto/sha256.h>
#include <primitives/transaction.h>

#include "pocketdb/models/base/Payload.h"
#include "pocketdb/models/base/TransactionInput.h"
#include "pocketdb/models/base/TransactionOutput.h"

#include <optional>

namespace PocketTx
{
    using namespace std;

    class Transaction : public Base
    {
    public:
        Transaction();
        Transaction(const CTransactionRef& tx);

        virtual optional<UniValue> Serialize() const;

        virtual void Deserialize(const UniValue& src);
        virtual void DeserializeRpc(const UniValue& src);
        virtual void DeserializePayload(const UniValue& src);

        virtual string BuildHash() = 0;

        const optional<string>& GetHash() const;
        void SetHash(string value);
        bool operator==(const string& hash) const;

        const optional<TxType>& GetType() const;
        void SetType(TxType value);

        const optional<int64_t>& GetTime() const;
        void SetTime(int64_t value);

        const optional<int64_t>& GetHeight() const;
        void SetHeight(int64_t value);

        const optional<string>& GetBlockHash() const;
        void SetBlockHash(string value);

        const optional<bool>& GetLast() const;
        void SetLast(bool value);

        const optional<int64_t>& GetId() const;
        void SetId(int64_t value);

        const optional<string>& GetString1() const;
        void SetString1(string value);

        const optional<string>& GetString2() const;
        void SetString2(string value);

        const optional<string>& GetString3() const;
        void SetString3(string value);

        const optional<string>& GetString4() const;
        void SetString4(string value);

        const optional<string>& GetString5() const;
        void SetString5(string value);

        const optional<int64_t>& GetInt1() const;
        void SetInt1(int64_t value);

        vector<TransactionInput>& Inputs();
        vector<TransactionOutput>& Outputs();
        const vector<TransactionOutput>& OutputsConst() const;

        optional<Payload>& GetPayload();
        const optional<Payload>& GetPayload() const;
        void SetPayload(Payload value);
        bool HasPayload() const;
        
        void GeneratePayload();
        void ClearPayload();
        virtual size_t PayloadSize();

    protected:
        optional<TxType> m_type = nullopt;
        optional<string> m_hash = nullopt;
        optional<int64_t> m_time = nullopt;
        optional<int64_t> m_height = nullopt;
        optional<string> m_blockhash = nullopt;
        optional<bool> m_last = nullopt;
        optional<int64_t> m_id = nullopt;
        optional<string> m_string1 = nullopt;
        optional<string> m_string2 = nullopt;
        optional<string> m_string3 = nullopt;
        optional<string> m_string4 = nullopt;
        optional<string> m_string5 = nullopt;
        optional<int64_t> m_int1 = nullopt;
        optional<Payload> m_payload = nullopt;
        vector<TransactionInput> m_inputs;
        vector<TransactionOutput> m_outputs;

        string GenerateHash(const string& data) const;
    };

} // namespace PocketTx

#endif // POCKETTX_TRANSACTION_H