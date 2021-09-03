// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include <primitives/transaction.h>
#include "pocketdb/models/base/Transaction.h"

namespace PocketTx
{

    Transaction::Transaction(const string& hash, int64_t time) : Base()
    {
        SetHash(hash);
        SetTime(time);
    }

    Transaction::Transaction(const std::shared_ptr<const CTransaction>& tx) : Base()
    {
        SetHash(tx->GetHash().GetHex());
        SetTime(tx->nTime);
    }

    shared_ptr <UniValue> Transaction::Serialize() const
    {
        auto result = make_shared<UniValue>(UniValue(UniValue::VOBJ));
        result->pushKV("txid", *GetHash());
        result->pushKV("time", *GetTime());
        result->pushKV("block", 0);
        result->pushKV("last", false);

        return result;
    }

    void Transaction::Deserialize(const UniValue& src) {}

    void Transaction::DeserializeRpc(const UniValue& src) {}

    void Transaction::DeserializePayload(const UniValue& src)
    {
        GeneratePayload();
    }

    shared_ptr <string> Transaction::GetHash() const { return m_hash; }
    void Transaction::SetHash(string value) { m_hash = make_shared<string>(value); }
    bool Transaction::operator==(const string& hash) const { return *m_hash == hash; }

    shared_ptr <PocketTxType> Transaction::GetType() const { return m_type; }
    shared_ptr<int> Transaction::GetTypeInt() const { return make_shared<int>((int) *m_type); }
    void Transaction::SetType(PocketTxType value) { m_type = make_shared<PocketTxType>(value); }

    shared_ptr <int64_t> Transaction::GetTime() const { return m_time; }
    void Transaction::SetTime(int64_t value) { m_time = make_shared<int64_t>(value); }

    shared_ptr<bool> Transaction::GetLast() const { return m_last; }
    void Transaction::SetLast(bool value) { m_last = make_shared<bool>(value); }

    shared_ptr <string> Transaction::GetString1() const { return m_string1; }
    void Transaction::SetString1(string value) { m_string1 = make_shared<string>(value); }

    shared_ptr <string> Transaction::GetString2() const { return m_string2; }
    void Transaction::SetString2(string value) { m_string2 = make_shared<string>(value); }

    shared_ptr <string> Transaction::GetString3() const { return m_string3; }
    void Transaction::SetString3(string value) { m_string3 = make_shared<string>(value); }

    shared_ptr <string> Transaction::GetString4() const { return m_string4; }
    void Transaction::SetString4(string value) { m_string4 = make_shared<string>(value); }

    shared_ptr <string> Transaction::GetString5() const { return m_string5; }
    void Transaction::SetString5(string value) { m_string5 = make_shared<string>(value); }

    shared_ptr <int64_t> Transaction::GetInt1() const { return m_int1; }
    void Transaction::SetInt1(int64_t value) { m_int1 = make_shared<int64_t>(value); }

    shared_ptr <int64_t> Transaction::GetId() const { return m_id; }
    void Transaction::SetId(int64_t value) { m_id = make_shared<int64_t>(value); }

    vector <shared_ptr<TransactionOutput>>& Transaction::Outputs() { return m_outputs; }

    shared_ptr <Payload> Transaction::GetPayload() const { return m_payload; }
    void Transaction::SetPayload(Payload value) { m_payload = make_shared<Payload>(value); }
    bool Transaction::HasPayload() const { return m_payload != nullptr; };

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
        m_payload = make_shared<Payload>();
        m_payload->SetTxHash(*GetHash());
    }

    void Transaction::ClearPayload()
    {
        m_payload = nullptr;
    }

} // namespace PocketTx