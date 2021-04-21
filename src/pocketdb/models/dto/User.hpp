#ifndef POCKETTX_USER_HPP
#define POCKETTX_USER_HPP

#include "pocketdb/models/base/Transaction.hpp"

namespace PocketTx {

class User : public PocketTx::Transaction
{
public:
    ~User() {

    }

    User() { SetTxType(PocketTxType::USER_ACCOUNT); 
    }

    void Deserialize(const UniValue& src)  {
    Transaction::Deserialize(src);

    if (src.exists("id"))
        SetId(src["id"].get_int64());

    if (src.exists("regdate"))
        SetRegistration(src["regdate"].get_int64());

    if (src.exists("lang"))
        SetLang(src["lang"].get_str());

    if (src.exists("name"))
        SetName(src["name"].get_str());

    if (src.exists("referrer"))
        SetReferrer(src["referrer"].get_str());
}

    [[nodiscard]] int64_t* GetId() const { return m_int1; }
    void SetId(int64_t id) { m_int1 = new int64_t(id); }

    [[nodiscard]] int64_t* GetRegistration() const { return m_int2; }
    void SetRegistration(int64_t value) { m_int2 = new int64_t(value); }

    [[nodiscard]] std::string* GetLang() const { return m_string1; }
    void SetLang(std::string value) { m_string1 = new std::string(std::move(value)); }

    [[nodiscard]] std::string* GetName() const { return m_string2; }
    void SetName(std::string value) { m_string2 = new std::string(std::move(value)); }

    [[nodiscard]] std::string* GetReferrer() const { return m_string3; }
    void SetReferrer(std::string value) { m_string3 = new std::string(std::move(value)); }
};

} // namespace PocketTx

#endif // POCKETTX_USER_HPP
