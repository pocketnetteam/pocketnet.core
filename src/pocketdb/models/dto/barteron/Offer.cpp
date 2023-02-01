// Copyright (c) 2023 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include <primitives/transaction.h>
#include "pocketdb/models/dto/barteron/Offer.h"

namespace PocketTx
{
    BarteronOffer::BarteronOffer() : BarteronList()
    {
        SetType(TxType::BARTERON_OFFER);
    }

    BarteronOffer::BarteronOffer(const CTransactionRef& tx) : BarteronList(tx)
    {
        SetType(TxType::BARTERON_OFFER);
    }

    const optional<string>& BarteronOffer::GetRootTxHash() const { return m_string2; }
    void BarteronOffer::SetRootTxHash(const string& value) { m_string2 = value; }

    bool BarteronOffer::IsEdit() const { return m_string2 != m_hash; }

    optional<string> BarteronOffer::GetPayloadLang() const { return GetPayload() ? GetPayload()->GetString1() : nullopt; }
    optional<string> BarteronOffer::GetPayloadCaption() const { return GetPayload() ? GetPayload()->GetString2() : nullopt; }
    optional<string> BarteronOffer::GetPayloadMessage() const { return GetPayload() ? GetPayload()->GetString3() : nullopt; }

    optional<string> BarteronOffer::GetPayloadTags() const { return GetPayload() ? GetPayload()->GetString4() : nullopt; }
    const optional<vector<int64_t>> BarteronOffer::GetPayloadTagsIds() const { return ParseList(GetPayloadTags()); }

    optional<string> BarteronOffer::GetPayloadImages() const { return GetPayload() ? GetPayload()->GetString5() : nullopt; }
    optional<string> BarteronOffer::GetPayloadSettings() const { return GetPayload() ? GetPayload()->GetString6() : nullopt; }

} // namespace PocketTx