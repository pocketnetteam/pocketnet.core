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

        uiInterface.InitMessage(_("Checking SQLDB Migration: CreateBlockingList...").translated);

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

    void MigrationRepository::AddTransactionFirstField()
    {
        uiInterface.InitMessage(_("Checking SQLDB Migration: AddTransactionFirstField...").translated);

        TryTransactionStep(__func__, [&]()
        {
            if (ExistsRows(SetupSqlStatement("select 1 from pragma_table_info('Transactions') where name='First'")))
            {
              TryStepStatement(SetupSqlStatement("create index if not exists Transactions_Id_First on Transactions (Id, First)"));
              return;
            }
                
            // Add new field "First"
            TryStepStatement(SetupSqlStatement("alter table Transactions add column First int not null default 0"));

            // Update all "lasted" transactions for set First version
            TryStepStatement(
                SetupSqlStatement(R"sql(
                    update Transactions set First = 1
                    where Transactions.ROWID in (
                      select t.ROWID from Transactions t indexed by Transactions_Type_Last_Height_Id
                      where t.Type in (100,170,302,303,304,305,306,103,200,201,202,204,205,206,209,210,207)
                        and t.Last in (0,1)
                        and t.Height = (select min(f.Height) from Transactions f indexed by Transactions_Last_Id_Height where f.Last in (0,1) and f.Id=t.Id)
                    )
                )sql")
            );

            // Set First=0 for checkpointed transactions
            TryStepStatement(
                SetupSqlStatement(R"sql(
                  update Transactions set First = 0
                  where Hash in (
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
                )sql")
            );

            // Setup indexes
            TryStepStatement(SetupSqlStatement("create index if not exists Transactions_Id_First on Transactions (Id, First)"));
        });
    }

} // namespace PocketDb

