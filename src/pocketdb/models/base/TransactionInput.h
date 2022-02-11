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

        shared_ptr<string> GetSpentTxHash() const;
        void SetSpentTxHash(string value);

        shared_ptr<string> GetTxHash() const;
        void SetTxHash(string value);

        shared_ptr<int64_t> GetNumber() const;
        void SetNumber(int64_t value);
        
    protected:
        shared_ptr<string> m_spentTxHash = nullptr;
        shared_ptr<string> m_txHash = nullptr;
        shared_ptr<int64_t> m_number = nullptr;
    };

} // namespace PocketTx

#endif // POCKETTX_TRANSACTION_INPUT_H