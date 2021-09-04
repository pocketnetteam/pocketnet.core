// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include <primitives/transaction.h>
#include "pocketdb/models/dto/Comment.h"

namespace PocketTx
{
    Comment::Comment() : Transaction()
    {
        SetType(PocketTxType::CONTENT_COMMENT);
    }

    Comment::Comment(const std::shared_ptr<const CTransaction>& tx) : Transaction(tx)
    {
        SetType(PocketTxType::CONTENT_COMMENT);
    }

    shared_ptr <UniValue> Comment::Serialize() const
    {
        auto result = Transaction::Serialize();

        result->pushKV("address", GetAddress() ? *GetAddress() : "");
        result->pushKV("otxid", GetRootTxHash() ? *GetRootTxHash() : "");
        result->pushKV("postid", GetPostTxHash() ? *GetPostTxHash() : "");
        result->pushKV("parentid", GetParentTxHash() ? *GetParentTxHash() : "");
        result->pushKV("answerid", GetAnswerTxHash() ? *GetAnswerTxHash() : "");

        result->pushKV("lang", (m_payload && m_payload->GetString1()) ? *m_payload->GetString1() : "en");
        result->pushKV("msg", (m_payload && m_payload->GetString2()) ? *m_payload->GetString2() : "");

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

    void Comment::DeserializeRpc(const UniValue& src, const std::shared_ptr<const CTransaction>& tx)
    {
        if (auto[ok, val] = TryGetStr(src, "txAddress"); ok) SetAddress(val);
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
    void Comment::SetAddress(string value) { m_string1 = make_shared<string>(value); }

    shared_ptr <string> Comment::GetRootTxHash() const { return m_string2; }
    void Comment::SetRootTxHash(string value) { m_string2 = make_shared<string>(value); }

    shared_ptr <string> Comment::GetPostTxHash() const { return m_string3; }
    void Comment::SetPostTxHash(string value) { m_string3 = make_shared<string>(value); }

    shared_ptr <string> Comment::GetParentTxHash() const { return m_string4; }
    void Comment::SetParentTxHash(string value) { m_string4 = make_shared<string>(value); }

    shared_ptr <string> Comment::GetAnswerTxHash() const { return m_string5; }
    void Comment::SetAnswerTxHash(string value) { m_string5 = make_shared<string>(value); }

    // Payload getters
    shared_ptr <string> Comment::GetPayloadMsg() const { return Transaction::GetPayload()->GetString1(); }
    void Comment::SetPayloadMsg(string value) { Transaction::GetPayload()->SetString1(value); }

    void Comment::DeserializePayload(const UniValue& src, const std::shared_ptr<const CTransaction>& tx)
    {
        Transaction::DeserializePayload(src, tx);

        if (auto[ok, val] = TryGetStr(src, "msg"); ok) SetPayloadMsg(val);
    }

    void Comment::BuildHash()
    {
        std::string data;

        data += GetPostTxHash() ? *GetPostTxHash() : "";
        data += m_payload->GetString1() ? *m_payload->GetString1() : "";
        data += GetParentTxHash() ? *GetParentTxHash() : "";
        data += GetAnswerTxHash() ? *GetAnswerTxHash() : "";

        Transaction::GenerateHash(data);
    }

} // namespace PocketTx
