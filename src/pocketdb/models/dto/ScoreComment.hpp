
#ifndef POCKETTX_SCORECOMMENT_HPP
#define POCKETTX_SCORECOMMENT_HPP

#include "pocketdb/models/base/Transaction.hpp"

namespace PocketTx
{

    class ScoreComment : public Transaction
    {
    public:

        ScoreComment() : Transaction()
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
        void SetCommentTxId(int64_t value) { m_int1 = make_shared<int64_t>(value); }
        void SetCommentTxHash(string value) { m_comment_tx_hash = make_shared<string>(value); }

        shared_ptr<int64_t> GetValue() const { return m_int2; }
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

            if (auto[ok, val] = TryGetStr(src, "commentid"); ok) data += val;
            if (auto[ok, val] = TryGetInt64(src, "value"); ok) data += std::to_string(val);

            Transaction::GenerateHash(data);
        }
    };

} // namespace PocketTx

#endif // POCKETTX_SCORECOMMENT_HPP
