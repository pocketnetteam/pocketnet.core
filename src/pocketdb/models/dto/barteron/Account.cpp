// Copyright (c) 2023 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include <primitives/transaction.h>
#include "pocketdb/models/dto/barteron/Account.h"

namespace PocketTx
{
    BarteronAccount::BarteronAccount() : SocialTransaction()
    {
        SetType(TxType::BARTERON_ACCOUNT);
    }

    BarteronAccount::BarteronAccount(const CTransactionRef& tx) : SocialTransaction(tx)
    {
        SetType(TxType::BARTERON_ACCOUNT);
    }

    const optional<vector<int64_t>>& BarteronAccount::_parse_list(const optional<string>& list) const
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

    optional<string> BarteronAccount::GetPayloadTagsAdd() const { return GetPayload() ? GetPayload()->GetString4() : nullopt; }

    const optional<vector<int64_t>>& BarteronAccount::GetPayloadTagsAddIds() const
    {
        return _parse_list(GetPayloadTagsAdd());
    }

    optional<string> BarteronAccount::GetPayloadTagsDel() const { return GetPayload() ? GetPayload()->GetString5() : nullopt; }

    const optional<vector<int64_t>>& BarteronAccount::GetPayloadTagsDelIds() const
    {
        return _parse_list(GetPayloadTagsDel());
    }

} // namespace PocketTx