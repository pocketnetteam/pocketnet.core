// Copyright (c) 2023 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include <primitives/transaction.h>
#include "pocketdb/models/dto/barteron/Offer.h"

namespace PocketTx
{
    BarteronOffer::BarteronOffer() : SocialTransaction()
    {
        SetType(TxType::BARTERON_OFFER);
    }

    BarteronOffer::BarteronOffer(const CTransactionRef& tx) : SocialTransaction(tx)
    {
        SetType(TxType::BARTERON_OFFER);
    }

    const optional<string>& BarteronOffer::GetRootTxHash() const { return m_string2; }
    void BarteronOffer::SetRootTxHash(const string& value) { m_string2 = value; }

    bool BarteronOffer::IsEdit() const { return m_string2 != m_hash; }

} // namespace PocketTx