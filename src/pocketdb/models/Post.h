#ifndef TESTDEMO_POST_H
#define TESTDEMO_POST_H


#include "Transaction.h"

class Post : public Transaction {
public:
    string * GetLang() const { return m_string1; }
    void SetLang(string value) { m_string1 = new string(value); }

    string * GetRootTxId() const { return m_string2; }
    void SetRootTxId(string value) { m_string2 = new string(value); }

    string * GetRelay() const { return m_string3; }
    void SetRelay(string value) { m_string3 = new string(value); }

    ~Post() override {};
};

#endif //TESTDEMO_POST_H
