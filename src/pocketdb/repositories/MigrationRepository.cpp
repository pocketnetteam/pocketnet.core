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
            bool exists = false;
            Sql("select 1 from First")
            .Select([&](Cursor& cursor)
            {
                exists = cursor.Step();
            });
            if (exists)
                return;
                
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
                    t.Type in (100,170,302,303,304,305,306,103,200,201,202,204,205,206,209,210,211,220,207)
            )sql")
            .Run();

            // Set First=0 for checkpointed transactions
            Sql(R"sql(
                insert into First (TxId)
                select
                    t.RowId
                from Transactions t
                where
                    t.RowId = (
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

    bool MigrationRepository::NeedMigrate0_22()
    {
        // No need to migrate if there is empty db
        return TableExists("Transactions") && !ColumnExists("Transactions", "RegId1");
    }

    void MigrationRepository::Migrate0_21__0_22()
    {
        SqlTransaction(__func__, [&]()
        {
            auto t1 = GetSystemTimeInSeconds();
            LogPrintf("Fulfilling Registry...\n");

            FulfillRegistry();

            auto t2 = GetSystemTimeInSeconds();
            LogPrintf("took %i seconds\n", t2 - t1);
            LogPrintf("Fulfilling Chain...\n");

            FulfillChain();

            auto t3 = GetSystemTimeInSeconds();
            LogPrintf("took %i seconds\n", t3 - t2);
            LogPrintf("Fulfilling Lists...\n");

            FulfillLists();

            auto t4 = GetSystemTimeInSeconds();
            LogPrintf("took %i seconds\n", t4 - t3);
            LogPrintf("Fulfilling Last...\n");

            FulfillLast();

            auto t5 = GetSystemTimeInSeconds();
            LogPrintf("took %i seconds\n", t5 - t4);
            LogPrintf("Fulfilling First...\n");

            FulfillFirst();

            auto t6 = GetSystemTimeInSeconds();
            LogPrintf("took %i seconds\n", t6 - t5);
            LogPrintf("Fulfilling Transactions...\n");

            FulfillTransactions();

            auto t7 = GetSystemTimeInSeconds();
            LogPrintf("took %i seconds\n", t7 - t6);
            LogPrintf("Fulfilling TxOutputs...\n");

            FulfillTxOutputs();

            auto t8 = GetSystemTimeInSeconds();
            LogPrintf("took %i seconds\n", t8 - t7);
            LogPrintf("Fulfilling TxInputs...\n");

            FulfillTxInputs();

            auto t9 = GetSystemTimeInSeconds();
            LogPrintf("took %i seconds\n", t9 - t8);
            LogPrintf("Fulfilling Balances...\n");

            FulfillBalances();

            auto t10 = GetSystemTimeInSeconds();
            LogPrintf("took %i seconds\n", t10 - t9);
            LogPrintf("Fulfilling Ratings...\n");

            FulfillRatings();

            auto t11 = GetSystemTimeInSeconds();
            LogPrintf("took %i seconds\n", t11 - t10);
            LogPrintf("Fulfilling Payload...\n");

            FulfillPayload();

            auto t12 = GetSystemTimeInSeconds();
            LogPrintf("took %i seconds\n", t12 - t11);
            LogPrintf("Fulfilling other tables (BlockingLists, Jury, Badgets)...\n");

            FulfillOthers();

            auto t13 = GetSystemTimeInSeconds();
            LogPrintf("took %i seconds\n", t13 - t12);

            LogPrintf("Total migrating time: %is", t13 - t1);
        });
    }

    void MigrationRepository::FulfillRegistry()
    {
        Sql(R"sql(
            insert or ignore into newdb.Registry (String)
            select Hash from Transactions
            union
            select BlockHash from Transactions
            union
            select String1 from Transactions
            union
            select String2 from Transactions
            union
            select iif(json_valid(String3), (select value from json_each(String3)), String3) from Transactions
            union
            select String4 from Transactions
            union
            select String5 from Transactions

        )sql")
        .Run();

        Sql(R"sql(
            insert or ignore into Registry (String)
            select AddressHash from TxOutputs
            union
            select ScriptPubKey from TxOutputs
        )sql")
        .Run();

        // TODO (losty): need SpentTxHash from TxInputs???
    }

    void MigrationRepository::FulfillLists()
    {
        Sql(R"sql(
            insert into newdb.Lists
            (TxId, OrderIndex, RegId)
            select
                h.RowId,
                0,
                (select value from json_each(t.String3))
            from
                Transactions t
                cross join newdb.Registry h on
                    h.String = t.Hash
            where
                t.Type = 305 and
                json_valid(t.String3)
        )sql")
        .Run();
    }

    void MigrationRepository::FulfillChain()
    {
        Sql(R"sql(
            insert into newdb.Chain
            (TxId, BlockId, BlockNum, Height, Uid)
            select
                h.RowId,
                (select r.RowId from Registry r where r.String = t.BlockHash),
                t.BlockNum,
                t.Height,
                t.Id
            from
                Transactions t
                cross join newdb.Registry h on
                    h.String = t.Hash
            where
                t.Height > 0
        )sql")
        .Run();
    }

    void MigrationRepository::FulfillLast()
    {
        Sql(R"sql(
            insert into newdb.Last (TxId)
            select
                h.RowId
            from
                Transactions t
                cross join newdb.Registry h on
                    h.String = t.Hash
            where
                t.Last = 1;
        )sql")
        .Run();
    }

    void MigrationRepository::FulfillFirst()
    {
        Sql(R"sql(
            insert into newdb.First (TxId)
            select
                h.RowId
            from
                Transactions t
                cross join newdb.Registry h
                    on h.String = t.Hash
            where
                t.First = 1;
        )sql")
        .Run();
    }

    void MigrationRepository::FulfillTransactions()
    {
        Sql(R"sql(
            insert into newdb.Transactions
            (RowId, Type, Time, RegId1, RegId2, RegId3, RegId4, RegId5, Int1)
            select 
                (select r.RowId from newdb.Registry r where r.String = t.Hash),
                t.Type,
                Time,
                (select r.RowId from newdb.Registry r where r.String = t.String1),
                (select r.RowId from newdb.Registry r where r.String = t.String2),
                iif(json_valid(t.String3), null, (select r.RowId from newdb.Registry r where r.String = t.String3)),
                (select r.RowId from newdb.Registry r where r.String = t.String4),
                (select r.RowId from newdb.Registry r where r.String = t.String5),
                Int1
            from
                Transactions t
        )sql")
        .Run();
    }

    void MigrationRepository::FulfillTxOutputs()
    {
        Sql(R"sql(
            insert into newdb.TxOutputs
            (TxId, Number, AddressId, Value, ScriptPubKeyId)
            select
                (select r.RowId from newdb.Registry r where r.String = o.TxHash),
                o.Number,
                (select r.RowId from newdb.Registry r where r.String = o.AddressHash),
                o.Value,
                (select r.RowId from newdb.Registry r where r.String = o.ScriptPubKey)
            from
                TxOutputs o
        )sql")
        .Run();
    }

    void MigrationRepository::FulfillRatings()
    {
        Sql(R"sql(
            insert into newdb.Ratings
            (Type, Last, Height, Uid, Value)
            select
                r.Type,
                r.Last,
                r.Height,
                r.Id,
                r.Value
            from
                Ratings r
        )sql")
        .Run();
    }

    void MigrationRepository::FulfillBalances()
    {
        Sql(R"sql(
            insert into newdb.Balances
            (AddressId, Value)
            select
                (select r.RowId from newdb.Registry r where r.String = b.AddressHash),
                b.Value
            from
                Balances b
            where
                b.Last = 1
        )sql")
        .Run();
    }

    void MigrationRepository::FulfillTxInputs()
    {
        Sql(R"sql(
            insert or ignore into newdb.TxInputs
            (SpentTxId, TxId, Number)
            select
                (select r.RowId from newdb.Registry r where r.String = i.SpentTxHash),
                (select r.RowId from newdb.Registry r where r.String = i.TxHash),
                i.Number
            from
                TxInputs i
        )sql")
        .Run();
    }

    void MigrationRepository::FulfillPayload()
    {
        Sql(R"sql(
            insert into newdb.Payload
            (TxId, String1, String2, String3, String4 ,String5, String6, String7, Int1)
            select
                (select r.RowId from newdb.Registry r where r.String = p.TxHash),
                String1,
                String2,
                String3,
                String4,
                String5,
                String6,
                String7,
                Int1
            from
                Payload p
        )sql")
        .Run();
    }

    void MigrationRepository::FulfillOthers()
    {
        Sql(R"sql(
            insert into newdb.BlockingLists
            (IdSource, IdTarget)
            select
                (select r.RowId from Registry r where r.String = s.String1),
                (select r.RowId from Registry r where r.String = t.String1)
            from
                BlockingLists
                cross join Transactions s indexed by Transactions_Id_Last on
                    s.Id = IdSource and 
                    s.Last = 1
                cross join Transactions t indexed by Transactions_Id_Last on
                    t.Id = IdTarget and
                    t.Last = 1
        )sql")
        .Run();

        Sql(R"sql(
            insert into newdb.Jury
            (FlagRowId, AccountId, Reason)
            select
                (select r.RowId from newdb.Registry r where r.String = ft.Hash),
                (select r.RowId from newdb.Registry r where r.String = at.Hash),
                Reason
            from
                Jury
                cross join Transactions ft on
                    ft.rowid = FlagRowId
                cross join Transactions at on
                    at.rowid = AccountId
        )sql")
        .Run();

        Sql(R"sql(
            insert into newdb.JuryVerdict
            (FlagRowId, VoteRowId, Verdict)
            select
                (select r.RowId from newdb.Registry r where r.String = ft.Hash),
                (select r.RowId from newdb.Registry r where r.String = vt.Hash),
                Verdict
            from
                JuryVerdict
                cross join Transactions ft on
                    ft.rowid = FlagRowId
                cross join Transactions vt on
                    vt.rowid = VoteRowId
        )sql")
        .Run();

        Sql(R"sql(
            insert into newdb.JuryModerators
            (FlagRowId, AccountId)
            select
                (select r.RowId from newdb.Registry r where r.String = ft.Hash),
                (select r.RowId from newdb.Registry r where r.String = at.Hash)
            from
                JuryModerators
                cross join Transactions ft on
                    ft.rowid = FlagRowId
                cross join Transactions at on
                    at.rowid = AccountId
        )sql")
        .Run();

        Sql(R"sql(
            insert into newdb.JuryBan
            (VoteRowId, AccountId, Ending)
            select
                (select r.RowId from newdb.Registry r where r.String = vt.Hash),
                (select r.RowId from newdb.Registry r where r.String = at.Hash),
                Ending
            from
                JuryBan
                cross join Transactions vt on
                    vt.rowid = VoteRowId
                cross join Transactions at on
                    at.rowid = AccountId
        )sql")
        .Run();

        Sql(R"sql(
            insert into newdb.Badges
            (AccountId, Badge, Cancel, Height)
            select
                AccountId,
                Badge,
                Cancel,
                Height
            from
                Badges
        )sql")
        .Run();
    }

    bool MigrationRepository::TableExists(const string& tableName)
    {
        bool result = false;

        SqlTransaction(__func__, [&]()
        {
            Sql(R"sql(
                select
                    1
                from
                    pragma_table_info(?)
            )sql")
            .Bind(tableName)
            .Select([&](Cursor& cursor) {
                result = cursor.Step();
            });
        });

        return result;
    }

    bool MigrationRepository::ColumnExists(const string& tableName, const string& columnName)
    {
        bool result = false;

        SqlTransaction(__func__, [&]()
        {
            Sql(R"sql(
                select
                    1
                from
                    pragma_table_info(?)
                where
                    name = ?
            )sql")
            .Bind(tableName, columnName)
            .Select([&](Cursor& cursor) {
                result = cursor.Step();
            });
        });

        return result;
    }
} // namespace PocketDb

