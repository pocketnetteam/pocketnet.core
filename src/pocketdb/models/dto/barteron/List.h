// Copyright (c) 2023 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETTX_BARTERON_List_H
#define POCKETTX_BARTERON_List_H

#include "pocketdb/models/base/SocialTransaction.h"

namespace PocketTx
{
    using namespace std;

    class BarteronList : public SocialTransaction
    {
    public:
        BarteronList();
        BarteronList(const CTransactionRef& tx);
        virtual ~BarteronList() = 0;

    protected:
        const optional<vector<int64_t>> ParseList(const optional<string>& list) const;

    };

} // namespace PocketTx

#endif // POCKETTX_POCKETTX_BARTERON_List_HAUDIO_H