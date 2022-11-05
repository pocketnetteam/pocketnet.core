// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include <primitives/transaction.h>
#include "pocketdb/models/dto/action/ScoreContent.h"

namespace PocketTx
{
    ScoreContent::ScoreContent() : Transaction()
    {
        SetType(TxType::ACTION_SCORE_CONTENT);
    }

    ScoreContent::ScoreContent(const std::shared_ptr<const CTransaction>& tx) : Transaction(tx)
    {
        SetType(TxType::ACTION_SCORE_CONTENT);
    }

    shared_ptr <UniValue> ScoreContent::Serialize() const
    {
        auto result = Transaction::Serialize();

        result->pushKV("address", GetAddress() ? *GetAddress() : "");
        result->pushKV("posttxid", GetContentTxHash() ? *GetContentTxHash() : "");
        result->pushKV("value", GetValue() ? *GetValue() : 0);

        return result;
    }

    void ScoreContent::Deserialize(const UniValue& src)
    {
        Transaction::Deserialize(src);
        if (auto[ok, val] = TryGetStr(src, "address"); ok) SetAddress(val);
        if (auto[ok, val] = TryGetStr(src, "posttxid"); ok) SetContentTxHash(val);
        if (auto[ok, val] = TryGetInt64(src, "value"); ok) SetValue(val);
    }

    void ScoreContent::DeserializeRpc(const UniValue& src)
    {
        if (auto[ok, val] = TryGetStr(src, "share"); ok) SetContentTxHash(val);
        if (auto[ok, val] = TryGetInt64(src, "value"); ok) SetValue(val);
    }

    shared_ptr <string> ScoreContent::GetAddress() const { return m_string1; }
    void ScoreContent::SetAddress(const string& value) { m_string1 = make_shared<string>(value); }

    shared_ptr <string> ScoreContent::GetContentTxHash() const { return m_string2; }
    void ScoreContent::SetContentTxHash(const string& value) { m_string2 = make_shared<string>(value); }

    shared_ptr <int64_t> ScoreContent::GetValue() const { return m_int1; }
    void ScoreContent::SetValue(int64_t value) { m_int1 = make_shared<int64_t>(value); }

    void ScoreContent::DeserializePayload(const UniValue& src)
    {
    }

    string ScoreContent::BuildHash()
    {
        std::string data;

        data += GetContentTxHash() ? *GetContentTxHash() : "";
        data += GetValue() ? std::to_string(*GetValue()) : "";

        return Transaction::GenerateHash(data);
    }
} // namespace PocketTx