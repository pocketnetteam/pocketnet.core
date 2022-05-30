// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include "pocketdb/models/shortform/ShortForm.h"

PocketDb::ShortForm::ShortForm(ShortTxData txData, std::optional<ShortTxData> relatedContent)
    : m_txData(std::move(txData)),
      m_relatedContent(std::move(relatedContent))
{}

UniValue PocketDb::ShortForm::Serialize() const
{
    auto data = m_txData.Serialize();
    if (m_relatedContent) data.pushKV("relatedContent", m_relatedContent->Serialize());

    return data;
}