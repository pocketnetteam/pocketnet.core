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

        optional<UniValue> Serialize() const override;
        void Deserialize(const UniValue& src) override;
        void DeserializeRpc(const UniValue& src) override;
        void DeserializePayload(const UniValue& src) override;

        const optional<string>& GetRelayTxHash() const;
        void SetRelayTxHash(const string& value);

        optional<string> GetPayloadLang() const;
        optional<string> GetPayloadCaption() const;
        optional<string> GetPayloadMessage() const;
        optional<string> GetPayloadTags() const;
        optional<string> GetPayloadImages() const;
        optional<string> GetPayloadSettings() const;
        optional<string> GetPayloadUrl() const;

        string BuildHash() override;
        size_t PayloadSize() const override;
    };

} // namespace PocketTx

#endif // POCKETTX_POST_H