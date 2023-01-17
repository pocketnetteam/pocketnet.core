// Copyright (c) 2023 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETTX_BARTERON_REQUEST_H
#define POCKETTX_BARTERON_REQUEST_H

#include "pocketdb/models/dto/account/Setting.h"

namespace PocketTx
{
    using namespace std;

    // TODO (barteron): inherit Transaction?
    class BarteronRequest : public AccountSetting
    {
    public:
        BarteronRequest();
        BarteronRequest(const CTransactionRef& tx);
    };

} // namespace PocketTx

#endif // POCKETTX_POCKETTX_BARTERON_REQUEST_HAUDIO_H