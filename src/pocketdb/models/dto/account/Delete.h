// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETTX_ACCOUNT_DELETE_H
#define POCKETTX_ACCOUNT_DELETE_H

#include "pocketdb/models/base/SocialTransaction.h"

namespace PocketTx
{
    class AccountDelete : public SocialTransaction
    {
    public:
        AccountDelete();
        AccountDelete(const CTransactionRef& tx);

        string BuildHash() override;
    }; // class AccountDelete

} // namespace PocketTx

#endif // POCKETTX_ACCOUNT_DELETE_H
