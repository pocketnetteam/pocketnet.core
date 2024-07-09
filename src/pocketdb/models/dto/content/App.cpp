// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include <primitives/transaction.h>
#include "pocketdb/models/dto/content/App.h"

namespace PocketTx
{
    App::App() : SocialEditableTransaction()
    {
        SetType(TxType::APP);
    }

    App::App(const CTransactionRef& tx) : SocialEditableTransaction(tx)
    {
        SetType(TxType::APP);
    }

    const optional<string>& App::GetId() const { return Transaction::GetPayload()->GetString2(); }
    
} // namespace PocketTx