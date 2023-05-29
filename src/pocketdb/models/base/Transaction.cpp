// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include "pocketdb/models/base/Transaction.h"

namespace PocketTx
{

    Transaction::Transaction() : Base()
    {
    }

    Transaction::Transaction(const CTransactionRef& tx) : Base()
    {
        SetHash(tx->GetHash().GetHex());
        SetTime(tx->nTime);
    }

    optional<UniValue> Transaction::Serialize() const
    {
        // TODO (losty): optional?
        UniValue result (UniValue::VOBJ);
        
        // TODO (aok): remove safe?        
        result.pushKV("txid", *GetHash());
        result.pushKV("time", *GetTime());
        result.pushKV("block", 0);
        result.pushKV("last", false);

        return result;
    }

    void Transaction::Deserialize(const UniValue& src) {}

    void Transaction::DeserializeRpc(const UniValue& src) {}

    void Transaction::DeserializePayload(const UniValue& src)
    {
        GeneratePayload();
    }

    const optional<string>& Transaction::GetHash() const { return m_hash; }
    void Transaction::SetHash(string value) { m_hash = value; }
    bool Transaction::operator==(const string& hash) const { return m_hash && *m_hash == hash; }

    const optional<TxType>& Transaction::GetType() const { return m_type; }
    void Transaction::SetType(TxType value) { m_type = value; }

    const optional<int64_t>& Transaction::GetTime() const { return m_time; }
    void Transaction::SetTime(int64_t value) { m_time = value; }

    const optional<int64_t>& Transaction::GetHeight() const { return m_height; }
    void Transaction::SetHeight(int64_t value) { m_height = value; }

    const optional<string>& Transaction::GetBlockHash() const { return m_blockhash; }
    void Transaction::SetBlockHash(string value) { m_blockhash = value; }

    const optional<bool>& Transaction::GetLast() const { return m_last; }
    void Transaction::SetLast(bool value) { m_last = value; }

    const optional<string>& Transaction::GetString1() const { return m_string1; }
    void Transaction::SetString1(string value) { m_string1 = value; }

    const optional<string>& Transaction::GetString2() const { return m_string2; }
    void Transaction::SetString2(string value) { m_string2 = value; }

    const optional<string>& Transaction::GetString3() const { return m_string3; }
    void Transaction::SetString3(string value) { m_string3 = value; }

    const optional<string>& Transaction::GetString4() const { return m_string4; }
    void Transaction::SetString4(string value) { m_string4 = value; }

    const optional<string>& Transaction::GetString5() const { return m_string5; }
    void Transaction::SetString5(string value) { m_string5 = value; }

    const optional<int64_t>& Transaction::GetInt1() const { return m_int1; }
    void Transaction::SetInt1(int64_t value) { m_int1 = value; }

    const optional<int64_t>& Transaction::GetId() const { return m_id; }
    void Transaction::SetId(int64_t value) { m_id = value; }

    vector <TransactionInput>& Transaction::Inputs() { return m_inputs; }
    vector <TransactionOutput>& Transaction::Outputs() { return m_outputs; }
    const vector <TransactionOutput>& Transaction::OutputsConst() const { return m_outputs; }

    optional<Payload>& Transaction::GetPayload() { return m_payload; }
    const optional<Payload>& Transaction::GetPayload() const { return m_payload; }
    void Transaction::SetPayload(Payload value) { m_payload = value; }
    bool Transaction::HasPayload() const { return m_payload.has_value(); };

    string Transaction::GenerateHash(const string& data) const
    {
        unsigned char hash[32] = {};
        CSHA256().Write((const unsigned char*) data.data(), data.size()).Finalize(hash);
        CSHA256().Write(hash, 32).Finalize(hash);
        std::vector<unsigned char> vec(hash, hash + sizeof(hash));

        return HexStr(vec);
    }

    void Transaction::GeneratePayload()
    {
        m_payload = Payload();
        m_payload->SetTxHash(*GetHash());
    }

    void Transaction::ClearPayload()
    {
        m_payload = nullopt;
    }

    size_t Transaction::PayloadSize() const
    {
        return 0;
    }

} // namespace PocketTx