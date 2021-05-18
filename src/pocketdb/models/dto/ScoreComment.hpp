
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
            if (auto[ok, val] = TryGetStr(src, "address"); ok) SetAddress(val);
            if (auto[ok, val] = TryGetStr(src, "commentid"); ok) SetCommentTxHash(val);
            if (auto[ok, val] = TryGetInt64(src, "value"); ok) SetValue(val);
        }

        shared_ptr <string> GetAddress() const { return m_string1; }
        void SetAddress(string value) { m_string1 = make_shared<string>(value); }

        shared_ptr <string> GetCommentTxHash() const { return m_string2; }
        void SetCommentTxHash(string value) { m_string2 = make_shared<string>(value); }

        shared_ptr <int64_t> GetValue() const { return m_int1; }
        void SetValue(int64_t value) { m_int1 = make_shared<int64_t>(value); }

    protected:

    private:

        void BuildPayload(const UniValue& src) override
        {
        }

        void BuildHash(const UniValue& src) override
        {
            std::string data;
            data += GetCommentTxHash() ? *GetCommentTxHash() : "";
            data += GetValue() ? std::to_string(*GetValue()) : "";
            Transaction::GenerateHash(data);
        }
    };

} // namespace PocketTx

#endif // POCKETTX_SCORECOMMENT_HPP
