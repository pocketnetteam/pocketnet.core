// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETTX_POST_H
#define POCKETTX_POST_H

#include "pocketdb/models/dto/content/Content.h"

namespace PocketTx
{
    using namespace std;

    class Post : public Content
    {
    public:
        Post();
        Post(const CTransactionRef& tx);

        shared_ptr<UniValue> Serialize() const override;
        void Deserialize(const UniValue& src) override;
        void DeserializeRpc(const UniValue& src) override;
        void DeserializePayload(const UniValue& src) override;

        shared_ptr<string> GetRelayTxHash() const;
        void SetRelayTxHash(const string& value);

        shared_ptr<string> GetPayloadLang() const;
        shared_ptr<string> GetPayloadCaption() const;
        shared_ptr<string> GetPayloadMessage() const;
        shared_ptr<string> GetPayloadTags() const;
        shared_ptr<string> GetPayloadUrl() const;
        shared_ptr<string> GetPayloadImages() const;
        shared_ptr<string> GetPayloadSettings() const;

        string BuildHash() override;
    };

} // namespace PocketTx

#endif // POCKETTX_POST_H