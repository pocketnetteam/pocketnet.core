// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include "pocketdb/repositories/web/ModerationRepository.h"

namespace PocketDb
{
    void ModerationRepository::Init() {}

    void ModerationRepository::Destroy() {}

    UniValue ModerationRepository::GetJury(const string& jury)
    {
        UniValue result(UniValue::VOBJ);

        TryTransactionStep(__func__, [&]()
        {
            auto stmt = SetupSqlStatement(R"sql(
                with
                    flag as (
                        select
                            ROWID
                        from
                            Transactions
                        where
                            Hash = ?
                    ),
                    juryRec as (
                        select
                            j.AddressHash,
                            j.Reason
                        from
                            Jury j,
                            flag
                        where
                            j.FlagRowId = flag.ROWID
                    ),
                    juryVerd as (
                        select
                            jv.Verdict
                        from
                            juryVerdict jv,
                            flag
                        where
                            jv.FlagRowId = flag.ROWID
                    )
                select
                    j.AddressHash,
                    j.Reason,
                    ifnull(jv.Verdict, -1)
                from
                    juryRec j
                    left join juryVerd jv
            )sql");

            TryBindStatementText(stmt, 1, jury);
            
            if (sqlite3_step(*stmt) == SQLITE_ROW)
            {
                result.pushKV("id", jury);

                if (auto[ok, value] = TryGetColumnString(*stmt, 0); ok)
                    result.pushKV("address", value);
                if (auto[ok, value] = TryGetColumnInt(*stmt, 1); ok)
                    result.pushKV("reason", value);
                if (auto[ok, value] = TryGetColumnInt(*stmt, 2); ok && value > -1)
                {
                    result.pushKV("verdict", value);

                    // TODO (aok): add object with ban information if verditc == 1
                }
            }

            FinalizeSqlStatement(*stmt);
        });

        return result;
    }

    UniValue ModerationRepository::GetJuryAssigned(const string& address, const Pagination& pagination)
    {
        UniValue result(UniValue::VARR);

        TryTransactionStep(__func__, [&]()
        {
            string orderBy = " order by f.Height ";
            orderBy += (pagination.Desc ? " desc " : " asc ");

            auto stmt = SetupSqlStatement(R"sql(
                select f.Hash
                from Transactions u indexed by Transactions_Type_Last_String1_Height_Id
                cross join JuryModerators jm indexed by JuryModerators_AccountId_FlagRowId
                    on jm.AccountId = u.Id
                cross join Transactions f
                    on f.ROWID = jm.FlagRowId
                where u.Type in (100)
                  and u.Last = 1
                  and u.Height is not null
                  and u.String1 = ? 
                  and f.Height <= ?
                )sql" + orderBy + R"sql(
                limit ? offset ?
            )sql");

            TryBindStatementText(stmt, 1, address);
            TryBindStatementInt(stmt, 2, pagination.TopHeight);
            TryBindStatementInt(stmt, 3, pagination.PageSize);
            TryBindStatementInt(stmt, 4, pagination.PageStart);
            
            while (sqlite3_step(*stmt) == SQLITE_ROW)
            {
                if (auto[ok, value] = TryGetColumnInt(*stmt, 0); ok)
                    result.push_back(value);
            }

            FinalizeSqlStatement(*stmt);
        });

        return result;
    }

    UniValue ModerationRepository::GetJuryModerators(const string& jury)
    {
        UniValue result(UniValue::VARR);

        TryTransactionStep(__func__, [&]()
        {
            auto stmt = SetupSqlStatement(R"sql(
                with
                    flag as (
                        select
                            ROWID
                        from
                            Transactions
                        where
                            Hash = ?
                    ),
                    jurymod as (
                        select
                            jm.AccountId
                        from
                            JuryModerators jm,
                            flag
                        where
                            jm.FlagRowId = flag.ROWID
                    ),
                    moderators as (
                        select
                            u.String1 as Address
                        from
                            Transactions u indexed by Transactions_Id_First,
                            jurymod
                        where
                            u.Id = jurymod.AccountId and
                            u.First = 1
                    )

                select
                    m.Address
                from
                    moderators m
            )sql");

            TryBindStatementText(stmt, 1, jury);
            
            while (sqlite3_step(*stmt) == SQLITE_ROW)
            {
                if (auto[ok, value] = TryGetColumnString(*stmt, 0); ok)
                    result.push_back(value);
            }

            FinalizeSqlStatement(*stmt);
        });

        return result;
    }

    UniValue ModerationRepository::GetBans(const string& address)
    {
        UniValue result(UniValue::VARR);

        TryTransactionStep(__func__, [&]()
        {
            auto stmt = SetupSqlStatement(R"sql(
                select
                    f.Hash as JuryId,
                    f.Int1 as Reason,
                    b.Ending
                from
                    JuryBan b indexed by JuryBan_AddressHash_Ending
                    cross join Transactions v
                        on v.ROWID = b.VoteRowId
                    cross join Transactions f
                        on f.Hash = v.String2
                where
                    b.AddressHash = ?
            )sql");

            TryBindStatementText(stmt, 1, address);
            
            while (sqlite3_step(*stmt) == SQLITE_ROW)
            {
                UniValue record(UniValue::VOBJ);
                
                if (auto[ok, value] = TryGetColumnString(*stmt, 0); ok)
                    record.pushKV("juryId", value);
                if (auto[ok, value] = TryGetColumnInt(*stmt, 1); ok)
                    record.pushKV("reason", value);
                if (auto[ok, value] = TryGetColumnInt64(*stmt, 2); ok)
                    record.pushKV("ending", value);
                
                result.push_back(record);
            }

            FinalizeSqlStatement(*stmt);
        });

        return result;
    }

}