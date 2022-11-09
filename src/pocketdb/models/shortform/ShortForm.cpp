// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include "pocketdb/models/shortform/ShortForm.h"
#include "pocketdb/helpers/ShortFormHelper.h"

PocketDb::ShortForm::ShortForm(PocketDb::ShortTxType type, ShortTxData txData, std::optional<ShortTxData> relatedContent)
    : m_type(type),
      m_txData(std::move(txData)),
      m_relatedContent(std::move(relatedContent))
{}

UniValue PocketDb::ShortForm::Serialize() const
{
    auto data = m_txData.Serialize();
    data.pushKV("type", PocketHelpers::ShortTxTypeConvertor::toString(m_type));
    if (m_relatedContent) data.pushKV("relatedContent", m_relatedContent->Serialize());

    return data;
}

PocketDb::ShortTxType PocketDb::ShortForm::GetType() const { return m_type; }

const PocketDb::ShortTxData& PocketDb::ShortForm::GetTxData() const { return m_txData; }

const std::optional<PocketDb::ShortTxData>& PocketDb::ShortForm::GetRelaytedContent() const { return m_relatedContent; }

void PocketDb::ShortForm::SetRelatedContent(const std::optional<ShortTxData> &relatedContent) { m_relatedContent = relatedContent; }
