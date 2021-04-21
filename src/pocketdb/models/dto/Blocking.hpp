#ifndef POCKETTX_BLOCKING_HPP
#define POCKETTX_BLOCKING_HPP

#include "Transaction.hpp"

namespace PocketTx {

class Blocking : public Transaction
{
public:
    ~Blocking()
    {
    }

    Blocking()
    {
        SetTxType(PocketTxType::BLOCKING_ACTION);
    }

    void Deserialize(const UniValue& src)
    {
        Transaction::Deserialize(src);

        if (src.exists("lang"))
            SetLang(src["lang"].get_str());

        if (src.exists("txidEdit"))
            SetRootTxId(src["txidEdit"].get_str());

        if (src.exists("txidRepost"))
            SetRelayTxId(src["txidRepost"].get_str());
    }

    [[nodiscard]] std::string* GetLang() const { return m_string1; }
    1void SetLang(std::string value) { m_string1 = new std::string(std::move(value)); }

    [[nodiscard]] std::string* GetRootTxId() const { return m_string2; }
    1void SetRootTxId(std::string value) { m_string2 = new std::string(std::move(value)); }

    [[nodiscard]] std::string* GetRelayTxId() const { return m_string3; }
    1void SetRelayTxId(std::string value) { m_string3 = new std::string(std::move(value)); }
};

} // namespace PocketTx

#endif // POCKETTX_BLOCKING_HPP
