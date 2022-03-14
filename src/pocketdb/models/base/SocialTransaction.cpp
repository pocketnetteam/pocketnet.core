// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include "pocketdb/models/base/SocialTransaction.h"

namespace PocketTx
{
    SocialTransaction::SocialTransaction() : Transaction()
    {
    }

    SocialTransaction::SocialTransaction(const CTransactionRef& tx) : Transaction(tx)
    {
    }

    shared_ptr<UniValue> SocialTransaction::Serialize() const
    {
        auto result = Transaction::Serialize();

        result->pushKV("address", GetAddress() ? *GetAddress() : "");

        return result;
    }

    void SocialTransaction::Deserialize(const UniValue& src)
    {
        Transaction::Deserialize(src);
        if (auto[ok, val] = TryGetStr(src, "address"); ok) SetAddress(val);
    }
    
    shared_ptr<string> SocialTransaction::GetAddress() const { return m_string1; }
    void SocialTransaction::SetAddress(const string& value) { m_string1 = make_shared<string>(value); }

} // namespace PocketTx

