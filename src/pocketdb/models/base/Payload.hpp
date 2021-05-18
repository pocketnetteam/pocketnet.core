// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 Bitcoin developers
// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETTX_PAYLOAD_H
#define POCKETTX_PAYLOAD_H

#include "pocketdb/models/base/Base.hpp"

namespace PocketTx
{
    class Payload : public Base
    {
    public:

        Payload() {}
        ~Payload() {}

        shared_ptr <string> GetTxHash() const { return m_txHash; }
        void SetTxHash(string value) { m_txHash = make_shared<string>(value); }

        shared_ptr <string> GetString1() const { return m_string1; }
        void SetString1(string value) { m_string1 = make_shared<string>(value); }

        shared_ptr <string> GetString2() const { return m_string2; }
        void SetString2(string value) { m_string2 = make_shared<string>(value); }

        shared_ptr <string> GetString3() const { return m_string3; }
        void SetString3(string value) { m_string3 = make_shared<string>(value); }

        shared_ptr <string> GetString4() const { return m_string4; }
        void SetString4(string value) { m_string4 = make_shared<string>(value); }

        shared_ptr <string> GetString5() const { return m_string5; }
        void SetString5(string value) { m_string5 = make_shared<string>(value); }

        shared_ptr <string> GetString6() const { return m_string6; }
        void SetString6(string value) { m_string6 = make_shared<string>(value); }

        shared_ptr <string> GetString7() const { return m_string7; }
        void SetString7(string value) { m_string7 = make_shared<string>(value); }

    protected:

        shared_ptr <string> m_txHash = nullptr;
        shared_ptr <string> m_string1 = nullptr;
        shared_ptr <string> m_string2 = nullptr;
        shared_ptr <string> m_string3 = nullptr;
        shared_ptr <string> m_string4 = nullptr;
        shared_ptr <string> m_string5 = nullptr;
        shared_ptr <string> m_string6 = nullptr;
        shared_ptr <string> m_string7 = nullptr;

    private:

    };

} // namespace PocketTx

#endif // POCKETTX_PAYLOAD_H