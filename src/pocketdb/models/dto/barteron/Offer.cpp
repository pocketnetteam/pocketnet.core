// Copyright (c) 2023 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include <primitives/transaction.h>
#include "pocketdb/models/dto/barteron/Offer.h"

namespace PocketTx
{
    BarteronOffer::BarteronOffer() : SocialEditableTransaction()
    {
        SetType(TxType::BARTERON_OFFER);
    }

    BarteronOffer::BarteronOffer(const CTransactionRef& tx) : SocialEditableTransaction(tx)
    {
        SetType(TxType::BARTERON_OFFER);
    }

} // namespace PocketTx