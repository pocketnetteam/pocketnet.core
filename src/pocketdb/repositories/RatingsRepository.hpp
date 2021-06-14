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
        bool InsertRatings(shared_ptr<vector<Rating>> ratings)
        {
            for (const auto& rating : *ratings)
            {
                int64_t nTime1 = GetTimeMicros();

                bool result = true;
                if (*rating.GetType() == RatingType::RATING_ACCOUNT_LIKERS)
                    result = InsertLiker(rating);
                else
                    result = InsertRating(rating);

                int64_t nTime2 = GetTimeMicros();
                LogPrint(BCLog::BENCH, "      - InsertRating (%d): %.2fms\n", *rating.GetTypeInt(), 0.001 * (nTime2 - nTime1));

                if (!result)
                {
                    LogPrintf("InsertRatings (type: %d) failed: id:%d height:%d\n", *rating.GetTypeInt(), *rating.GetId(), *rating.GetHeight());
                        return false;
                }
            }

            return true;
        }

        // TODO (brangr): move to ConsensusRepository
        tuple<bool, int> GetUserReputation(int addressId, int height)
        {
            int result = 0;

            bool tryResult = TryTransactionStep([&]()
            {
                auto stmt = SetupSqlStatement(R"sql(
                    select r.Value
                    from Ratings r
                    where r.Type = ?
                        and r.Id = ?
                        and r.Height <= ?
                    order by r.Height desc
                    limit 1
                )sql");

                auto bindResult = TryBindStatementInt(stmt, 1, (int)RatingType::RATING_ACCOUNT);
                bindResult &= TryBindStatementInt(stmt, 2, addressId);
                bindResult &= TryBindStatementInt(stmt, 3, height);

                if (bindResult && sqlite3_step(*stmt) == SQLITE_ROW)
                {
                    result = GetColumnInt(*stmt, 0);
                }

                FinalizeSqlStatement(*stmt);
                return bindResult;
            });

            return make_tuple(tryResult, result);
        }

        // TODO (brangr): move to ConsensusRepository
        tuple<bool, int> GetUserLikersCount(int addressId, int height)
        {
            int result = 0;

            bool tryResult = TryTransactionStep([&]()
            {
                auto stmt = SetupSqlStatement(R"sql(
                    select count(1)
                    from Ratings r
                    where   r.Type = ?
                        and r.Id = ?
                        and r.Height <= ?
                )sql");

                auto typePtr = make_shared<int>(RatingType::RATING_ACCOUNT_LIKERS);
                auto heightPtr = make_shared<int>(height);
                auto addressIdPtr = make_shared<int>(addressId);

                auto bindResult = TryBindStatementInt(stmt, 1, typePtr);
                bindResult &= TryBindStatementInt(stmt, 2, heightPtr);
                bindResult &= TryBindStatementInt(stmt, 3, addressIdPtr);

                // not exists - not error
                if (bindResult && sqlite3_step(*stmt) == SQLITE_ROW)
                    result = GetColumnInt(*stmt, 0);

                FinalizeSqlStatement(*stmt);
                return true;
            });

            return make_tuple(tryResult, result);
        }

        // TODO (brangr): move to ConsensusRepository
        tuple<bool, int> GetScoreContentCount(PocketTxType scoreType, PocketTxType contentType,
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
            bool tryResult = TryTransactionStep([&]()
            {
                auto stmt = SetupSqlStatement(sql);

                auto bindResult = TryBindStatementText(stmt, 1, scoreAddress);
                bindResult &= TryBindStatementInt(stmt, 2, height);
                bindResult &= TryBindStatementInt64(stmt, 3, tx->nTime);
                bindResult &= TryBindStatementInt64(stmt, 4, (int64_t) tx->nTime - scoresOneToOneDepth);
                bindResult &= TryBindStatementText(stmt, 5, tx->GetHash().GetHex());
                bindResult &= TryBindStatementInt(stmt, 6, scoreType);
                bindResult &= TryBindStatementText(stmt, 7, contentAddress);
                bindResult &= TryBindStatementInt(stmt, 8, contentType);

                // not exists - not error
                if (bindResult && sqlite3_step(*stmt) == SQLITE_ROW)
                    result = GetColumnInt(*stmt, 0);

                FinalizeSqlStatement(*stmt);
                return bindResult;
            });

            return make_tuple(tryResult, result);
        }

    private:

        bool InsertRating(const Rating& rating)
        {
            return TryTransactionStep([&]()
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

                auto result = TryBindStatementInt(stmt, 1, *rating.GetTypeInt());
                result &= TryBindStatementInt(stmt, 2, *rating.GetHeight());
                result &= TryBindStatementInt64(stmt, 3, *rating.GetId());
                result &= TryBindStatementInt(stmt, 4, *rating.GetTypeInt());
                result &= TryBindStatementInt64(stmt, 5, *rating.GetId());
                result &= TryBindStatementInt(stmt, 6, *rating.GetHeight());
                result &= TryBindStatementInt64(stmt, 7, *rating.GetValue());
                
                return result && TryStepStatement(stmt);
            });
        }

        bool InsertLiker(const Rating& rating)
        {
            return TryTransactionStep([&]()
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

                auto result = TryBindStatementInt(stmt, 1, *rating.GetTypeInt());
                result &= TryBindStatementInt(stmt, 2, *rating.GetHeight());
                result &= TryBindStatementInt64(stmt, 3, *rating.GetId());
                result &= TryBindStatementInt64(stmt, 4, *rating.GetValue());
                result &= TryBindStatementInt(stmt, 5, *rating.GetTypeInt());
                result &= TryBindStatementInt64(stmt, 6, *rating.GetId());
                result &= TryBindStatementInt64(stmt, 7, *rating.GetValue());

                return result && TryStepStatement(stmt);
            });
        }

    }; // namespace PocketDb
}
#endif //SRC_RATINGSREPOSITORY_HPP
