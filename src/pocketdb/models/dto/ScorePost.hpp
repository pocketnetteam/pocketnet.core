
#ifndef SRC_SCOREPOST_HPP
#define SRC_SCOREPOST_HPP

#include "pocketdb/models/base/Transaction.hpp"

namespace PocketTx
{

    class ScorePost : public Transaction
    {
    public:
        ~ScorePost() = default;

        ScorePost(const UniValue &src) : base(src)
        {
            SetTxType(PocketTxType::SCORE_POST_ACTION);

            assert(src.exists("value") && src["value"].isNum());
            SetValue(src["value"].get_int());

            assert(src.exists("posttxid") && src["posttxid"].isStr());
            SetPostTxId(src["posttxid"].get_str());
        }
        
        [[nodiscard]] int64_t *GetValue() const { return m_int1; }
        void SetValue(int64_t value) { m_int1 = new int64_t(value); }

        [[nodiscard]] std::string* GetPostTxId() const { return m_string1; }
        void SetPostTxId(std::string value) { m_string1 = new std::string(std::move(value)); }
    };

} // namespace PocketTx

#endif //SRC_SCOREPOST_HPP
