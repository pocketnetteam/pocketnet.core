// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETTX_PAYLOAD_H
#define POCKETTX_PAYLOAD_H

#include "pocketdb/models/base/Base.h"

namespace PocketTx
{
    using namespace std;

    class Payload : public Base
    {
    public:
        Payload();

        shared_ptr<string> GetTxHash() const;
        void SetTxHash(string value);

        shared_ptr<string> GetString1() const;
        void SetString1(string value);

        shared_ptr<string> GetString2() const;
        void SetString2(string value);

        shared_ptr<string> GetString3() const;
        void SetString3(string value);

        shared_ptr<string> GetString4() const;
        void SetString4(string value);

        shared_ptr<string> GetString5() const;
        void SetString5(string value);

        shared_ptr<string> GetString6() const;
        void SetString6(string value);

        shared_ptr<string> GetString7() const;
        void SetString7(string value);

        shared_ptr<int> GetInt1() const;
        void SetInt1(int value);

    protected:

        shared_ptr<string> m_txHash = nullptr;
        shared_ptr<string> m_string1 = nullptr;
        shared_ptr<string> m_string2 = nullptr;
        shared_ptr<string> m_string3 = nullptr;
        shared_ptr<string> m_string4 = nullptr;
        shared_ptr<string> m_string5 = nullptr;
        shared_ptr<string> m_string6 = nullptr;
        shared_ptr<string> m_string7 = nullptr;
        shared_ptr<int> m_int1 = nullptr;

    };

} // namespace PocketTx

#endif // POCKETTX_PAYLOAD_H