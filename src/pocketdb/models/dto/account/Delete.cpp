// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include <primitives/transaction.h>
#include "pocketdb/models/dto/account/Delete.h"

namespace PocketTx
{
    AccountDelete::AccountDelete() : SocialTransaction()
    {
        SetType(TxType::ACCOUNT_DELETE);
    }

    AccountDelete::AccountDelete(const CTransactionRef& tx) : SocialTransaction(tx)
    {
        SetType(TxType::ACCOUNT_DELETE);
    }

    string AccountDelete::BuildHash()
    {
        std::string data;
        return Transaction::GenerateHash(data);
    }

} // namespace PocketTx

