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

        const optional<string>& GetTxHash() const;
        void SetTxHash(string value);

        const optional<string>& GetString1() const;
        void SetString1(string value);

        const optional<string>& GetString2() const;
        void SetString2(string value);

        const optional<string>& GetString3() const;
        void SetString3(string value);

        const optional<string>& GetString4() const;
        void SetString4(string value);

        const optional<string>& GetString5() const;
        void SetString5(string value);

        const optional<string>& GetString6() const;
        void SetString6(string value);

        const optional<string>& GetString7() const;
        void SetString7(string value);

        const optional<int64_t>& GetInt1() const;
        void SetInt1(int64_t value);

    protected:

        optional<string> m_txHash = nullopt;
        optional<string> m_string1 = nullopt;
        optional<string> m_string2 = nullopt;
        optional<string> m_string3 = nullopt;
        optional<string> m_string4 = nullopt;
        optional<string> m_string5 = nullopt;
        optional<string> m_string6 = nullopt;
        optional<string> m_string7 = nullopt;
        optional<int64_t> m_int1 = nullopt;

    };

} // namespace PocketTx

#endif // POCKETTX_PAYLOAD_H