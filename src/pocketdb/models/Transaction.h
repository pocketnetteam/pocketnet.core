#ifndef TESTDEMO_TRANSACTION_H
#define TESTDEMO_TRANSACTION_H

#include <string>
#include <utility>
#include <univalue.h>
#include <utilstrencodings.h>

namespace PocketTx {

enum PocketTxType {
    USERACCOUNT = 100,
    VIDEOSERVERACCOUNT = 101,
    MESSAGESERVERACCOUNT = 102,

    POSTCONTENT = 200,
    VIDEOCONTENT = 201,
    TRANSLATECONTENT = 202,
    SERVERPINGCONTENT = 203,
    COMMENTCONTENT = 204,

    SCOREPOSTACTION = 300,
    SCORECOMMENTACTION = 301,
    SUBSCRIBEACTION = 302,
    BLOCKACTION = 303,
    COMPLAINACTION = 304,
};

class Transaction
{
public:
    Transaction() = default;


    PocketTxType* GetTxType() const { return m_txType; }
    void SetTxType(PocketTxType value) { m_txType = new PocketTxType(value); }

    std::string* GetTxId() const { return m_txId; }
    void SetTxId(std::string value) { m_txId = new std::string(std::move(value)); }

    int64_t * GetTxTime() const { return m_txTime; }
    void SetTxTime(int64_t value) { m_txTime = new int64_t(value); }

//    int* GetBlock() const { return m_block; }
//    void SetBlock(int value) { m_block = &value; }

    std::string* GetAddress() const { return m_address; }
    void SetAddress(std::string value) { m_address = new std::string(std::move(value)); }

    int64_t* GetInt1() const { return m_int1; }
    int64_t* GetInt2() const { return m_int2; }
    int64_t* GetInt3() const { return m_int3; }
    int64_t* GetInt4() const { return m_int4; }
    int64_t* GetInt5() const { return m_int5; }

    std::string* GetString1() const { return m_string1; }
    std::string* GetString2() const { return m_string2; }
    std::string* GetString3() const { return m_string3; }
    std::string* GetString4() const { return m_string4; }
    std::string* GetString5() const { return m_string5; }

    virtual void Deserialize(const UniValue& src);
    std::string Serialize(const PocketTxType& txType);

protected:
    PocketTxType* m_txType = nullptr;
    std::string* m_txId = nullptr;
    int64_t* m_txTime = nullptr;
    std::string* m_address = nullptr;

    int64_t* m_int1 = nullptr;
    int64_t* m_int2 = nullptr;
    int64_t* m_int3 = nullptr;
    int64_t* m_int4 = nullptr;
    int64_t* m_int5 = nullptr;

    std::string* m_string1 = nullptr;
    std::string* m_string2 = nullptr;
    std::string* m_string3 = nullptr;
    std::string* m_string4 = nullptr;
    std::string* m_string5 = nullptr;
};

} // namespace PocketTx

#endif //TESTDEMO_TRANSACTION_H
