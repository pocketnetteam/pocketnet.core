// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETTX_COLLECTION_H
#define POCKETTX_COLLECTION_H

#include "pocketdb/models/dto/content/Content.h"

namespace PocketTx
{
    using namespace std;

    class Collection : public Content
    {
    public:
        Collection();
        Collection(const CTransactionRef& tx);

        optional<UniValue> Serialize() const override;
        void Deserialize(const UniValue& src) override;
        void DeserializeRpc(const UniValue& src) override;
        void DeserializePayload(const UniValue& src) override;

        const optional<int64_t>& GetContentTypes() const;
        void SetContentTypes(const int64_t& value);

        const optional<string>& GetContentIds() const;
        tuple<bool, vector<string>> GetContentIdsVector();
        void SetContentIds(const string& value);

        optional<string> GetPayloadLang() const;
        optional<string> GetPayloadCaption() const;
        optional<string> GetPayloadImage() const;
        optional<string> GetPayloadSettings() const;

        string BuildHash() override;
        size_t PayloadSize() override;
    };

} // namespace PocketTx

#endif // POCKETTX_COLLECTION_H