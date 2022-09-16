// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include <primitives/transaction.h>
#include "pocketdb/models/dto/content/Comment.h"

namespace PocketTx
{
    Comment::Comment() : Transaction()
    {
        SetType(TxType::CONTENT_COMMENT);
    }

    Comment::Comment(const std::shared_ptr<const CTransaction>& tx) : Transaction(tx)
    {
        SetType(TxType::CONTENT_COMMENT);
    }

    shared_ptr <UniValue> Comment::Serialize() const
    {
        auto result = Transaction::Serialize();

        result->pushKV("address", GetAddress() ? *GetAddress() : "");
        result->pushKV("otxid", GetRootTxHash() ? *GetRootTxHash() : "");
        result->pushKV("postid", GetPostTxHash() ? *GetPostTxHash() : "");
        result->pushKV("parentid", GetParentTxHash() ? *GetParentTxHash() : "");
        result->pushKV("answerid", GetAnswerTxHash() ? *GetAnswerTxHash() : "");

        result->pushKV("msg", (m_payload && m_payload->GetString1()) ? *m_payload->GetString1() : "");

        result->pushKV("last", false);
        result->pushKV("scoreUp", 0);
        result->pushKV("scoreDown", 0);
        result->pushKV("reputation", 0);

        return result;
    }

    void Comment::Deserialize(const UniValue& src)
    {
        Transaction::Deserialize(src);
        if (auto[ok, val] = TryGetStr(src, "address"); ok) SetAddress(val);
        if (auto[ok, val] = TryGetStr(src, "postid"); ok) SetPostTxHash(val);
        if (auto[ok, val] = TryGetStr(src, "parentid"); ok) SetParentTxHash(val);
        if (auto[ok, val] = TryGetStr(src, "answerid"); ok) SetAnswerTxHash(val);

        SetRootTxHash(*GetHash());
        if (auto[ok, val] = TryGetStr(src, "otxid"); ok)
            SetRootTxHash(val);
    }

    void Comment::DeserializeRpc(const UniValue& src)
    {
        if (auto[ok, val] = TryGetStr(src, "postid"); ok) SetPostTxHash(val);
        if (auto[ok, val] = TryGetStr(src, "parentid"); ok) SetParentTxHash(val);
        if (auto[ok, val] = TryGetStr(src, "answerid"); ok) SetAnswerTxHash(val);

        SetRootTxHash(*GetHash());
        if (auto[ok, val] = TryGetStr(src, "id"); ok)
            SetRootTxHash(val);

        GeneratePayload();
        if (auto[ok, val] = TryGetStr(src, "msg"); ok) SetPayloadMsg(val);
    }

    shared_ptr <string> Comment::GetAddress() const { return m_string1; }
    void Comment::SetAddress(const string& value) { m_string1 = make_shared<string>(value); }

    shared_ptr <string> Comment::GetRootTxHash() const { return m_string2; }
    void Comment::SetRootTxHash(const string& value) { m_string2 = make_shared<string>(value); }

    shared_ptr <string> Comment::GetPostTxHash() const { return m_string3; }
    void Comment::SetPostTxHash(const string& value) { m_string3 = make_shared<string>(value); }

    shared_ptr <string> Comment::GetParentTxHash() const { return m_string4; }
    void Comment::SetParentTxHash(const string& value) { m_string4 = make_shared<string>(value); }

    shared_ptr <string> Comment::GetAnswerTxHash() const { return m_string5; }
    void Comment::SetAnswerTxHash(const string& value) { m_string5 = make_shared<string>(value); }

    shared_ptr <string> Comment::GetPayloadMsg() const { return Transaction::GetPayload()->GetString1(); }
    void Comment::SetPayloadMsg(const string& value) { Transaction::GetPayload()->SetString1(value); }

    void Comment::DeserializePayload(const UniValue& src)
    {
        Transaction::DeserializePayload(src);

        if (auto[ok, val] = TryGetStr(src, "msg"); ok) SetPayloadMsg(val);
    }

    string Comment::BuildHash()
    {
        std::string data;

        data += GetPostTxHash() ? *GetPostTxHash() : "";
        data += m_payload && m_payload->GetString1() ? *m_payload->GetString1() : "";
        data += GetParentTxHash() ? *GetParentTxHash() : "";
        data += GetAnswerTxHash() ? *GetAnswerTxHash() : "";

        return Transaction::GenerateHash(data);
    }

} // namespace PocketTx
