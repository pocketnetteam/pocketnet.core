// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 Bitcoin developers
// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETTX_TRANSACTION_H
#define POCKETTX_TRANSACTION_H

#include <string>
#include <univalue.h>
#include <utility>
#include <utilstrencodings.h>

namespace PocketTx
{

    enum PocketTxType
    {
        USER_ACCOUNT = 100,
        VIDEO_SERVER_ACCOUNT = 101,
        MESSAGE_SERVER_ACCOUNT = 102,

        POST_CONTENT = 200,
        VIDEO_CONTENT = 201,
        TRANSLATE_CONTENT = 202,
        SERVERPING_CONTENT = 203,
        COMMENT_CONTENT = 204,

        SCORE_POST_ACTION = 300,
        SCORE_COMMENT_ACTION = 301,

        SUBSCRIBE_ACTION = 302,
        SUBSCRIBE_PRIVATE_ACTION = 303,
        SUBSCRIBE_CANCEL_ACTION = 304,

        BLOCKING_ACTION = 305,
        BLOCKING_CANCEL_ACTION = 306,

        COMPLAIN_ACTION = 307,
    };

    class Transaction
    {
    public:

        Transaction(const UniValue &src)
        {
            assert(src.exists("txid") && src["txid"].isStr());
            SetTxId(src["txid"].get_str());

            assert(src.exists("time") && src["time"].isNum());
            SetTxTime(src["time"].get_int64());

            assert(src.exists("address") && src["address"].isStr());
            SetAddress(src["address"].get_str());
        }

        ~Transaction() {
            // TODO (brangr): remove all pointers
        }

        std::string Serialize(const PocketTxType &txType)
        {
            std::string data;
            // TODO (brangr): implement

            return data;
        }


        PocketTxType *GetTxType() const { return m_txType; }

        void SetTxType(PocketTxType value) { m_txType = new PocketTxType(value); }

        std::string *GetTxId() const { return m_txId; }

        void SetTxId(std::string value) { m_txId = new std::string(std::move(value)); }

        int64_t *GetTxTime() const { return m_txTime; }

        void SetTxTime(int64_t value) { m_txTime = new int64_t(value); }

        //    int* GetBlock() const { return m_block; }
        //    void SetBlock(int value) { m_block = &value; }

        std::string *GetAddress() const { return m_address; }

        void SetAddress(std::string value) { m_address = new std::string(std::move(value)); }

        int64_t *GetInt1() const { return m_int1; }

        void SetInt1(int64_t value) { m_int1 = new int64_t(value); }

        int64_t *GetInt2() const { return m_int2; }

        void SetInt2(int64_t value) { m_int2 = new int64_t(value); }

        int64_t *GetInt3() const { return m_int3; }

        void SetInt3(int64_t value) { m_int3 = new int64_t(value); }

        int64_t *GetInt4() const { return m_int4; }

        void SetInt4(int64_t value) { m_int4 = new int64_t(value); }

        int64_t *GetInt5() const { return m_int5; }

        void SetInt5(int64_t value) { m_int5 = new int64_t(value); }


        std::string *GetString1() const { return m_string1; }

        void SetString1(std::string value) { m_string1 = new std::string(std::move(value)); }

        std::string *GetString2() const { return m_string2; }

        void SetString2(std::string value) { m_string2 = new std::string(std::move(value)); }

        std::string *GetString3() const { return m_string3; }

        void SetString3(std::string value) { m_string3 = new std::string(std::move(value)); }

        std::string *GetString4() const { return m_string4; }

        void SetString4(std::string value) { m_string4 = new std::string(std::move(value)); }

        std::string *GetString5() const { return m_string5; }

        void SetString5(std::string value) { m_string5 = new std::string(std::move(value)); }


        std::string *GetPayload() const { return m_payload; }
        bool HasPayload() const { return m_payload; };

        void SetPayload(std::string value) { m_payload = new std::string(std::move(value)); }

    protected:
        PocketTxType *m_txType = nullptr;
        std::string *m_txId = nullptr;
        int64_t *m_txTime = nullptr;
        std::string *m_address = nullptr;

        int64_t *m_int1 = nullptr;
        int64_t *m_int2 = nullptr;
        int64_t *m_int3 = nullptr;
        int64_t *m_int4 = nullptr;
        int64_t *m_int5 = nullptr;

        std::string *m_string1 = nullptr;
        std::string *m_string2 = nullptr;
        std::string *m_string3 = nullptr;
        std::string *m_string4 = nullptr;
        std::string *m_string5 = nullptr;

        std::string *m_payload = nullptr;
    };

} // namespace PocketTx

#endif // POCKETTX_TRANSACTION_H