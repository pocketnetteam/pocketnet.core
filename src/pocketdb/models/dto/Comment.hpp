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
            if (auto[ok, val] = TryGetStr(src, "otxid"); ok) SetRootTxHash(val);
            if (auto[ok, val] = TryGetStr(src, "postid"); ok) SetPostTxHash(val);
            if (auto[ok, val] = TryGetStr(src, "parentid"); ok) SetParentTxHash(val);
            if (auto[ok, val] = TryGetStr(src, "answerid"); ok) SetAnswerTxHash(val);
        }

        shared_ptr <int64_t> GetRootTxId() const { return m_int1; }
        void SetRootTxId(int64_t value) { m_int1 = make_shared<int64_t>(value); }
        shared_ptr <string> GetRootTxHash() const { return m_root_tx_hash; }
        void SetRootTxHash(string value) { m_root_tx_hash = make_shared<string>(value); }

        shared_ptr <int64_t> GetPostTxId() const { return m_int2; }
        void SetPostTxId(int64_t value) { m_int2 = make_shared<int64_t>(value); }
        shared_ptr <string> GetPostTxHash() const { return m_post_tx_hash; }
        void SetPostTxHash(string value) { m_post_tx_hash = make_shared<string>(value); }

        shared_ptr <int64_t> GetParentTxId() const { return m_int3; }
        void SetParentTxId(int64_t value) { m_int3 = make_shared<int64_t>(value); }
        shared_ptr <string> GetParentTxHash() const { return m_parent_tx_hash; }
        void SetParentTxHash(string value) { m_parent_tx_hash = make_shared<string>(value); }

        shared_ptr <int64_t> GetAnswerTxId() const { return m_int4; }
        void SetAnswerTxId(int64_t value) { m_int4 = make_shared<int64_t>(value); }
        shared_ptr <string> GetAnswerTxHash() const { return m_answer_tx_hash; }
        void SetAnswerTxHash(string value) { m_answer_tx_hash = make_shared<string>(value); }

    protected:

        shared_ptr <string> m_root_tx_hash = nullptr;
        shared_ptr <string> m_post_tx_hash = nullptr;
        shared_ptr <string> m_parent_tx_hash = nullptr;
        shared_ptr <string> m_answer_tx_hash = nullptr;

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
            data += m_payload->GetString2Str();
            data += GetParentTxHash() ? *GetParentTxHash() : "";
            data += GetAnswerTxHash() ? *GetAnswerTxHash() : "";

            Transaction::GenerateHash(data);
        }
    };

} // namespace PocketTx

#endif //POCKETTX_COMMENT_HPP
