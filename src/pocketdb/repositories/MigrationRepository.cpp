// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include "pocketdb/repositories/MigrationRepository.h"
#include <node/ui_interface.h>
#include <util/translation.h>

namespace PocketDb
{
    bool MigrationRepository::NeedMigrate0_22()
    {
        // No need to migrate if there is empty db
        return TableExists("Transactions") && !ColumnExists("Transactions", "RegId1");
    }

    void MigrationRepository::Migrate0_21__0_22()
    {
        LogPrintf("Starting migration process, ETA: 24h\n");

        SqlTransaction(__func__, [&]()
        {
            auto t1 = GetSystemTimeInSeconds();
            uiInterface.InitMessage(_("Migration: Fulfilling Registry (1 of 12)...").translated);

            FulfillRegistry();

            auto t2 = GetSystemTimeInSeconds();
            LogPrintf("took %i seconds\n", t2 - t1);
            uiInterface.InitMessage(_("Migration: Fulfilling Chain (2 of 12)...").translated);

            FulfillChain();

            auto t3 = GetSystemTimeInSeconds();
            LogPrintf("took %i seconds\n", t3 - t2);
            uiInterface.InitMessage(_("Migration: Fulfilling Lists (3 of 12)...").translated);

            FulfillLists();

            auto t4 = GetSystemTimeInSeconds();
            LogPrintf("took %i seconds\n", t4 - t3);
            uiInterface.InitMessage(_("Migration: Fulfilling Last (4 of 12)...").translated);

            FulfillLast();

            auto t5 = GetSystemTimeInSeconds();
            LogPrintf("took %i seconds\n", t5 - t4);
            uiInterface.InitMessage(_("Migration: Fulfilling First(5 of 12)...").translated);

            FulfillFirst();

            auto t6 = GetSystemTimeInSeconds();
            LogPrintf("took %i seconds\n", t6 - t5);
            uiInterface.InitMessage(_("Migration: Fulfilling Transactions (6 of 12)...").translated);

            FulfillTransactions();

            auto t7 = GetSystemTimeInSeconds();
            LogPrintf("took %i seconds\n", t7 - t6);
            uiInterface.InitMessage(_("Migration: Fulfilling TxOutputs (7 of 12)...").translated);

            FulfillTxOutputs();

            auto t8 = GetSystemTimeInSeconds();
            LogPrintf("took %i seconds\n", t8 - t7);
            uiInterface.InitMessage(_("Migration: Fulfilling TxInputs (8 of 12)...").translated);

            FulfillTxInputs();

            auto t9 = GetSystemTimeInSeconds();
            LogPrintf("took %i seconds\n", t9 - t8);
            uiInterface.InitMessage(_("Migration: Fulfilling Balances (9 of 12)...").translated);

            FulfillBalances();

            auto t10 = GetSystemTimeInSeconds();
            LogPrintf("took %i seconds\n", t10 - t9);
            uiInterface.InitMessage(_("Migration: Fulfilling Ratings (10 of 12)...").translated);

            FulfillRatings();

            auto t11 = GetSystemTimeInSeconds();
            LogPrintf("took %i seconds\n", t11 - t10);
            uiInterface.InitMessage(_("Migration: Fulfilling Payload (11 of 12)...").translated);

            FulfillPayload();

            auto t12 = GetSystemTimeInSeconds();
            LogPrintf("took %i seconds\n", t12 - t11);
            uiInterface.InitMessage(_("Migration: Fulfilling other tables (BlockingLists, Jury, Badgets) (12 of 12)...").translated);

            FulfillOthers();

            auto t13 = GetSystemTimeInSeconds();
            LogPrintf("took %i seconds\n", t13 - t12);

            LogPrintf("Total migrating time: %is\n", t13 - t1);
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
                j.key,
                (select r.RowId from Registry r where r.String = j.value)
            from
                Transactions t,
                json_each(t.String3) j
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

