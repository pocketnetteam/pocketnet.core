// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include <primitives/transaction.h>
#include "pocketdb/models/dto/content/App.h"

namespace PocketTx
{
    App::App() : SocialEditableTransaction()
    {
        SetType(TxType::CONTENT_APP);
    }

    App::App(const CTransactionRef& tx) : SocialEditableTransaction(tx)
    {
        SetType(TxType::CONTENT_APP);
    }

    const optional<string>& App::GetId() const { return m_string3; }
    void App::SetId(const string& value) { m_string3 = value; }

    const optional<string>& App::GetName() const { return Transaction::GetPayload()->GetString1(); }
    void App::SetName(const string& value) { Transaction::GetPayload()->SetString1(value); }

    const optional<string>& App::GetDescription() const { return Transaction::GetPayload()->GetString2(); }
    void App::SetDescription(const string& value) { Transaction::GetPayload()->SetString2(value); }

    const optional<string>& App::GetSettings() const { return Transaction::GetPayload()->GetString3(); }
    void App::SetSettings(const string& value) { Transaction::GetPayload()->SetString3(value); }
    
} // namespace PocketTx