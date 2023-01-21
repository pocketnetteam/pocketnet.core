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
                  usc.Uid,
                  utc.Uid
                from Transactions b
                join Chain bc
                    on bc.TxId = b.RowId
                cross join Transactions us
                  on us.Type in (100, 170) and us.RegId1 = b.RegId1
                join Chain usc
                    on usc.TxId = us.RowId
                join Last usl
                    on usl.TxId = us.RowId
                cross join Transactions ut
                  on ut.Type in (100, 170)
                    and ut.RegId1 in (select b.RegId2 union select l.RegId from Lists l where l.TxId = b.RowId)
                join Chain utc
                    on utc.TxId = ut.RowId
                join Last utl
                    on utl.TxId = ut.RowId
                where b.Type in (305)
                  and not exists (select 1 from Transactions bc
                                  join Chain bcc
                                    on bcc.TxId = bc.RowId
                                  join Last bcl
                                    on bcl.TxId = bc.RowId
                                  where bc.Type in (306)
                                    and bc.RegId1 = us.RegId1
                                    and bc.RegId2 = ut.RegId1
                                    and bcc.Height >= bc.Height)
                  and not exists (select 1 from BlockingLists bl where bl.IdSource = usc.Uid and bl.IdTarget = utc.Uid)
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
                    on bc.TxId = b.RowId
                join Transactions us
                    on us.Type in (100, 170) and us.RegId1 = b.RegId1
                join Last l
                    on l.TxId = us.RowId
                join Chain usc
                    on usc.TxId = us.RowId
                join Last usl
                    on usl.TxId = us.RowId
                join Transactions ut
                    on ut.Type in (100, 170)
                    and ut.RegId1 in (select b.RegId2 union select l.RegId from Lists l where l.TxId = b.RowId)
                join Chain utc
                    on utc.TxId = ut.RowId
                where b.Type in (305)
                  and not exists (select 1 from Transactions bb
                                  join Chain bbc
                                    on bbc.TxId = bb.RowId
                                  join Last bbl
                                    on bbl.TxId = bb.RowId
                                  where bb.Type in (306)
                                    and bb.RegId1 = us.RegId1
                                    and bb.RegId2 = ut.RegId1
                                    and bc.Height >= bbc.Height)
                  and not exists (select 1 from BlockingLists bl where bl.IdSource = usc.Uid and bl.IdTarget = utc.Uid)
                limit 1
            )sql");

            result = (stmt->Step() == SQLITE_ROW);
        });

        return result;
    }

} // namespace PocketDb

