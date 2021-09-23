// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETTX_CONTENT_DELETE_H
#define POCKETTX_CONTENT_DELETE_H

#include "pocketdb/models/base/Transaction.h"

namespace PocketTx
{
    class ContentDelete : public PocketTx::Transaction
    {
    public:
        ContentDelete();
        ContentDelete(const CTransactionRef& tx);

        shared_ptr<UniValue> Serialize() const override;

        void Deserialize(const UniValue& src) override;
        void DeserializeRpc(const UniValue& src, const CTransactionRef& tx) override;

        shared_ptr <string> GetAddress() const;
        void SetAddress(string value);
        
        shared_ptr<string> GetRootTxHash() const;
        void SetRootTxHash(string value);

    protected:
        void DeserializePayload(const UniValue& src, const CTransactionRef& tx) override;
        void BuildHash() override;
    }; // class ContentDelete

} // namespace PocketTx

#endif // POCKETTX_CONTENT_DELETE_H
