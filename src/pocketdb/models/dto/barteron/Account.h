// Copyright (c) 2023 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETTX_BARTERON_ACCOUNT_H
#define POCKETTX_BARTERON_ACCOUNT_H

#include "pocketdb/models/dto/barteron/List.h"

namespace PocketTx
{
    using namespace std;

    class BarteronAccount : public BarteronList
    {
    public:
        BarteronAccount();
        BarteronAccount(const CTransactionRef& tx);

        optional<string> GetPayloadTagsAdd() const;
        const optional<vector<int64_t>> GetPayloadTagsAddIds() const;

        optional<string> GetPayloadTagsDel() const;
        const optional<vector<int64_t>> GetPayloadTagsDelIds() const;

    };

} // namespace PocketTx

#endif // POCKETTX_POCKETTX_BARTERON_ACCOUNT_HAUDIO_H