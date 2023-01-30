// Copyright (c) 2023 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include <primitives/transaction.h>
#include "pocketdb/models/dto/content/BarteronOffer.h"

namespace PocketTx
{
    BarteronOffer::BarteronOffer() : Post()
    {
        SetType(TxType::CONTENT_BARTERON_OFFER);
    }

    BarteronOffer::BarteronOffer(const CTransactionRef& tx) : Post(tx)
    {
        SetType(TxType::CONTENT_BARTERON_OFFER);
    }

} // namespace PocketTx