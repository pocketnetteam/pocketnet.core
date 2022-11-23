// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETTX_TRANSACTION_INPUT_H
#define POCKETTX_TRANSACTION_INPUT_H

#include "pocketdb/models/base/Base.h"

namespace PocketTx
{
    using namespace std;

    class TransactionInput : public Base
    {
    public:
        TransactionInput() = default;

        const optional<string>& GetSpentTxHash() const;
        void SetSpentTxHash(string value);

        const optional<string>& GetTxHash() const;
        void SetTxHash(string value);

        const optional<int64_t>& GetNumber() const;
        void SetNumber(int64_t value);

        const optional<string>& GetAddressHash() const;
        void SetAddressHash(string value);

        const optional<int64_t>& GetValue() const;
        void SetValue(int64_t value);
        
    protected:
        optional<string> m_spentTxHash = nullopt;
        optional<string> m_txHash = nullopt;
        optional<int64_t> m_number = nullopt;
        optional<string> m_addresshash = nullopt;
        optional<int64_t> m_value = nullopt;
    };

} // namespace PocketTx

#endif // POCKETTX_TRANSACTION_INPUT_H