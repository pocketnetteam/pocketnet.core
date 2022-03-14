// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETTX_SOCIAL_TRANSACTION_H
#define POCKETTX_SOCIAL_TRANSACTION_H

#include "pocketdb/models/base/Transaction.h"

namespace PocketTx
{
    class SocialTransaction : public Transaction
    {
    public:
        SocialTransaction();
        SocialTransaction(const CTransactionRef& tx);

        shared_ptr<UniValue> Serialize() const override;
        void Deserialize(const UniValue& src) override;

        shared_ptr<string> GetAddress() const;
        void SetAddress(const string& value) override;
        
    }; // class SocialTransaction

} // namespace PocketTx

#endif // POCKETTX_SOCIAL_TRANSACTION_H
