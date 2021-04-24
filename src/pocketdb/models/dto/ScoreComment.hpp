
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
            SetTxType(PocketTxType::SCORE_COMMENT_ACTION);
        }

        void Deserialize(const UniValue& src) override
        {
            Transaction::Deserialize(src);
            if (auto[ok, val] = TryGetInt64(src, "value"); ok) SetValue(val);
            if (auto[ok, val] = TryGetStr(src, "commentid"); ok) SetCommentTxId(val);
        }

        shared_ptr<int64_t> GetValue() const { return m_int1; }
        void SetValue(int64_t value) { m_int1 = make_shared<int64_t>(value); }

        shared_ptr<string> GetCommentTxId() const { return m_string1; }
        void SetCommentTxId(string value) { m_string1 = make_shared<string>(value); }

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
