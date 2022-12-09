// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include "pocketdb/repositories/MigrationRepository.h"
#include <node/ui_interface.h>
#include <util/translation.h>

namespace PocketDb
{
    bool MigrationRepository::CreateBlockingList()
    {
        bool result = false;

        if (!CheckNeedCreateBlockingList())
            return true;

        uiInterface.InitMessage(_("SQLDB Migration: CreateBlockingList...").translated);

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
                from Transactions b
                join Chain bc
                    on bc.TxId = b.Id
                join Blocks bb
                    on bb.Id = bc.BlockId
                cross join Transactions us
                  on us.Type in (100, 170) and us.Int1 = b.Int1
                join Chain usc
                    on usc.TxId = us.Id
                    and usc.Last = 1
                    and usc.BlockId > 0 -- tx is in block (analog of Height > 0)
                cross join Transactions ut
                  on ut.Type in (100, 170)
                    and ut.Int1 in (select b.Int2 union select cast(value as int) from json_each(b.String1))
                join Chain utc
                    on utc.TxId = ut.Id
                    and utc.Last = 1
                join Blocks utb
                    on utb.Id = utc.BlockId
                    and utb.Height > 0
                where b.Type in (305)
                  and not exists (select 1 from Transactions bc
                                  join Chain bcc
                                    on bcc.TxId = bc.Id
                                  join Blocks bcb
                                    on bcb.Id = bcc.BlockId
                                  where bc.Type in (306)
                                    and bc.Last = 1
                                    and bc.Int1 = us.Int1
                                    and bc.Int2 = ut.Int1
                                    and bcb.Height >= bb.Height)
                  and not exists (select 1 from BlockingLists bl where bl.IdSource = usc.Id and bl.IdTarget = utc.Id)
            )sql")

        });

        return !CheckNeedCreateBlockingList();
    }

    bool MigrationRepository::CheckNeedCreateBlockingList()
    {
        bool result = false;

        uiInterface.InitMessage(_("Checking SQLDB Migration: CreateBlockingList...").translated);

        TryTransactionStep(__func__, [&]()
        {
            auto stmt = SetupSqlStatement(R"sql(
                select 1
                from Transactions b
                join Chain bc
                    on bc.TxId = b.Id
                join Blocks bb
                    on bb.Id = bc.BlockId
                    and bb.Height > 0
                join Transactions us
                    on us.Type in (100, 170)  and us.String1 = b.String1
                join Chain usc
                    on usc.TxId = us.Id
                    and usc.Last = 1
                    and usc.BlockId > 0 -- tx is in block (analog Height>0)
                join Transactions ut
                    on ut.Type in (100, 170)
                    and ut.Int1 in (select b.Int2 union select cast(value as int) from json_each(b.String1))
                join CHain utc
                    on utc.TxId = ut.Id
                    and utc.Last = 1
                join Blocks utb
                    on utb.Id = utc.BlockId
                    and utb.Height > 0
                where b.Type in (305)
                  and not exists (select 1 from Transactions bc
                                  join Chain bcc
                                    on bcc.TxId = bc.Id
                                  join Blocks bcb
                                    on bcb.Id = bcc.BlockId
                                  where bc.Type in (306)
                                    and bc.Last = 1
                                    and bc.Int1 = us.Int1
                                    and bc.Int2 = ut.Int1
                                    and bcb.Height >= bb.Height)
                  and not exists (select 1 from BlockingLists bl where bl.IdSource = us.Id and bl.IdTarget = ut.Id)
                limit 1
            )sql");

            result = (sqlite3_step(*stmt) == SQLITE_ROW);

            FinalizeSqlStatement(*stmt);
        });

        return result;
    }

} // namespace PocketDb

