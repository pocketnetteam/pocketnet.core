// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include <primitives/transaction.h>
#include "pocketdb/models/dto/action/ScoreComment.h"

namespace PocketTx
{
    ScoreComment::ScoreComment() : Transaction()
    {
        SetType(TxType::ACTION_SCORE_COMMENT);
    }

    ScoreComment::ScoreComment(const std::shared_ptr<const CTransaction>& tx) : Transaction(tx)
    {
        SetType(TxType::ACTION_SCORE_COMMENT);
    }

    shared_ptr <UniValue> ScoreComment::Serialize() const
    {
        auto result = Transaction::Serialize();

        result->pushKV("address", GetAddress() ? *GetAddress() : "");
        result->pushKV("commentid", GetCommentTxHash() ? *GetCommentTxHash() : "");
        result->pushKV("value", GetValue() ? *GetValue() : 0);

        return result;
    }

    void ScoreComment::Deserialize(const UniValue& src)
    {
        Transaction::Deserialize(src);
        if (auto[ok, val] = TryGetStr(src, "address"); ok) SetAddress(val);
        if (auto[ok, val] = TryGetStr(src, "commentid"); ok) SetCommentTxHash(val);
        if (auto[ok, val] = TryGetInt64(src, "value"); ok) SetValue(val);
    }

    void ScoreComment::DeserializeRpc(const UniValue& src)
    {
        if (auto[ok, val] = TryGetStr(src, "commentid"); ok) SetCommentTxHash(val);
        if (auto[ok, val] = TryGetInt64(src, "value"); ok) SetValue(val);
    }

    shared_ptr <string> ScoreComment::GetAddress() const { return m_string1; }
    void ScoreComment::SetAddress(const string& value) { m_string1 = make_shared<string>(value); }

    shared_ptr <string> ScoreComment::GetCommentTxHash() const { return m_string2; }
    void ScoreComment::SetCommentTxHash(const string& value) { m_string2 = make_shared<string>(value); }

    shared_ptr <int64_t> ScoreComment::GetValue() const { return m_int1; }
    void ScoreComment::SetValue(int64_t value) { m_int1 = make_shared<int64_t>(value); }

    void ScoreComment::DeserializePayload(const UniValue& src)
    {
    }

    string ScoreComment::BuildHash()
    {
        std::string data;

        data += GetCommentTxHash() ? *GetCommentTxHash() : "";
        data += GetValue() ? std::to_string(*GetValue()) : "";

        return Transaction::GenerateHash(data);
    }
} // namespace PocketTx