// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include <primitives/transaction.h>
#include "pocketdb/models/dto/content/Content.h"

namespace PocketTx
{
    Content::Content() : Transaction()
    {
    }

    Content::Content(const CTransactionRef& tx) : Transaction(tx)
    {
    }

    shared_ptr<string> Content::GetAddress() const { return m_string1; }
    void Content::SetAddress(const string& value) { m_string1 = make_shared<string>(value); }

    shared_ptr<string> Content::GetRootTxHash() const { return m_string2; }
    void Content::SetRootTxHash(const string& value) { m_string2 = make_shared<string>(value); }

    bool Content::IsEdit() const { return *m_string2 != *m_hash; }

} // namespace PocketTx