#ifndef POCKETTX_POST_HPP
#define POCKETTX_POST_HPP

#include "pocketdb/models/base/Transaction.hpp"

namespace PocketTx
{

    class Post : public Transaction
    {
    public:
        ~Post() = default;

        Post()
        {
            SetTxType(PocketTxType::POST_CONTENT);
        }

        void Deserialize(const UniValue &src) override
        {
            Transaction::Deserialize(src);

            if (src.exists("lang"))
                SetLang(src["lang"].get_str());

            if (src.exists("txidEdit"))
                SetRootTxId(src["txidEdit"].get_str());

            if (src.exists("txidRepost"))
                SetRelayTxId(src["txidRepost"].get_str());
        }

        [[nodiscard]] std::string *GetLang() const { return m_string1; }

        void SetLang(std::string value) { m_string1 = new std::string(std::move(value)); }

        [[nodiscard]] std::string *GetRootTxId() const { return m_string2; }

        void SetRootTxId(std::string value) { m_string2 = new std::string(std::move(value)); }

        [[nodiscard]] std::string *GetRelayTxId() const { return m_string3; }

        void SetRelayTxId(std::string value) { m_string3 = new std::string(std::move(value)); }
    };

} // namespace PocketTx

#endif // POCKETTX_POST_HPP