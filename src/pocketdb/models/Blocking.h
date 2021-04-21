#ifndef SRC_BLOCKING_H
#define SRC_BLOCKING_H

#include "Transaction.h"
using namespace PocketTx;

class Blocking : Transaction
{
    ~Blocking();
    Blocking() { SetTxType(PocketTxType::BLOCKINGACTION); }
    void Deserialize(const UniValue& src) override;

    [[nodiscard]] std::string* GetLang() const { return m_string1; }
    void SetLang(std::string value) { m_string1 = new std::string(std::move(value)); }

    [[nodiscard]] std::string* GetRootTxId() const { return m_string2; }
    void SetRootTxId(std::string value) { m_string2 = new std::string(std::move(value)); }

    [[nodiscard]] std::string* GetRelayTxId() const { return m_string3; }
    void SetRelayTxId(std::string value) { m_string3 = new std::string(std::move(value)); }

};


#endif //SRC_BLOCKING_H
