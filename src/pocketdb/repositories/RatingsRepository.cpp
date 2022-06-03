// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include "pocketdb/repositories/RatingsRepository.h"

namespace PocketDb
{
    void RatingsRepository::InsertRatings(shared_ptr<vector<Rating>> ratings)
    {
        for (const auto& rating: *ratings)
        {
            switch (*rating.GetType())
            {
            case RatingType::ACCOUNT_LIKERS:
            case RatingType::ACCOUNT_LIKERS_POST:
            case RatingType::ACCOUNT_LIKERS_COMMENT_ROOT:
            case RatingType::ACCOUNT_LIKERS_COMMENT_ANSWER:
            case RatingType::ACCOUNT_DISLIKERS_COMMENT_ANSWER:
                InsertLiker(rating);
                break;
            default:
                InsertRating(rating);
                break;
            }
        }
    }

    bool RatingsRepository::ExistsLiker(int addressId, int likerId)
    {
        bool result = false;

        TryTransactionStep(__func__, [&]()
        {
            auto stmt = SetupSqlStatement(R"sql(
                select 1
                from Ratings indexed by Ratings_Type_Id_Value
                where Type in (1)
                  and Id = ?
                  and Value = ?
            )sql");

            TryBindStatementInt(stmt, 1, addressId);
            TryBindStatementInt(stmt, 2, likerId);

            if (sqlite3_step(*stmt) == SQLITE_ROW)
                result = true;

            FinalizeSqlStatement(*stmt);
        });

        return result;
    }

    // TODO (brangr): implement height and additional types
    bool RatingsRepository::ExistsLiker(int addressId, int likerId, int height)
    {
        bool result = false;

        TryTransactionStep(__func__, [&]()
        {
            auto stmt = SetupSqlStatement(R"sql(
                select 1
                from Ratings indexed by Ratings_Type_Id_Value
                where Type in (11,12,13)
                  and Id = ?
                  and Value = ?
            )sql");

            TryBindStatementInt(stmt, 1, addressId);
            TryBindStatementInt(stmt, 2, likerId);

            if (sqlite3_step(*stmt) == SQLITE_ROW)
                result = true;

            FinalizeSqlStatement(*stmt);
        });

        return result;
    }

    void RatingsRepository::InsertRating(const Rating& rating)
    {
        TryTransactionStep(__func__, [&]()
        {
            // Insert new Last record
            auto stmt = SetupSqlStatement(R"sql(
                INSERT OR FAIL INTO Ratings (
                    Type,
                    Last,
                    Height,
                    Id,
                    Value
                ) SELECT ?,1,?,?,
                    ifnull((
                        select r.Value
                        from Ratings r indexed by Ratings_Type_Id_Last_Height
                        where r.Type = ?
                            and r.Last = 1
                            and r.Id = ?
                            and r.Height < ?
                        limit 1
                    ), 0) + ?
            )sql");
            TryBindStatementInt(stmt, 1, *rating.GetType());
            TryBindStatementInt(stmt, 2, rating.GetHeight());
            TryBindStatementInt64(stmt, 3, rating.GetId());
            TryBindStatementInt(stmt, 4, *rating.GetType());
            TryBindStatementInt64(stmt, 5, rating.GetId());
            TryBindStatementInt(stmt, 6, rating.GetHeight());
            TryBindStatementInt64(stmt, 7, rating.GetValue());
            TryStepStatement(stmt);

            // Clear old Last record
            auto stmtUpdate = SetupSqlStatement(R"sql(
                update Ratings indexed by Ratings_Type_Id_Last_Height
                  set Last = 0
                where Type = ?
                  and Last = 1
                  and Id = ?
                  and Height < ?
            )sql");
            TryBindStatementInt(stmtUpdate, 1, *rating.GetType());
            TryBindStatementInt64(stmtUpdate, 2, rating.GetId());
            TryBindStatementInt(stmtUpdate, 3, rating.GetHeight());
            TryStepStatement(stmtUpdate);
        });
    }

    void RatingsRepository::InsertLiker(const Rating& rating)
    {
        TryTransactionStep(__func__, [&]()
        {
            auto stmtInsert = SetupSqlStatement(R"sql(
                insert or fail into Ratings (
                    Type,
                    Last,
                    Height,
                    Id,
                    Value
                ) values ( ?,1,?,?,? )
            )sql");
            TryBindStatementInt(stmtInsert, 1, *rating.GetType());
            TryBindStatementInt(stmtInsert, 2, rating.GetHeight());
            TryBindStatementInt64(stmtInsert, 3, rating.GetId());
            TryBindStatementInt64(stmtInsert, 4, rating.GetValue());
            TryStepStatement(stmtInsert);

            // Set last count value
            int type = -1;
            switch (*rating.GetType())
            {
                case RatingType::ACCOUNT_LIKERS_POST:
                    type = RatingType::ACCOUNT_LIKERS_POST_LAST;
                    break;
                case RatingType::ACCOUNT_LIKERS_COMMENT_ROOT:
                    type = RatingType::ACCOUNT_LIKERS_COMMENT_ROOT_LAST;
                    break;
                case RatingType::ACCOUNT_LIKERS_COMMENT_ANSWER:
                    type = RatingType::ACCOUNT_LIKERS_COMMENT_ANSWER_LAST;
                    break;
                // Other types not accumulate
                default:
                    return;
            }

            // Init first last value record
            auto stmtInsertLast = SetupSqlStatement(R"sql(
                insert or fail into Ratings (Type, Last, Height, Id, Value)
                select
                    ?, -- Type
                    1, -- Last
                    ?, -- Height
                    ?, -- Id
                    0  -- Init Value
                where not exists (
                    select 1
                    from Ratings r
                    where r.Type = ?
                    and r.Id = ?
                    and r.Last = 1
                )
            )sql");
            TryBindStatementInt(stmtInsertLast, 1, type);
            TryBindStatementInt(stmtInsertLast, 2, rating.GetHeight());
            TryBindStatementInt64(stmtInsertLast, 3, rating.GetId());
            TryBindStatementInt(stmtInsertLast, 4, type);
            TryBindStatementInt64(stmtInsertLast, 5, rating.GetId());
            TryStepStatement(stmtInsertLast);

            // Increase last value
            auto stmtUpdateLast = SetupSqlStatement(R"sql(
                update Ratings set
                    Value = (
                        1 + ifnull((
                            select r.Value
                            from Ratings r
                            where r.Type = ?
                              and r.Id = ?
                              and r.Last = 1
                            limit 1
                        ),0)
                    )
                    , Height = ?
                where Type = ?
                and Id = ?
                and Last = 1
            )sql");
            TryBindStatementInt(stmtUpdateLast, 1, type);
            TryBindStatementInt64(stmtUpdateLast, 2, rating.GetId());
            TryBindStatementInt(stmtUpdateLast, 3, rating.GetHeight());
            TryBindStatementInt(stmtUpdateLast, 4, type);
            TryBindStatementInt64(stmtUpdateLast, 5, rating.GetId());
            TryStepStatement(stmtUpdateLast);
        });
    }
}
