
#ifndef SRC_SCOREPOST_HPP
#define SRC_SCOREPOST_HPP

#include "pocketdb/models/base/Transaction.hpp"

namespace PocketTx
{

    class ScorePost : public Transaction
    {
    public:
        ~ScorePost() = default;

        ScorePost()
        {
            SetTxType(PocketTxType::SCORE_POST_ACTION);
        }

        void Deserialize(const UniValue &src) override
        {
        }
//
//    [[nodiscard]] std::string* GetLang() const { return m_string1; }
//    1void SetLang(std::string value) { m_string1 = new std::string(std::move(value)); }
//
//    [[nodiscard]] std::string* GetRootTxId() const { return m_string2; }
//    1void SetRootTxId(std::string value) { m_string2 = new std::string(std::move(value)); }
//
//    [[nodiscard]] std::string* GetRelayTxId() const { return m_string3; }
//    1void SetRelayTxId(std::string value) { m_string3 = new std::string(std::move(value)); }
    };

} // namespace PocketTx

#endif //SRC_SCOREPOST_HPP
