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
    void App::SetId(const string& value) { Transaction::GetPayload()->SetString2(value); }

    const optional<UniValue>& App::GetData() const
    {
        if (!Transaction::GetPayload())
            return nullopt;

        UniValue data(UniValue::VOBJ);
        // TODO app : parse data with try catch ?
        data.read(*Transaction::GetPayload()->GetString1());

        if (data.isNull() || !data.isObject())
            return nullopt;

        return data;
    }
    
} // namespace PocketTx