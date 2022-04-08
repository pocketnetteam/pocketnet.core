// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETTX_CONTENT_H
#define POCKETTX_CONTENT_H

#include "pocketdb/models/base/Transaction.h"

namespace PocketTx
{
    using namespace std;

    class Content : public Transaction
    {
    public:
        Content();
        Content(const CTransactionRef& tx);

        shared_ptr<string> GetAddress() const;
        void SetAddress(const string& value);

        shared_ptr<string> GetRootTxHash() const;
        void SetRootTxHash(const string& value);
        
        bool IsEdit() const;
    };

} // namespace PocketTx

#endif // POCKETTX_CONTENT_H