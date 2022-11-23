// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETTX_TRANSACTIONOUTPUT_H
#define POCKETTX_TRANSACTIONOUTPUT_H

#include "pocketdb/models/base/Base.h"

namespace PocketTx
{
    using namespace std;

    class TransactionOutput : public Base
    {
    public:
        TransactionOutput() = default;

        const optional<string>& GetTxHash() const;
        void SetTxHash(string value);

        const optional<int64_t>& GetNumber() const;
        void SetNumber(int64_t value);

        const optional<string>& GetAddressHash() const;
        void SetAddressHash(string value);

        const optional<int64_t>& GetValue() const;
        void SetValue(int64_t value);
        
        const optional<string>& GetScriptPubKey() const;
        void SetScriptPubKey(string value);
        
        const optional<string>& GetSpentTxHash() const;
        void SetSpentTxHash(string value);

        const optional<int64_t>& GetSpentHeight() const;
        void SetSpentHeight(int64_t value);

    protected:
        optional<string> m_txHash = nullopt;
        optional<int64_t> m_number = nullopt;
        optional<string> m_addressHash = nullopt;
        optional<int64_t> m_value = nullopt;
        optional<string> m_scriptPubKey = nullopt;
        optional<string> m_spentTxHash = nullopt;
        optional<int64_t> m_spentHeight = nullopt;
    };

} // namespace PocketTx

#endif // POCKETTX_TRANSACTIONOUTPUT_H