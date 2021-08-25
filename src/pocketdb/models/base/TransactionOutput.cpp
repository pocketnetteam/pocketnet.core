// Copyright (c) 2018-2021 Pocketnet developers
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

} // namespace PocketTx