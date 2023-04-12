// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include <primitives/transaction.h>
#include "pocketdb/models/dto/content/Content.h"

namespace PocketTx
{
    Content::Content() : SocialTransaction() {}
    Content::Content(const CTransactionRef& tx) : SocialTransaction(tx) {}
    Content::~Content() {}

    const optional<string>& Content::GetAddress() const { return m_string1; }
    void Content::SetAddress(const string& value) { m_string1 = value; }

    const optional<string>& Content::GetRootTxHash() const { return m_string2; }
    void Content::SetRootTxHash(const string& value) { m_string2 = value; }

    bool Content::IsEdit() const { return m_string2 != m_hash; }

} // namespace PocketTx