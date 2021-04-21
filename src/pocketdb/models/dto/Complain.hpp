#ifndef POCKETTX_COMPLAIN_HPP
#define POCKETTX_COMPLAIN_HPP

#include "Transaction.h"

namespace PocketTx {

class Complain : public Transaction
{
public:
    ~Complain()
    {
    }

    Complain()
    {
        SetTxType(PocketTxType::COMPLAIN_ACTION);
    }

    Complain(const UniValue& src)
    {
        Complain();
        Deserialize(src);
    }

    void Deserialize(const UniValue& src) override
    {
        Transaction::Deserialize(src);

        // TODO (brangr): 1111111111111
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

#endif // POCKETTX_COMPLAIN_HPP
