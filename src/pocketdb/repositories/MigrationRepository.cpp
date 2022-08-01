// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include "pocketdb/repositories/MigrationRepository.h"
#include <ui_interface.h>

namespace PocketDb
{
    bool MigrationRepository::SplitLikers()
    {
        if (!CheckNeedSplitLikers())
            return true;

        uiInterface.InitMessage(_("SQLDB Migration: SplitLikers..."));

        TryTransactionStep(__func__, [&]()
        {
            // delete exists data before insert splitted data
            auto stmtDelete = SetupSqlStatement(R"sql(
                delete from Ratings where Type in (101,102,103)
            )sql");
            TryStepStatement(stmtDelete);

            // Insert new splitted types of likers
            auto stmtSelect = SetupSqlStatement(R"sql(
                select
                
                    (
                
                        select
                            case sc.Type
                                when 300 then 101
                                when 301 then (
                                case
                                when c.String5 isnull then 102
                                else 103
                                end
                                )
                            end
                
                        from Transactions sc indexed by Transactions_Height_Type
                        join Transactions ul indexed by Transactions_Type_Last_String1_Height_Id
                            on ul.Type = 100 and ul.Last = 1 and ul.Height > 0 and ul.String1 = sc.String1
                        join Transactions c
                            on c.Hash = sc.String2
                            and (
                                (sc.Type in (300) and (sc.Time-c.Time) < ((case when sc.height < 322700 then 336 else 30 end) * 24 * 3600))
                                    or
                                (sc.Type in (301))
                            )
                        join Transactions ua indexed by Transactions_Type_Last_String1_Height_Id
                            on ua.Type = 100 and ua.Last = 1 and ua.Height > 0 and ua.String1 = c.String1
                
                        where sc.Type in (300,301)
                            and sc.Height = r.Height
                            and ((sc.Int1 in (4,5) and sc.Type = 300) or (sc.Int1 = 1 and sc.Type = 301))
                            and ua.Id = r.Id
                            and ul.Id = r.Value
                
                        and ((
                            sc.Type = 300
                            and (
                                select count()
                
                                from Transactions s indexed by Transactions_Type_String1_Height_Time_Int1
                                join Transactions c
                                    on c.Hash = s.String2
                                        and c.String1 = ua.String1
                                        and c.Type in (200,201,202,207)
                
                                where s.Type = sc.Type
                                    and s.Height <= sc.Height
                                    and s.String1 = sc.String1
                                    and s.Hash != sc.Hash
                    
                                    and s.Time < sc.Time
                                    and s.Time >= sc.Time - ((
                                        case
                                            when sc.Height >= 322700 then 2
                                            when sc.Height >= 292800 then 7
                                            when sc.Height >= 225000 then 1
                                            else 336
                                        end
                                    ) * 24 * 3600)
                    
                                    and s.Int1 in (4,5)
                            ) < (case when sc.Height >= 225000 then 2 else 99999 end)
                            )
                            or
                            (
                            sc.Type = 301
                            and (
                                select count()
                
                                from Transactions s indexed by Transactions_Type_String1_Height_Time_Int1
                                join Transactions c
                                    on c.Hash = s.String2
                                        and c.String1 = ua.String1
                                        and c.Type in (204,205,206)
                
                                where s.Type = sc.Type
                                    and s.String1 = sc.String1
                                    and s.Height <= sc.Height
                                    and s.Hash != sc.Hash
                
                                and s.Time < sc.Time
                                and s.Time >= sc.Time - ((
                                    case
                                        when sc.Height >= 322700 then 2
                                        when sc.Height >= 292800 then 7
                                        when sc.Height >= 225000 then 1
                                        else 336
                                    end
                                ) * 24 * 3600)
                
                                and s.Int1 = 1
                            ) < 20
                            )
                        )
                
                        order by sc.BlockNum
                        limit 1
                
                    ) lkrType
                
                    , 1
                    , r.Height
                    , r.Id
                    , r.Value
                
                from Ratings r
                where r.Type in (1)
            )sql");
            
            while (sqlite3_step(*stmtSelect) == SQLITE_ROW)
            {
                auto[okType, Type] = TryGetColumnInt(*stmtSelect, 0);
                auto[okLast, Last] = TryGetColumnInt(*stmtSelect, 1);
                auto[okHeight, Height] = TryGetColumnInt64(*stmtSelect, 2);
                auto[okId, Id] = TryGetColumnInt64(*stmtSelect, 3);
                auto[okValue, Value] = TryGetColumnInt64(*stmtSelect, 4);

                auto stmtInsert = SetupSqlStatement(R"sql(
                    insert into Ratings (type, last, height, id, value)
                    values (?,?,?,?,?)
                )sql");

                TryBindStatementInt(stmtInsert, 1, Type);
                TryBindStatementInt(stmtInsert, 2, Last);
                TryBindStatementInt64(stmtInsert, 3, Height);
                TryBindStatementInt64(stmtInsert, 4, Id);
                TryBindStatementInt64(stmtInsert, 5, Value);
                TryStepStatement(stmtInsert);
            }

            FinalizeSqlStatement(*stmtSelect);
        });

        return !CheckNeedSplitLikers();
    }

