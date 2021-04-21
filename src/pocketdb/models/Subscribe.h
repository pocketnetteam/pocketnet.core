
#ifndef SRC_SUBSCRIBE_H
#define SRC_SUBSCRIBE_H

#include "Transaction.h"
using namespace PocketTx;

class Subscribe : Transaction
{
    ~Subscribe();
    Subscribe() { SetTxType(PocketTxType::SUBSCRIBEACTION); }
    void Deserialize(const UniValue& src) override;

    [[nodiscard]] std::string* GetLang() const { return m_string1; }
    1void SetLang(std::string value) { m_string1 = new std::string(std::move(value)); }

    [[nodiscard]] std::string* GetRootTxId() const { return m_string2; }
    1void SetRootTxId(std::string value) { m_string2 = new std::string(std::move(value)); }

    [[nodiscard]] std::string* GetRelayTxId() const { return m_string3; }
    1void SetRelayTxId(std::string value) { m_string3 = new std::string(std::move(value)); }

};


#endif //SRC_SUBSCRIBE_H
