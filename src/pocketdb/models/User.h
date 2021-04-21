#ifndef POCKETTX_ACCOUNT_H
#define POCKETTX_ACCOUNT_H

#include <utility>
#include "Transaction.h"

namespace PocketTx {

class User : public PocketTx::Transaction
{
public:
    ~User();

    User() { SetTxType(PocketTxType::USERACCOUNT); }
    void Deserialize(const UniValue& src) override;

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

#endif // POCKETTX_ACCOUNT_H
