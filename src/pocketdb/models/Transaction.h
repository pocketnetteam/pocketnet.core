#ifndef TESTDEMO_TRANSACTION_H
#define TESTDEMO_TRANSACTION_H

#include <string>

using std::string;

class Transaction {
public:
    int GetTxType() const { return m_txType; }
    void SetTxType(int value) { m_txType = value; }

    string * GetTxId() const { return m_txId; }
    void SetTxId(string value) { m_txId = new string(value); }

    int GetTxTime() const { return m_txTime; }
    void SetTxTime(int value) { m_txTime = value; }

    int GetBlock() const { return m_block; }
    void SetBlock(int value) { m_block = value; }

    string * GetAddress() const { return m_address; }
    void SetAddress(string value) { m_address = new string(value); }

    int GetInt1() const { return m_int1; }
    int GetInt2() const { return m_int2; }
    int GetInt3() const { return m_int3; }
    int GetInt4() const { return m_int4; }
    int GetInt5() const { return m_int5; }

    string * GetString1() const { return m_string1; }
    string * GetString2() const { return m_string2; }
    string * GetString3() const { return m_string3; }
    string * GetString4() const { return m_string4; }
    string * GetString5() const { return m_string5; }

protected:
    virtual ~Transaction() = 0;

    int m_txType;
    string * m_txId;
    int m_txTime;
    int m_block;
    string * m_address;

    int m_int1;
    int m_int2;
    int m_int3;
    int m_int4;
    int m_int5;

    string * m_string1;
    string * m_string2;
    string * m_string3;
    string * m_string4;
    string * m_string5;
};

#endif //TESTDEMO_TRANSACTION_H
