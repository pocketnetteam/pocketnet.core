// Copyright (c) 2023 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include <primitives/transaction.h>
#include "pocketdb/models/dto/barteron/List.h"

namespace PocketTx
{
    BarteronList::BarteronList() : SocialTransaction() {}
    BarteronList::BarteronList(const CTransactionRef& tx) : SocialTransaction(tx) {}
    BarteronList::~BarteronList() {}

    const optional<vector<int64_t>> BarteronList::ParseList(const optional<string>& list) const
    {
        if (!list)
            return nullopt;

        vector<int64_t> ids;
        UniValue arr(UniValue::VARR);
        arr.read(*list);
        for (size_t i = 0; i < arr.size(); ++i)
        {
            if (!arr.At(i).isNum())
                return nullopt;

            ids.emplace_back(arr[i].get_int64());
        }

        if (ids.size() <= 0)
            return nullopt;

        return ids;
    }

} // namespace PocketTx