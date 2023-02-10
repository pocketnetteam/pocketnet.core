// Copyright (c) 2023 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETTX_BARTERON_ACCOUNT_H
#define POCKETTX_BARTERON_ACCOUNT_H

#include "pocketdb/models/base/SocialTransaction.h"

namespace PocketTx
{
    using namespace std;

    class BarteronAccount : public SocialTransaction
    {
    public:
        BarteronAccount();
        BarteronAccount(const CTransactionRef& tx);

        /*
        Use p.s4 for JSON payload
        {
            ...
            "t": { // Tags
                "a": [ 1, 2, 3, .. ] // Append tags,
                "r": [ 4, 5, 6, .. ] // Remove tags
            }
            ...
        }
        */
    };

} // namespace PocketTx

#endif // POCKETTX_POCKETTX_BARTERON_ACCOUNT_HAUDIO_H