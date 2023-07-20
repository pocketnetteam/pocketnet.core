// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include "pocketdb/repositories/web/BarteronRepository.h"

namespace PocketDb
{
    UniValue BarteronRepository::GetAccount(const string& address)
    {
        UniValue result(UniValue::VOBJ);

        SqlTransaction(__func__, [&]()
        {
            Sql(R"sql(
                
            )sql")
            .Bind(address)
            .Select([&](Cursor& cursor) {
                if (cursor.Step())
                {

                }
            });
        });

        return result;
    }

}