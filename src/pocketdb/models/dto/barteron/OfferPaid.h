// Copyright (c) 2023 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETTX_BARTERON_OFFER_PAID_H
#define POCKETTX_BARTERON_OFFER_PAID_H

#include "pocketdb/models/dto/barteron/Offer.h"

namespace PocketTx
{
    class BarteronOfferPaid : public BarteronOffer
    {
    public:
        BarteronOfferPaid();
        BarteronOfferPaid(const CTransactionRef& tx);
    };

} // namespace PocketTx

#endif // POCKETTX_BARTERON_OFFER_PAID_H