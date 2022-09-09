// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include "pocketdb/models/shortform/ShortTxOutput.h"


UniValue PocketDb::ShortTxOutput::Serialize() const
{
    UniValue data (UniValue::VOBJ);

    if (m_txHash) data.pushKV("txhash", *m_txHash);
    if (m_spentTxHash) data.pushKV("spenttxhash", *m_spentTxHash);    
    if (m_addressHash) data.pushKV("addresshash", *m_addressHash);
    if (m_number) data.pushKV("number", *m_number);
    if (m_value) data.pushKV("value", *m_value);
    if (m_scriptPubKey) data.pushKV("scriptpubkey", *m_scriptPubKey);

    return data;
}

void PocketDb::ShortTxOutput::SetTxHash(const std::optional<std::string>& txHash) { m_txHash = txHash; }

const std::optional<std::string>& PocketDb::ShortTxOutput::GetTxHash() const { return m_txHash; }

void PocketDb::ShortTxOutput::SetSpentTxHash(const std::optional<std::string>& spentTxHash) { m_spentTxHash = spentTxHash; }

const std::optional<std::string>& PocketDb::ShortTxOutput::GetSpentTxHash() const { return m_spentTxHash; }

void PocketDb::ShortTxOutput::SetNumber(const std::optional<int>& number) { m_number = number; }

const std::optional<int>& PocketDb::ShortTxOutput::GetNumber() const { return m_number; }

void PocketDb::ShortTxOutput::SetAddressHash(const std::optional<std::string>& addressHash) { m_addressHash = addressHash; }

const std::optional<std::string>& PocketDb::ShortTxOutput::GetAddressHash() const { return m_addressHash; }

void PocketDb::ShortTxOutput::SetScriptPubKey(const std::optional<std::string>& scriptPubKey) { m_scriptPubKey = scriptPubKey; }

const std::optional<std::string>& PocketDb::ShortTxOutput::GetcriptPubKey() const { return m_scriptPubKey; }

void PocketDb::ShortTxOutput::SetValue(const std::optional<int64_t>& value) { m_value = value; }

const std::optional<int64_t>& PocketDb::ShortTxOutput::GetValue() const { return m_value; }

