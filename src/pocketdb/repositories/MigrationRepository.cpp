// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include "pocketdb/repositories/MigrationRepository.h"
#include <ui_interface.h>

namespace PocketDb
{
    bool MigrationRepository::CreateBlockingList()
    {
        bool result = false;

        if (!CheckNeedCreateBlockingList())
            return true;

        uiInterface.InitMessage(_("SQLDB Migration: CreateBlockingList..."));

        TryTransactionBulk(__func__, {

            // Clear old data - this first init simple migration
            SetupSqlStatement(R"sql(
                delete from BlockingLists
            )sql"),

            // Insert new last values
            SetupSqlStatement(R"sql(
                insert into BlockingLists
                (
                    IdSource,
                    IdTarget
                )
                select distinct
                  us.Id,
                  ut.Id
                from Transactions b indexed by Transactions_Type_Last_String1_Height_Id
                cross join Transactions us indexed by Transactions_Type_Last_String1_Height_Id
                  on us.Type in (100, 170) and us.Last = 1 and us.String1 = b.String1 and us.Height > 0
                cross join Transactions ut indexed by Transactions_Type_Last_String1_Height_Id
                  on ut.Type in (100, 170) and ut.Last = 1
                    and ut.String1 in (select b.String2 union select value from json_each(b.String3))
                    and ut.Height > 0
                where b.Type in (305)
                  and b.Height > 0
                  and not exists (select 1 from Transactions bc indexed by Transactions_Type_Last_String1_String2_Height
                                  where bc.Type in (306)
                                    and bc.Last = 1
                                    and bc.String1 = us.String1
                                    and bc.String2 = ut.String1
                                    and bc.Height >= b.Height)
                  and not exists (select 1 from BlockingLists bl where bl.IdSource = us.Id and bl.IdTarget = ut.Id)
            )sql")

        });

        return !CheckNeedCreateBlockingList();
    }

    bool MigrationRepository::CheckNeedCreateBlockingList()
    {
        bool result = false;

        uiInterface.InitMessage(_("Checking SQLDB Migration: CreateBlockingList..."));

        TryTransactionStep(__func__, [&]()
        {
            auto stmt = SetupSqlStatement(R"sql(
                select 1
                from Transactions b indexed by Transactions_Type_Last_String1_Height_Id
                join Transactions us indexed by Transactions_Type_Last_String1_Height_Id
                  on us.Type in (100, 170) and us.Last = 1 and us.String1 = b.String1 and us.Height > 0
                join Transactions ut indexed by Transactions_Type_Last_String1_Height_Id
                  on ut.Type in (100, 170) and ut.Last = 1
                    and ut.String1 in (select b.String2 union select value from json_each(b.String3))
                    and ut.Height > 0
                where b.Type in (305)
                  and b.Height > 0
                  and not exists (select 1 from Transactions bc indexed by Transactions_Type_Last_String1_String2_Height
                                  where bc.Type in (306)
                                    and bc.Last = 1
                                    and bc.String1 = us.String1
                                    and bc.String2 = ut.String1
                                    and bc.Height >= b.Height)
                  and not exists (select 1 from BlockingLists bl where bl.IdSource = us.Id and bl.IdTarget = ut.Id)
                limit 1
            )sql");

            result = (sqlite3_step(*stmt) == SQLITE_ROW);

            FinalizeSqlStatement(*stmt);
        });

        return result;
    }

} // namespace PocketDb

