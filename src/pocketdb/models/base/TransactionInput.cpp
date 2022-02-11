// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include "pocketdb/models/base/TransactionInput.h"

namespace PocketTx
{
    shared_ptr <string> TransactionInput::GetSpentTxHash() const { return m_spentTxHash; }
    void TransactionInput::SetSpentTxHash(string value) { m_spentTxHash = make_shared<string>(value); }

    shared_ptr <string> TransactionInput::GetTxHash() const { return m_txHash; }
    void TransactionInput::SetTxHash(string value) { m_txHash = make_shared<string>(value); }

    shared_ptr <int64_t> TransactionInput::GetNumber() const { return m_number; }
    void TransactionInput::SetNumber(int64_t value) { m_number = make_shared<int64_t>(value); }
    
} // namespace PocketTx