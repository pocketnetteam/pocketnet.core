// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include "pocketdb/migrations/old_minimal.h"

namespace PocketDb
{
    PocketDbOldMinimalMigration::PocketDbOldMinimalMigration()
    {
        _requiredIndexes = R"sql(
            create index if not exists Transactions_Id_Last on Transactions (Id, Last);
        )sql";
    }
}
