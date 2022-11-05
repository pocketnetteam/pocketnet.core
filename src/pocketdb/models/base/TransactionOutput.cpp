// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include "pocketdb/models/base/TransactionOutput.h"

namespace PocketTx
{
    shared_ptr <string> TransactionOutput::GetTxHash() const { return m_txHash; }
    void TransactionOutput::SetTxHash(string value) { m_txHash = make_shared<string>(value); }

    shared_ptr <int64_t> TransactionOutput::GetNumber() const { return m_number; }
    void TransactionOutput::SetNumber(int64_t value) { m_number = make_shared<int64_t>(value); }

    shared_ptr <string> TransactionOutput::GetAddressHash() const { return m_addressHash; }
    void TransactionOutput::SetAddressHash(string value) { m_addressHash = make_shared<string>(value); }

    shared_ptr <int64_t> TransactionOutput::GetValue() const { return m_value; }
    void TransactionOutput::SetValue(int64_t value) { m_value = make_shared<int64_t>(value); }
    
    shared_ptr <string> TransactionOutput::GetScriptPubKey() const { return m_scriptPubKey; }
    void TransactionOutput::SetScriptPubKey(string value) { m_scriptPubKey = make_shared<string>(value); }
        
    shared_ptr<string> TransactionOutput::GetSpentTxHash() const { return m_spentTxHash; }
    void TransactionOutput::SetSpentTxHash(string value) { m_spentTxHash = make_shared<string>(value); }

    shared_ptr<int64_t> TransactionOutput::GetSpentHeight() const { return m_spentHeight; }
    void TransactionOutput::SetSpentHeight(int64_t value) { m_spentHeight = make_shared<int64_t>(value); }

} // namespace PocketTx