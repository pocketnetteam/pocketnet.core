// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include "pocketdb/repositories/RatingsRepository.h"

namespace PocketDb
{
    void RatingsRepository::InsertRatings(shared_ptr<vector<Rating>> ratings)
    {
        SqlTransaction(__func__, [&]()
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
        });
    }

    bool RatingsRepository::ExistsLiker(int addressId, int likerId, const vector<RatingType>& types)
    {
        bool result = false;
        
        SqlTransaction(__func__, [&]()
        {
            result = Sql(R"sql(
                select 1
                from Ratings r indexed by Ratings_Type_Uid_Value
                where 
                    Type in ( )sql" + join(vector<string>(types.size(), "?"), ",") + R"sql( ) and
                    r.Uid = ? and
                    r.Value = ?
            )sql")
            .Bind(types, addressId, likerId)
            .Run() == SQLITE_ROW;
        });

        return result;
    }

    void RatingsRepository::InsertRating(const Rating& rating)
    {
        // Insert new Last record
        Sql(R"sql(
            insert or fail into Ratings (
                Type,
                Last,
                Height,
                Uid,
                Value
            ) select ?,1,?,?,
                ifnull((
                    select r.Value
                    from Ratings r indexed by Ratings_Type_Uid_Last_Height
                    where
                        r.Type = ? and
                        r.Last = 1 and
                        r.Uid = ? and
                        r.Height < ?
                    limit 1
                ), 0) + ?
        )sql")
        .Bind(
            *rating.GetType(),
            rating.GetHeight(),
            rating.GetId(),
            *rating.GetType(),
            rating.GetId(),
            rating.GetHeight(),
            rating.GetValue())
        .Run();

        // Clear old Last record
        Sql(R"sql(
            update Ratings indexed by Ratings_Type_Uid_Last_Height
                set Last = 0
            where Type = ?
                and Last = 1
                and Uid = ?
                and Height < ?
        )sql")
        .Bind(*rating.GetType(), rating.GetId(), rating.GetHeight())
        .Run();
    }

    void RatingsRepository::InsertLiker(const Rating& rating)
    {
        Sql(R"sql(
            insert or fail into Ratings (
                Type,
                Last,
                Height,
                Uid,
                Value
            ) values ( ?,1,?,?,? )
        )sql")
        .Bind(*rating.GetType(), rating.GetHeight(), rating.GetId(), rating.GetValue())
        .Run();
    }
}
