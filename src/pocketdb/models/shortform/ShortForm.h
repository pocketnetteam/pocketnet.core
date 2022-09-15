// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETDB_SHORTFORM_H
#define POCKETDB_SHORTFORM_H

#include "pocketdb/models/shortform/ShortTxData.h"
#include "pocketdb/models/shortform/ShortTxType.h"

#include <optional>

namespace PocketDb
{
    class ShortForm
    {
    public:
        ShortForm(PocketDb::ShortTxType type, ShortTxData txData, std::optional<ShortTxData> relatedContent);

        UniValue Serialize() const;

        PocketDb::ShortTxType GetType() const;
        const ShortTxData& GetTxData() const;
        const std::optional<ShortTxData>& GetRelaytedContent() const;
        void SetRelatedContent(const std::optional<ShortTxData> &relatedContent);
    private:
        PocketDb::ShortTxType m_type;
        ShortTxData m_txData;
        std::optional<ShortTxData> m_relatedContent;
    };
}

#endif // POCKETDB_SHORTFORM_H