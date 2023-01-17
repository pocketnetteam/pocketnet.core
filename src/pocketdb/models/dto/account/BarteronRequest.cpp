// Copyright (c) 2023 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include <primitives/transaction.h>
#include "pocketdb/models/dto/account/BarteronRequest.h"

namespace PocketTx
{
    BarteronRequest::BarteronRequest() : AccountSetting()
    {
        SetType(TxType::ACCOUNT_BARTERON_REQUEST);
    }

    BarteronRequest::BarteronRequest(const CTransactionRef& tx) : AccountSetting(tx)
    {
        SetType(TxType::ACCOUNT_BARTERON_REQUEST);
    }

} // namespace PocketTx