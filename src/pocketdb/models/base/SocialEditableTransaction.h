// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETTX_SOCIAL_EDITABLE_TRANSACTION_H
#define POCKETTX_SOCIAL_EDITABLE_TRANSACTION_H

#include "pocketdb/models/base/SocialTransaction.h"

namespace PocketTx
{
    class SocialEditableTransaction : public SocialTransaction
    {
    public:
        SocialEditableTransaction();
        SocialEditableTransaction(const CTransactionRef& tx);

        const optional<string>& GetRootTxHash() const;
        void SetRootTxHash(const string& value);

        bool IsEdit() const;
        
        void Deserialize(const UniValue& src) override;
        string BuildHash() override;
        size_t PayloadSize() const override;
    };

    typedef shared_ptr<SocialTransaction> SocialTransactionRef;
}

#endif // POCKETTX_SOCIAL_EDITABLE_TRANSACTION_H
