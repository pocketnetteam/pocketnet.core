
#ifndef POCKETTX_SCORECOMMENT_HPP
#define POCKETTX_SCORECOMMENT_HPP

#include "pocketdb/models/base/Transaction.hpp"

namespace PocketTx
{

    class ScoreComment : public Transaction
    {
    public:
        ~ScoreComment() = default;

        ScoreComment(const UniValue &src) : Transaction(src)
        {
            SetTxType(PocketTxType::SCORE_COMMENT_ACTION);

            assert(src.exists("value") && src["value"].isNum());
            SetValue(src["value"].get_int());

            assert(src.exists("commentid") && src["commentid"].isStr());
            SetCommentTxId(src["commentid"].get_str());
        }
        
        [[nodiscard]] int64_t *GetValue() const { return m_int1; }
        void SetValue(int64_t value) { m_int1 = new int64_t(value); }

        [[nodiscard]] std::string* GetCommentTxId() const { return m_string1; }
        void SetCommentTxId(std::string value) { m_string1 = new std::string(std::move(value)); }
    };

} // namespace PocketTx

#endif // POCKETTX_SCORECOMMENT_HPP
