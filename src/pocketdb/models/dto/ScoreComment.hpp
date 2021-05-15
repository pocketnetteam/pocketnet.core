
#ifndef POCKETTX_SCORECOMMENT_HPP
#define POCKETTX_SCORECOMMENT_HPP

#include "pocketdb/models/base/Transaction.hpp"

namespace PocketTx
{

    class ScoreComment : public Transaction
    {
    public:

        ScoreComment(string hash, int64_t time) : Transaction(hash, time)
        {
            SetType(PocketTxType::ACTION_SCORE_COMMENT);
        }

        void Deserialize(const UniValue& src) override
        {
            Transaction::Deserialize(src);
            if (auto[ok, val] = TryGetStr(src, "commentid"); ok) SetCommentTxHash(val);
            if (auto[ok, val] = TryGetInt64(src, "value"); ok) SetValue(val);
        }

        shared_ptr<int64_t> GetCommentTxId() const { return m_int1; }
        string GetCommentTxHashStr() const { return m_comment_tx_hash ? *m_comment_tx_hash : ""; }
        void SetCommentTxId(int64_t value) { m_int1 = make_shared<int64_t>(value); }
        void SetCommentTxHash(string value) { m_comment_tx_hash = make_shared<string>(value); }

        shared_ptr<int64_t> GetValue() const { return m_int2; }
        string GetValueStr() const { return m_int2 == nullptr ? "" : std::to_string(*m_int2); }
        void SetValue(int64_t value) { m_int2 = make_shared<int64_t>(value); }

    protected:
        shared_ptr<string> m_comment_tx_hash = nullptr;

    private:

        void BuildPayload(const UniValue &src) override
        {
        }

        void BuildHash(const UniValue &src) override
        {
            std::string data;
            data += GetCommentTxHashStr();
            data += GetValueStr();
            Transaction::GenerateHash(data);
        }
    };

} // namespace PocketTx

#endif // POCKETTX_SCORECOMMENT_HPP
