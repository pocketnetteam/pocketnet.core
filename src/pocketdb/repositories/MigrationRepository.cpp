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

        SqlTransaction(__func__, [&]()
        {

            // Clear old data - this first init simple migration
            Sql(R"sql(
                delete from BlockingLists
            )sql")
            .Run();

            // Insert new last values
            Sql(R"sql(
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
            .Run();
        });

        return !CheckNeedCreateBlockingList();
    }

    bool MigrationRepository::CheckNeedCreateBlockingList()
    {
        bool result = false;

        uiInterface.InitMessage(_("Checking SQLDB Migration: CreateBlockingList...").translated);

        SqlTransaction(__func__, [&]()
        {
            auto& stmt = Sql(R"sql(
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

            result = (stmt.Run() == SQLITE_ROW);
        });

        return result;
    }

    void MigrationRepository::AddTransactionFirstField()
    {
        uiInterface.InitMessage(_("Checking SQLDB Migration: AddTransactionFirstField...").translated);

        SqlTransaction(__func__, [&]()
        {
            if (ExistsRows(Sql("select 1 from First")))
            {
              return;
            }
                
            // Update all "lasted" transactions for set First version
            Sql(R"sql(
                insert into First (TxId)
                -- TODO (optimization): indexes
                select
                    t.RowId
                from Transactions t
                join Chain c on
                    c.TxId = t.RowId and
                    c.Height = (select min(f.Height) from Chain f where f.Uid = c.Uid)
                where
                    t.Type in (100,170,302,303,304,305,306,103,200,201,202,204,205,206,209,210,207)
            )sql").Run();

            // Set First=0 for checkpointed transactions
            Sql(R"sql(
                insert into First (TxId)
                select
                    t.RowId
                from Transactions t
                where
                    t.HashId = (
                        select r.RowId from Registry r where r.String in (
                            '30064229865e5f18d51725e3c3339facae0bfecd35b0e5a92e9dc1a168748439',
                            '45ad6c18c67717ab02c658e5153cbc97e3e7be9ed964739bb3a6181f89973b52',
                            '34d3db603ac0ba65960a3ef2508967ab4a9fc5e55a3c637af7aa50580a0f32dd',
                            'a925f4e6bcfe70199692e581847e7fe75f120b70fffe4818eadf0b931680c128',
                            '5a4d4aac2235262d2776c4f49d56b2b7291d44b4912a96997c5341daa460613b',
                            'd8bb6ca1fd5e9f6738f3678cd781d0b52e74b60acbcf532fb05036f5c9572b64',
                            'ab686cb7bc2b5385f35f7c0ab53002eae05d0799e31858fc541be43a6f0c293b',
                            'bdd4db4904edc131c19348843313f3e9869ba3998b796d03ce620e1cbaa5ecee',
                            '6778f1b9a502d128139517285dc75a11650b1587e78def91cb552299970dd9cc',
                            'da9040c6257f4d7021889ddffad3b1513be1c000fd219b833ef13cc93d53899b',
                            '7fcc2788bb08fb0de97fe4983a4cf9c856ce68a348d6cdb91459fe7a371f90f8',
                            '2b53e7fce1965bd9409de61ff8c47c600d6fee82f5ba5bc9c70ce97d72305a78',
                            '7321445c16fe641c56606d377a29a24d117f2849197bd7bd32054e906401fb8e'
                        )
                    )
            )sql").Run();
        });
    }

} // namespace PocketDb

