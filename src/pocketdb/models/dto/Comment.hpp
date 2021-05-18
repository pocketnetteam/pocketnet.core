// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 Bitcoin developers
// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0


#ifndef POCKETTX_COMMENT_HPP
#define POCKETTX_COMMENT_HPP

#include "pocketdb/models/base/Transaction.hpp"

namespace PocketTx
{

    class Comment : public Transaction
    {
    public:

        Comment(string hash, int64_t time) : Transaction(hash, time)
        {
            SetType(PocketTxType::CONTENT_COMMENT);
        }

        void Deserialize(const UniValue& src) override
        {
            Transaction::Deserialize(src);
            if (auto[ok, val] = TryGetStr(src, "address"); ok) SetAddress(val);
            if (auto[ok, val] = TryGetStr(src, "otxid"); ok) SetRootTxHash(val);
            if (auto[ok, val] = TryGetStr(src, "postid"); ok) SetPostTxHash(val);
            if (auto[ok, val] = TryGetStr(src, "parentid"); ok) SetParentTxHash(val);
            if (auto[ok, val] = TryGetStr(src, "answerid"); ok) SetAnswerTxHash(val);
        }

        shared_ptr<string> GetAddress() const { return m_string1; }
        void SetAddress(string value) { m_string1 = make_shared<string>(value); }

        shared_ptr <string> GetRootTxHash() const { return m_string2; }
        void SetRootTxHash(string value) { m_string2 = make_shared<string>(value); }

        shared_ptr <string> GetPostTxHash() const { return m_string3; }
        void SetPostTxHash(string value) { m_string3 = make_shared<string>(value); }

        shared_ptr <string> GetParentTxHash() const { return m_string4; }
        void SetParentTxHash(string value) { m_string4 = make_shared<string>(value); }

        shared_ptr <string> GetAnswerTxHash() const { return m_string5; }
        void SetAnswerTxHash(string value) { m_string5 = make_shared<string>(value); }

    protected:

    private:

        void BuildPayload(const UniValue& src) override
        {
            Transaction::BuildPayload(src);

            if (auto[ok, val] = TryGetStr(src, "lang"); ok) m_payload->SetString1(val);
            else m_payload->SetString1("en");

            if (auto[ok, val] = TryGetStr(src, "msg"); ok) m_payload->SetString2(val);
        }

        void BuildHash(const UniValue& src) override
        {
            std::string data;

            data += GetPostTxHash() ? *GetPostTxHash() : "";
            data += m_payload->GetString2() ? *m_payload->GetString2() : "";
            data += GetParentTxHash() ? *GetParentTxHash() : "";
            data += GetAnswerTxHash() ? *GetAnswerTxHash() : "";

            Transaction::GenerateHash(data);
        }
    };

} // namespace PocketTx

#endif //POCKETTX_COMMENT_HPP
