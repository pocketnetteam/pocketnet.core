// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETTX_CONTENT_DELETE_H
#define POCKETTX_CONTENT_DELETE_H

#include "pocketdb/models/dto/content/Content.h"

namespace PocketTx
{
    using namespace std;

    class ContentDelete : public Content
    {
    public:
        ContentDelete();
        ContentDelete(const CTransactionRef& tx);

        shared_ptr<UniValue> Serialize() const override;

        void Deserialize(const UniValue& src) override;
        void DeserializeRpc(const UniValue& src) override;
        void DeserializePayload(const UniValue& src) override;

        string BuildHash() override;

    }; // class ContentDelete

} // namespace PocketTx

#endif // POCKETTX_CONTENT_DELETE_H
