// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include "pocketdb/models/shortform/ShortTxData.h"

#include "pocketdb/helpers/ShortFormHelper.h"


PocketDb::ShortTxData::ShortTxData(std::string hash, PocketDb::ShortTxType type, std::string address,
                                    std::optional<ShortAccount> account, std::optional<int> val,
                                    std::optional<std::string> description)
    : m_hash(std::move(hash)),
      m_type(type),
      m_address(std::move(address)),
      m_account(std::move(account)),
      m_val(std::move(val)),
      m_description(std::move(description))
{}

UniValue PocketDb::ShortTxData::Serialize() const
{
    UniValue data(UniValue::VOBJ);

    data.pushKV("hash", m_hash);
    data.pushKV("type", PocketHelpers::ShortTxTypeConvertor::toString(m_type));
    data.pushKV("address", m_address);
    if (m_account) data.pushKV("account", m_account->Serialize());
    if (m_val) data.pushKV("val", m_val.value());
    if (m_description) data.pushKV("description", m_description.value());

    return data;
}