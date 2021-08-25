// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 Bitcoin developers
// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0


#ifndef POCKETTX_COMMENT_HPP
#define POCKETTX_COMMENT_HPP

#include "pocketdb/models/base/Transaction.h"

namespace PocketTx
{

    class Comment : public Transaction
    {
    public:

        Comment(const string& hash, int64_t time) : Transaction(hash, time)
        {
            SetType(PocketTxType::CONTENT_COMMENT);
        }

        shared_ptr <UniValue> Serialize() const override
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

        void Deserialize(const UniValue& src) override
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
        
        void DeserializeRpc(const UniValue& src) override
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

        shared_ptr <string> GetAddress() const { return m_string1; }
        void SetAddress(string value) { m_string1 = make_shared<string>(value); }

        shared_ptr <string> GetRootTxHash() const { return m_string2; }
        void SetRootTxHash(string value) { m_string2 = make_shared<string>(value); }

        shared_ptr <string> GetPostTxHash() const { return m_string3; }
        void SetPostTxHash(string value) { m_string3 = make_shared<string>(value); }

        shared_ptr <string> GetParentTxHash() const { return m_string4; }
        void SetParentTxHash(string value) { m_string4 = make_shared<string>(value); }

        shared_ptr <string> GetAnswerTxHash() const { return m_string5; }
        void SetAnswerTxHash(string value) { m_string5 = make_shared<string>(value); }

        // Payload getters
        shared_ptr <string> GetPayloadMsg() const { return Transaction::GetPayload()->GetString1(); }
        void SetPayloadMsg(string value) { Transaction::GetPayload()->SetString1(value); }
        
    protected:

        void DeserializePayload(const UniValue& src) override
        {
            Transaction::DeserializePayload(src);

            if (auto[ok, val] = TryGetStr(src, "msg"); ok) SetPayloadMsg(val);
        }

        void BuildHash() override
        {
            std::string data;

            data += GetPostTxHash() ? *GetPostTxHash() : "";
            data += m_payload->GetString1() ? *m_payload->GetString1() : "";
            data += GetParentTxHash() ? *GetParentTxHash() : "";
            data += GetAnswerTxHash() ? *GetAnswerTxHash() : "";

            Transaction::GenerateHash(data);
        }

    };

} // namespace PocketTx

#endif //POCKETTX_COMMENT_HPP
