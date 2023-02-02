// Copyright (c) 2023 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETTX_BARTERON_OFFER_H
#define POCKETTX_BARTERON_OFFER_H

#include "pocketdb/models/base/SocialTransaction.h"

namespace PocketTx
{
    using namespace std;

    class BarteronOffer : public SocialTransaction
    {
    public:
        BarteronOffer();
        BarteronOffer(const CTransactionRef& tx);

        const optional<string>& GetRootTxHash() const;
        void SetRootTxHash(const string& value);
        
        bool IsEdit() const;
    };

} // namespace PocketTx

#endif // POCKETTX_BARTERON_OFFER_H