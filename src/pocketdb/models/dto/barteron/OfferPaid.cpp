// Copyright (c) 2023 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include "pocketdb/models/dto/barteron/OfferPaid.h"

namespace PocketTx
{
    BarteronOfferPaid::BarteronOfferPaid() : BarteronOffer()
    {
        SetType(TxType::BARTERON_OFFER_PAID);
    }

    BarteronOfferPaid::BarteronOfferPaid(const CTransactionRef& tx) : BarteronOffer(tx)
    {
        SetType(TxType::BARTERON_OFFER_PAID);
    }

} // namespace PocketTx