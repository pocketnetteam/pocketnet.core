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
            SetTxType(PocketTxType::CONTENT_COMMENT);
        }

        void Deserialize(const UniValue& src) override {
            Transaction::Deserialize(src);

            if (auto[ok, val] = TryGetStr(src, "otxid"); ok) SetRootTxHash(val);
            if (auto[ok, val] = TryGetStr(src, "postid"); ok) SetPostTxHash(val);
            if (auto[ok, val] = TryGetStr(src, "parentid"); ok) SetParentTxHash(val);
            if (auto[ok, val] = TryGetStr(src, "answerid"); ok) SetAnswerTxHash(val);
        }

        shared_ptr<int64_t> GetRootTxId() const { return m_int1; }
        void SetRootTxId(int64_t value) { m_int1 = make_shared<int64_t>(value); }
        void SetRootTxHash(string value) { m_root_tx_hash = make_shared<string>(value); }

        shared_ptr<int64_t> GetPostTxId() const { return m_int2; }
        void SetPostTxId(int64_t value) { m_int2 = make_shared<int64_t>(value); }
        void SetPostTxHash(string value) { m_post_tx_hash = make_shared<string>(value); }

        shared_ptr<int64_t> GetParentTxId() const { return m_int3; }
        void SetParentTxId(int64_t value) { m_int3 = make_shared<int64_t>(value); }
        void SetParentTxHash(string value) { m_parent_tx_hash = make_shared<string>(value); }

        shared_ptr<int64_t> GetAnswerTxId() const { return m_int4; }
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
            // TODO (brangr): payload as object
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
