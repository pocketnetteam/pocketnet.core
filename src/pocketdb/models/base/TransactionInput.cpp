// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include "pocketdb/models/base/TransactionInput.h"

namespace PocketTx
{
    const optional<string>& TransactionInput::GetSpentTxHash() const { return m_spentTxHash; }
    void TransactionInput::SetSpentTxHash(string value) { m_spentTxHash = value; }

    const optional<string>& TransactionInput::GetTxHash() const { return m_txHash; }
    void TransactionInput::SetTxHash(string value) { m_txHash = value; }

    const optional<int64_t>& TransactionInput::GetNumber() const { return m_number; }
    void TransactionInput::SetNumber(int64_t value) { m_number = value; }

    const optional<string>& TransactionInput::GetAddressHash() const { return m_addresshash; }
    void TransactionInput::SetAddressHash(string value) { m_addresshash = value; }

    const optional<int64_t>& TransactionInput::GetValue() const { return m_value; }
    void TransactionInput::SetValue(int64_t value) { m_value = value; }
    
} // namespace PocketTx