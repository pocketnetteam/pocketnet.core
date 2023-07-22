// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include "pocketdb/repositories/web/BarteronRepository.h"

namespace PocketDb
{
    vector<string> BarteronRepository::GetAccountIds(const vector<string>& addresses)
    {
        vector<string> result;

        SqlTransaction(__func__, [&]()
        {
            Sql(R"sql(
                with
                addr as (
                    select
                        RowId as id
                    from
                        Registry
                    where
                        String in ( )sql" + join(vector<string>(addresses.size(), "?"), ",") + R"sql( )
                )
                select
                    (select r.String from Registry r where r.RowId = a.HashId)
                from
                    addr
                cross join
                    Transactions a
                        on a.Type in (104) and a.RegId1 = addr.id
                cross join
                    Last l
                        on l.TxId = a.RowId
            )sql")
            .Bind(addresses)
            .Select([&](Cursor& cursor) {
                while (cursor.Step())
                {
                    if (auto[ok, value] = cursor.TryGetColumnString(0); ok)
                        result.push_back(value);
                }
            });
        });

        return result;
    }

}