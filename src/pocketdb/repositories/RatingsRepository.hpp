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

        // TODO (brangr): move to ConsensusRepository
        int GetUserLikersCount(int addressId)
        {
            int result = 0;

            auto stmt = SetupSqlStatement(R"sql(
                select count(1)
                from Ratings r
                where   r.Type = ?
                    and r.Id = ?
            )sql");
            TryBindStatementInt(stmt, 1, (int) RatingType::RATING_ACCOUNT_LIKERS);
            TryBindStatementInt(stmt, 2, addressId);

            TryTransactionStep([&]()
            {
                if (sqlite3_step(*stmt) == SQLITE_ROW)
                    result = GetColumnInt(*stmt, 0);

                FinalizeSqlStatement(*stmt);
            });

            return result;
        }

        // TODO (brangr): move to ConsensusRepository
        int GetScoreContentCount(PocketTxType scoreType, PocketTxType contentType,
            const string& scoreAddress, const string& contentAddress,
            int height, const CTransactionRef& tx,
            const std::vector<int>& values,
            int64_t scoresOneToOneDepth)
        {
            string sql = R"sql(
                select count(1)
                from vScores s
                where   s.AddressHash = ?
                    and s.Height <= ?
                    and s.Time < ?
                    and s.Time >= ?
                    and s.Hash != ?
                    and s.Type = ?
                    and exists (
                        select 1
                        from vContents c
                        where   c.AddressHash = ?
                            and c.Type = ?
                            and c.Hash = s.ContentTxHash
                    )
                    and s.Value in
            )sql";
            sql += "(";
            sql += std::to_string(values[0]);
            for (size_t i = 1; i < values.size(); i++)
            {
                sql += ',';
                sql += std::to_string(values[i]);
            }
            sql += ")";

            int result = 0;
            TryTransactionStep([&]()
            {
                auto stmt = SetupSqlStatement(sql);

                TryBindStatementText(stmt, 1, scoreAddress);
                TryBindStatementInt(stmt, 2, height);
                TryBindStatementInt64(stmt, 3, tx->nTime);
                TryBindStatementInt64(stmt, 4, (int64_t) tx->nTime - scoresOneToOneDepth);
                TryBindStatementText(stmt, 5, tx->GetHash().GetHex());
                TryBindStatementInt(stmt, 6, scoreType);
                TryBindStatementText(stmt, 7, contentAddress);
                TryBindStatementInt(stmt, 8, contentType);

                if (sqlite3_step(*stmt) == SQLITE_ROW)
                    result = GetColumnInt(*stmt, 0);

                FinalizeSqlStatement(*stmt);
            });

            return result;
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
