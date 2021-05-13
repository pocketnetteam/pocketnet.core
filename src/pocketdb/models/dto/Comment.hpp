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
            SetType(PocketTxType::CONTENT_COMMENT);
        }

        void Deserialize(const UniValue& src) override {
            Transaction::Deserialize(src);

            if (auto[ok, val] = TryGetStr(src, "otxid"); ok) SetRootTxHash(val);
            if (auto[ok, val] = TryGetStr(src, "postid"); ok) SetPostTxHash(val);
            if (auto[ok, val] = TryGetStr(src, "parentid"); ok) SetParentTxHash(val);
            if (auto[ok, val] = TryGetStr(src, "answerid"); ok) SetAnswerTxHash(val);
        }

        shared_ptr<int64_t> GetRootTxId() const { return m_int1; }
        string GetRootTxHashStr() const { return m_root_tx_hash == nullptr ? "" : *m_root_tx_hash; }
        void SetRootTxId(int64_t value) { m_int1 = make_shared<int64_t>(value); }
        void SetRootTxHash(string value) { m_root_tx_hash = make_shared<string>(value); }

        shared_ptr<int64_t> GetPostTxId() const { return m_int2; }
        string GetPostTxHashStr() const { return m_post_tx_hash == nullptr ? "" : *m_post_tx_hash; }
        void SetPostTxId(int64_t value) { m_int2 = make_shared<int64_t>(value); }
        void SetPostTxHash(string value) { m_post_tx_hash = make_shared<string>(value); }

        shared_ptr<int64_t> GetParentTxId() const { return m_int3; }
        string GetParentTxHashStr() const { return m_parent_tx_hash == nullptr ? "" : *m_parent_tx_hash; }
        void SetParentTxId(int64_t value) { m_int3 = make_shared<int64_t>(value); }
        void SetParentTxHash(string value) { m_parent_tx_hash = make_shared<string>(value); }

        shared_ptr<int64_t> GetAnswerTxId() const { return m_int4; }
        string GetAnswerTxHashStr() const { return m_answer_tx_hash == nullptr ? "" : *m_answer_tx_hash; }
        void SetAnswerTxId(int64_t value) { m_int4 = make_shared<int64_t>(value); }
        void SetAnswerTxHash(string value) { m_answer_tx_hash = make_shared<string>(value); }

    protected:

        shared_ptr<string> m_root_tx_hash = nullptr;
        shared_ptr<string> m_post_tx_hash = nullptr;
        shared_ptr<string> m_parent_tx_hash = nullptr;
        shared_ptr<string> m_answer_tx_hash = nullptr;

    private:

        void BuildPayload(const UniValue &src) override
        {
            Transaction::BuildPayload(src);

            if (auto[ok, val] = TryGetStr(src, "lang"); ok) m_payload->SetString1(val);
            else m_payload->SetString1("en");

            if (auto[ok, val] = TryGetStr(src, "msg"); ok) m_payload->SetString2(val);
        }

        void BuildHash(const UniValue &src) override
        {
            std::string data;

            data += GetPostTxHashStr();
            data += m_payload->GetString2Str();
            data += GetParentTxHashStr();
            data += GetAnswerTxHashStr();

            Transaction::GenerateHash(data);
        }
    };

} // namespace PocketTx

#endif //POCKETTX_COMMENT_HPP
