// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include "pocketdb/repositories/RatingsRepository.h"

namespace PocketDb
{
    void RatingsRepository::InsertRatings(shared_ptr<vector<Rating>> ratings)
    {
        for (const auto& rating: *ratings)
        {
            if (*rating.GetType() == RatingType::RATING_ACCOUNT_LIKERS)
                InsertLiker(rating);
            else
                InsertRating(rating);
        }
    }

    bool RatingsRepository::ExistsLiker(int addressId, int likerId, int height)
    {
        bool result = false;

        TryTransactionStep(__func__, [&]()
        {
            auto stmt = SetupSqlStatement(R"sql(
                select count(*)
                from Ratings
                where Type = ?
                  and Id = ?
                  and Value = ?
            )sql");

            TryBindStatementInt(stmt, 1, RatingType::RATING_ACCOUNT_LIKERS);
            TryBindStatementInt(stmt, 2, addressId);
            TryBindStatementInt(stmt, 3, likerId);

            if (sqlite3_step(*stmt) == SQLITE_ROW)
                if (auto[ok, value] = TryGetColumnInt(*stmt, 0); ok)
                    result = (value > 0);

            FinalizeSqlStatement(*stmt);
        });

        return result;
    }

    void RatingsRepository::InsertRating(const Rating& rating)
    {
        TryTransactionStep(__func__, [&]()
        {
            auto stmt = SetupSqlStatement(R"sql(
                INSERT OR FAIL INTO Ratings (
                    Type,
                    Height,
                    Id,
                    Value
                ) SELECT ?,?,?,
                    ifnull((
                        select r.Value
                        from Ratings r
                        where r.Type = ?
                            and r.Id = ?
                            and r.Height < ?
                        order by r.Height desc
                        limit 1
                    ), 0) + ?
            )sql");

            TryBindStatementInt(stmt, 1, rating.GetTypeInt());
            TryBindStatementInt(stmt, 2, rating.GetHeight());
            TryBindStatementInt64(stmt, 3, rating.GetId());
            TryBindStatementInt(stmt, 4, rating.GetTypeInt());
            TryBindStatementInt64(stmt, 5, rating.GetId());
            TryBindStatementInt(stmt, 6, rating.GetHeight());
            TryBindStatementInt64(stmt, 7, rating.GetValue());

            TryStepStatement(stmt);
        });
    }

    void RatingsRepository::InsertLiker(const Rating& rating)
    {
        TryTransactionStep(__func__, [&]()
        {
            auto stmt = SetupSqlStatement(R"sql(
                INSERT OR FAIL INTO Ratings (
                    Type,
                    Height,
                    Id,
                    Value
                ) SELECT ?,?,?,?
                WHERE NOT EXISTS (select 1 from Ratings r where r.Type=? and r.Id=? and r.Value=?)
            )sql");

            TryBindStatementInt(stmt, 1, rating.GetTypeInt());
            TryBindStatementInt(stmt, 2, rating.GetHeight());
            TryBindStatementInt64(stmt, 3, rating.GetId());
            TryBindStatementInt64(stmt, 4, rating.GetValue());
            TryBindStatementInt(stmt, 5, rating.GetTypeInt());
            TryBindStatementInt64(stmt, 6, rating.GetId());
            TryBindStatementInt64(stmt, 7, rating.GetValue());

            TryStepStatement(stmt);
        });
    }
}
