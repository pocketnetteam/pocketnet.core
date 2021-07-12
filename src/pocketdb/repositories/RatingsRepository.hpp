// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 Bitcoin developers
// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef SRC_RATINGSREPOSITORY_HPP
#define SRC_RATINGSREPOSITORY_HPP

#include <util.h>

#include "pocketdb/repositories/BaseRepository.hpp"
#include "pocketdb/models/base/Rating.hpp"
#include "pocketdb/models/base/ReturnDtoModels.hpp"

namespace PocketDb
{
    using std::runtime_error;

    using namespace PocketTx;

    class RatingsRepository : public BaseRepository
    {
    public:
        explicit RatingsRepository(SQLiteDatabase& db) : BaseRepository(db) {}

        void Init() override {}

        void Destroy() override {}

        // Accumulate new rating records
        void InsertRatings(shared_ptr<vector<Rating>> ratings)
        {
            for (const auto& rating : *ratings)
            {
                int64_t nTime1 = GetTimeMicros();

                if (*rating.GetType() == RatingType::RATING_ACCOUNT_LIKERS)
                    InsertLiker(rating);
                else
                    InsertRating(rating);

                int64_t nTime2 = GetTimeMicros();
                LogPrint(BCLog::BENCH, "      - InsertRating (%d): %.2fms\n", *rating.GetTypeInt(),
                    0.001 * (nTime2 - nTime1));
            }
        }

    private:

        void InsertRating(const Rating& rating)
        {
            auto sql = R"sql(
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
            )sql";

            TryTransactionStep([&]()
            {
                auto stmt = SetupSqlStatement(sql);

                TryBindStatementInt(stmt, 1, *rating.GetTypeInt());
                TryBindStatementInt(stmt, 2, *rating.GetHeight());
                TryBindStatementInt64(stmt, 3, *rating.GetId());
                TryBindStatementInt(stmt, 4, *rating.GetTypeInt());
                TryBindStatementInt64(stmt, 5, *rating.GetId());
                TryBindStatementInt(stmt, 6, *rating.GetHeight());
                TryBindStatementInt64(stmt, 7, *rating.GetValue());

                TryStepStatement(stmt);
            });
        }

        void InsertLiker(const Rating& rating)
        {
            auto sql = R"sql(
                INSERT OR FAIL INTO Ratings (
                    Type,
                    Height,
                    Id,
                    Value
                ) SELECT ?,?,?,?
                WHERE NOT EXISTS (select 1 from Ratings r where r.Type=? and r.Id=? and r.Value=?)
            )sql";

            TryTransactionStep([&]()
            {
                auto stmt = SetupSqlStatement(sql);

                TryBindStatementInt(stmt, 1, *rating.GetTypeInt());
                TryBindStatementInt(stmt, 2, *rating.GetHeight());
                TryBindStatementInt64(stmt, 3, *rating.GetId());
                TryBindStatementInt64(stmt, 4, *rating.GetValue());
                TryBindStatementInt(stmt, 5, *rating.GetTypeInt());
                TryBindStatementInt64(stmt, 6, *rating.GetId());
                TryBindStatementInt64(stmt, 7, *rating.GetValue());

                TryStepStatement(stmt);
            });
        }

    }; // namespace PocketDb
}
#endif //SRC_RATINGSREPOSITORY_HPP
