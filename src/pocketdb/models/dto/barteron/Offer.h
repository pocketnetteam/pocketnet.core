// Copyright (c) 2023 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETTX_BARTERON_OFFER_H
#define POCKETTX_BARTERON_OFFER_H

#include "pocketdb/models/dto/barteron/List.h"

namespace PocketTx
{
    using namespace std;

    class BarteronOffer : public BarteronList
    {
    public:
        BarteronOffer();
        BarteronOffer(const CTransactionRef& tx);

        const optional<string>& GetRootTxHash() const;
        void SetRootTxHash(const string& value);
        
        bool IsEdit() const;

        optional<string> GetPayloadLang() const;
        optional<string> GetPayloadCaption() const;
        optional<string> GetPayloadMessage() const;
        
        optional<string> GetPayloadTags() const;
        const optional<vector<int64_t>> GetPayloadTagsIds() const;

        optional<string> GetPayloadImages() const;
        optional<string> GetPayloadSettings() const;
    };

} // namespace PocketTx

#endif // POCKETTX_BARTERON_OFFER_H