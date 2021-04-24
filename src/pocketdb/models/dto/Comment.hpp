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

        Comment() : Transaction()
        {
            SetTxType(PocketTxType::COMMENT_CONTENT);
        }

        void Deserialize(const UniValue& src) override {
            Transaction::Deserialize(src);
            if (auto[ok, val] = TryGetStr(src, "lang"); ok) SetLang(val);
            if (auto[ok, val] = TryGetStr(src, "otxid"); ok) SetRootTxId(val);
            if (auto[ok, val] = TryGetStr(src, "postid"); ok) SetPostTxId(val);
            if (auto[ok, val] = TryGetStr(src, "parentid"); ok) SetParentTxId(val);
            if (auto[ok, val] = TryGetStr(src, "answerid"); ok) SetAnswerTxId(val);
        }

        shared_ptr<string> GetLang() const { return m_string1; }
        void SetLang(std::string value) { m_string1 = make_shared<string>(value); }

        shared_ptr<string> GetRootTxId() const { return m_string2; }
        void SetRootTxId(std::string value) { m_string2 = make_shared<string>(value); }

        shared_ptr<string> GetPostTxId() const { return m_string3; }
        void SetPostTxId(std::string value) { m_string3 = make_shared<string>(value); }

        shared_ptr<string> GetParentTxId() const { return m_string4; }
        void SetParentTxId(std::string value) { m_string4 = make_shared<string>(value); }

        shared_ptr<string> GetAnswerTxId() const { return m_string5; }
        void SetAnswerTxId(std::string value) { m_string5 = make_shared<string>(value); }

    private:

        void BuildPayload(const UniValue &src) override
        {
            UniValue payload(UniValue::VOBJ);
            if (auto[ok, val] = TryGetStr(src, "msg"); ok) payload.pushKV("msg", val);
            SetPayload(payload);
        }

        void BuildHash(const UniValue &src) override
        {
            std::string data;
            if (auto[ok, val] = TryGetStr(src, "postid"); ok) data += val;
            if (auto[ok, val] = TryGetStr(src, "msg"); ok) data += val;
            if (auto[ok, val] = TryGetStr(src, "parentid"); ok) data += val;
            if (auto[ok, val] = TryGetStr(src, "answerid"); ok) data += val;
            Transaction::GenerateHash(data);
        }
    };

} // namespace PocketTx

#endif //POCKETTX_COMMENT_HPP
