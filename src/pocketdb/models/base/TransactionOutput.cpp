// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include "pocketdb/models/base/TransactionOutput.h"

namespace PocketTx
{
    const optional <string>& TransactionOutput::GetTxHash() const { return m_txHash; }
    void TransactionOutput::SetTxHash(string value) { m_txHash = value; }

    const optional <int64_t>& TransactionOutput::GetNumber() const { return m_number; }
    void TransactionOutput::SetNumber(int64_t value) { m_number = value; }

    const optional <string>& TransactionOutput::GetAddressHash() const { return m_addressHash; }
    void TransactionOutput::SetAddressHash(string value) { m_addressHash = value; }

    const optional <int64_t>& TransactionOutput::GetValue() const { return m_value; }
    void TransactionOutput::SetValue(int64_t value) { m_value = value; }
    
    const optional <string>& TransactionOutput::GetScriptPubKey() const { return m_scriptPubKey; }
    void TransactionOutput::SetScriptPubKey(string value) { m_scriptPubKey = value; }
        
    const optional<string>& TransactionOutput::GetSpentTxHash() const { return m_spentTxHash; }
    void TransactionOutput::SetSpentTxHash(string value) { m_spentTxHash = value; }

    const optional<int64_t>& TransactionOutput::GetSpentHeight() const { return m_spentHeight; }
    void TransactionOutput::SetSpentHeight(int64_t value) { m_spentHeight = value; }

} // namespace PocketTx