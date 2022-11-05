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

        shared_ptr<string> GetTxHash() const;
        void SetTxHash(string value);

        shared_ptr<int64_t> GetNumber() const;
        void SetNumber(int64_t value);

        shared_ptr<string> GetAddressHash() const;
        void SetAddressHash(string value);

        shared_ptr<int64_t> GetValue() const;
        void SetValue(int64_t value);
        
        shared_ptr<string> GetScriptPubKey() const;
        void SetScriptPubKey(string value);
        
        shared_ptr<string> GetSpentTxHash() const;
        void SetSpentTxHash(string value);

        shared_ptr<int64_t> GetSpentHeight() const;
        void SetSpentHeight(int64_t value);

    protected:
        shared_ptr<string> m_txHash = nullptr;
        shared_ptr<int64_t> m_number = nullptr;
        shared_ptr<string> m_addressHash = nullptr;
        shared_ptr<int64_t> m_value = nullptr;
        shared_ptr<string> m_scriptPubKey = nullptr;
        shared_ptr<string> m_spentTxHash = nullptr;
        shared_ptr<int64_t> m_spentHeight = nullptr;
    };

} // namespace PocketTx

#endif // POCKETTX_TRANSACTIONOUTPUT_H