    bool MigrationRepository::CheckNeedSplitLikers()
    {
        bool result = false;

        uiInterface.InitMessage(_("Checking SQLDB Migration: SplitLikers..."));

        TryTransactionStep(__func__, [&]()
        {
            auto stmt = SetupSqlStatement(R"sql(
                select 'All', ifnull(sum(rAll.Id),0)sAllId, ifnull(sum(rAll.Value),0)sAllValue, count()cnt
                from Ratings rAll
                where rAll.Type in (1)

                union

                select 'Split', ifnull(sum(r.Id),0)sId, ifnull(sum(r.Value),0)sValue, count()cnt
                from Ratings r
                where r.Type in (101,102,103)
            )sql");

            if (sqlite3_step(*stmt) == SQLITE_ROW)
            {
                auto[sumAllIdOk, sumAllId] = TryGetColumnInt64(*stmt, 1);
                auto[sumAllValueOk, sumAllValue] = TryGetColumnInt64(*stmt, 2);
                auto[cntAllOk, cntAll] = TryGetColumnInt64(*stmt, 3);

                if (sqlite3_step(*stmt) == SQLITE_ROW)
                {
                    auto[sumSpltIdOk, sumSpltId] = TryGetColumnInt64(*stmt, 1);
                    auto[sumSpltValueOk, sumSpltValue] = TryGetColumnInt64(*stmt, 2);
                    auto[cntSpltOk, cntSplt] = TryGetColumnInt64(*stmt, 3);

                    result = (sumAllId != sumSpltId || sumAllValue != sumSpltValue || cntAll != cntSplt);

                }
            }

            FinalizeSqlStatement(*stmt);
        });

        return result;
    }

    bool MigrationRepository::AccumulateLikers()
    {
        bool result = false;

        if (!CheckNeedAccumulateLikers())
            return true;

        uiInterface.InitMessage(_("SQLDB Migration: AccumulateLikers..."));

        TryTransactionBulk(__func__, {

            // Clear old data - this first init simple migration
            SetupSqlStatement(R"sql(     
                delete from Ratings
                where Type in (111,112,113)
            )sql"),

            // Insert new last values
            SetupSqlStatement(R"sql(
                insert into Ratings (Type, Last, Height, Id, Value)
                select
                    (case r.Type when 101 then 111 when 102 then 112 when 103 then 113 end),
                    ifnull((select 0 from Ratings ll indexed by Ratings_Type_Id_Height_Value where ll.Type = r.Type and ll.Id = r.Id and ll.Height > r.Height limit 1),1),
                    r.Height,
                    r.Id,
                    (select count() from Ratings l indexed by Ratings_Type_Id_Height_Value where l.Type = r.Type and l.Id = r.Id and l.Height <= r.Height)
                from Ratings r
                where r.Type in (101,102,103)
                group by r.Type, r.Id, r.Height
            )sql")

        });

        return !CheckNeedAccumulateLikers();
    }

    bool MigrationRepository::CheckNeedAccumulateLikers()
    {
        bool result = false;

        uiInterface.InitMessage(_("Checking SQLDB Migration: AccumulateLikers..."));

        TryTransactionStep(__func__, [&]()
        {
            auto stmt = SetupSqlStatement(R"sql(
                select 1
                from Ratings r indexed by Ratings_Type_Id_Height_Value
                left join Ratings ll indexed by Ratings_Type_Id_Height_Value
                    on ll.Id = r.Id and ll.Type = (case r.Type when 101 then 111 when 102 then 112 when 103 then 113 end) and ll.Height = r.Height
                where r.Type in (101,102,103)
                  and (
                    (select count() from Ratings l where l.Id = r.Id and l.Height <= r.Height and l.Type = r.Type) != ll.Value
                    or
                    ll.Value isnull
                  )
                limit 1
            )sql");

            result = (sqlite3_step(*stmt) == SQLITE_ROW);

            FinalizeSqlStatement(*stmt);
        });

        return result;
    }

    // TODO (o1q): CheckNeedRefactorBlockingsLists


} // namespace PocketDb

