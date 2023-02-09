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
                            j.AccountId,
                            j.Reason
                        from
                            Jury j,
                            flag
                        where
                            j.FlagRowId = flag.ROWID
                    ),
                    account as (
                        select
                            u.String1 as AddressHash
                        from
                            Transactions u indexed by Transactions_Id_First,
                            juryRec
                        where
                            u.Id = juryRec.AccountId and
                            u.First = 1
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
                    a.AddressHash,
                    j.Reason,
                    ifnull(jv.Verdict, -1)
                from
                    juryRec j
                    join account a
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

    UniValue ModerationRepository::GetAllJury()
    {
        UniValue result(UniValue::VARR);

        TryTransactionStep(__func__, [&]()
        {
            auto stmt = SetupSqlStatement(R"sql(
                with
                    juryRec as (
                        select
                            j.FlagRowId,
                            j.AccountId,
                            j.Reason
                        from
                            Jury j
                    ),
                    flag as (
                        select
                            f.Hash,
                            f.String3 as Address
                        from
                            Transactions f,
                            juryRec
                        where
                            f.ROWID = juryRec.FlagRowId
                    ),
                    juryVerd as (
                        select
                            jv.Verdict
                        from
                            juryVerdict jv,
                            juryRec j
                        where
                            jv.FlagRowId = j.FlagRowId
                    )
                select
                    f.Hash,
                    f.Address,
                    j.Reason,
                    ifnull(jv.Verdict, -1)
                from
                    juryRec j
                    join flag f
                    left join juryVerd jv
            )sql");

            while (sqlite3_step(*stmt) == SQLITE_ROW)
            {
                UniValue rcrd(UniValue::VOBJ);

                if (auto[ok, value] = TryGetColumnString(*stmt, 0); ok)
                    rcrd.pushKV("id", value);
                if (auto[ok, value] = TryGetColumnString(*stmt, 1); ok)
                    rcrd.pushKV("address", value);
                if (auto[ok, value] = TryGetColumnInt(*stmt, 2); ok)
                    rcrd.pushKV("reason", value);
                if (auto[ok, value] = TryGetColumnInt(*stmt, 3); ok && value > -1) rcrd.pushKV("verdict", value);

                result.push_back(rcrd);
            }

            FinalizeSqlStatement(*stmt);
        });

        return result;
    }

    vector<JuryContent> ModerationRepository::GetJuryAssigned(const string& address, bool verdict, const Pagination& pagination)
    {
        vector<JuryContent> result;

        TryTransactionStep(__func__, [&]()
        {
            string orderBy = " order by f.Height ";
            orderBy += (pagination.Desc ? " desc " : " asc ");

            auto stmt = SetupSqlStatement(R"sql(
                select

                    f.Hash as FlagHash,
                    f.Height as FlagHeight,
                    f.Int1 as Reason,
                    c.Id as ContentId,
                    c.Type as ContentType

                from Transactions u indexed by Transactions_Type_Last_String1_Height_Id
                cross join JuryModerators jm indexed by JuryModerators_AccountId_FlagRowId
                    on jm.AccountId = u.Id
                cross join Transactions f
                    on f.ROWID = jm.FlagRowId
                cross join Transactions c
                    on c.Hash = f.String2

                where u.Type in (100)
                  and u.Last = 1
                  and u.Height is not null
                  and u.String1 = ? 
                  and f.Height <= ?

                  and )sql" + string(verdict ? "" : "not") + R"sql( exists (
                     select 1
                     from Transactions v indexed by Transactions_Type_String1_String2_Height
                     where
                        v.Type = 420 and
                        v.String1 = u.String1 and
                        v.String2 = f.Hash and
                        v.Height > 0
                  )

                )sql" + orderBy + R"sql(

                limit ? offset ?
            )sql");

            TryBindStatementText(stmt, 1, address);
            TryBindStatementInt(stmt, 2, pagination.TopHeight);
            TryBindStatementInt(stmt, 3, pagination.PageSize);
            TryBindStatementInt(stmt, 4, pagination.PageStart);
            
            while (sqlite3_step(*stmt) == SQLITE_ROW)
            {
                UniValue record(UniValue::VOBJ);

                if (auto[ok, value] = TryGetColumnString(*stmt, 0); ok)
                    record.pushKV("juryid", value);
                if (auto[ok, value] = TryGetColumnInt64(*stmt, 1); ok)
                    record.pushKV("height", value);
                if (auto[ok, value] = TryGetColumnInt(*stmt, 2); ok)
                    record.pushKV("reason", value);

                auto[okContentId, contentId] = TryGetColumnInt64(*stmt, 3);
                auto[okContentType, contentType] = TryGetColumnInt(*stmt, 4);

                result.push_back(JuryContent{contentId, (TxType)contentType, record});
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
                    Transactions u indexed by Transactions_Type_Last_String1_Height_Id
                    cross join JuryBan b indexed by JuryBan_AccountId_Ending
                        on b.AccountId = u.Id
                    cross join Transactions v
                        on v.ROWID = b.VoteRowId
                    cross join Transactions f
                        on f.Hash = v.String2
                where
                    u.Type = 100 and
                    u.Last = 1 and
                    u.String1 = ? and
                    u.Height > 0
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