// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 Bitcoin developers
// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef SRC_RATINGSREPOSITORY_HPP
#define SRC_RATINGSREPOSITORY_HPP

#include "pocketdb/pocketnet.h"
#include "pocketdb/repositories/BaseRepository.hpp"
#include "pocketdb/models/base/Rating.hpp"
#include "pocketdb/models/dto/ReturnDtoModels.hpp"

namespace PocketDb {
    using std::runtime_error;

    using namespace PocketTx;

    class RatingsRepository : public BaseRepository
    {
    public:
        explicit RatingsRepository(SQLiteDatabase& db) : BaseRepository(db) {}

        void Init() override {}
        void Destroy() override {}

        tuple<bool, int> GetUserReputation(string address, int height)
        {
            int result = 0;
            auto func = __func__;

            bool tryResult = TryTransactionStep([&]() {
                auto stmt = SetupSqlStatement(R"sql(
                    select r.Value
                    from Ratings r
                    where r.Type = ? and r.Height <= ? and r.Id = (select t.Id from vUsers t where t.Hash = ? limit 1)
                    order by r.Height desc
                    limit 1
                )sql");

                //TODO (joni): check
                auto typePtr = make_shared<int>(RatingType::RATING_ACCOUNT);
                auto heightPtr = make_shared<int>(height);
                auto idPtr = make_shared<string>(address);

                auto bindResult = TryBindStatementInt(stmt, 1, typePtr);
                bindResult &= TryBindStatementInt(stmt, 2, heightPtr);
                bindResult &= TryBindStatementText(stmt, 3, idPtr);

                if (!bindResult)
                {
                    FinalizeSqlStatement(*stmt);
                    throw runtime_error(strprintf("%s: can't get user reputation (bind)\n", func));
                }

                if (sqlite3_step(*stmt) == SQLITE_ROW) {
                    result = GetColumnInt(*stmt, 1);
                }

                FinalizeSqlStatement(*stmt);
                return true;
            });

            return make_tuple(true, result);
        }

        tuple<bool, int> GetUserLikersCount(string address, int height)
        {
            int result = 0;
            auto func = __func__;

            bool tryResult = TryTransactionStep([&]() {
                auto stmt = SetupSqlStatement(R"sql(
                    select count(1)
                    from Ratings r
                    where r.Type = ? and r.Height <= ? and r.Id = (select t.Id from vUsers t where t.Hash = ? limit 1)
                )sql");

                //TODO (joni): check
                auto typePtr = make_shared<int>(RatingType::RATING_ACCOUNT_LIKERS);
                auto heightPtr = make_shared<int>(height);
                auto idPtr = make_shared<string>(address);

                auto bindResult = TryBindStatementInt(stmt, 1, typePtr);
                bindResult &= TryBindStatementInt(stmt, 2, heightPtr);
                bindResult &= TryBindStatementText(stmt, 3, idPtr);
                if (!bindResult) {
                    FinalizeSqlStatement(*stmt);
                    throw runtime_error(strprintf("%s: can't select user likers count (bind)\n", func));
                }

                if (sqlite3_step(*stmt) == SQLITE_ROW) {
                    result = GetColumnInt(*stmt, 1);
                }

                FinalizeSqlStatement(*stmt);
                return true;
            });

            return make_tuple(true, result);
        }

        int GetScorePostCount(string addressHash, string postHash, int height, std::vector<int> values, int64_t txTime, int64_t ScoresOneToOneDepth, string txHash)
        {
//            size_t scores_one_to_one_count = g_pocketdb->SelectCount(
//                reindexer::Query("Scores")
//                    .Where("address", CondEq, scoreAddress) //+
//                    .Where("time", CondGe, (int64_t)tx->nTime - _scores_one_to_one_depth) //+
//                    .Where("time", CondLt, (int64_t)tx->nTime) //+
//                    .Where("block", CondLe, blockHeight) //+
//                    .Where("value", CondSet, values) //+
//                    .Not()
//                    .Where("txid", CondEq, tx->GetHash().GetHex())
//                    .InnerJoin("posttxid", "txid", CondEq, reindexer::Query("Posts").Where("address", CondEq, postAddress))); //+

            int result = 0;
            auto func = __func__;

            bool tryResult = TryTransactionStep([&]() {
                auto stmt = SetupSqlStatement(R"sql(
                    select count(1)
                    from vScorePosts s
                    where s.AddressHash = ?
                        and s.PostTxHash = ?
                        and s.Height <= ?
                        and s.Value in ?
                        and s.Time < ?
                        and s.Time >= ?
                        and s.Hash != ?
                )sql");

                //TODO (joni): check
                auto typePtr = make_shared<int>(RatingType::RATING_ACCOUNT_LIKERS);
                auto heightPtr = make_shared<int>(height);
                auto idPtr = make_shared<string>(addressHash);

                auto bindResult = TryBindStatementInt(stmt, 1, typePtr);
                bindResult &= TryBindStatementInt(stmt, 2, heightPtr);
                bindResult &= TryBindStatementText(stmt, 3, idPtr);
                if (!bindResult) {
                    FinalizeSqlStatement(*stmt);
                    throw runtime_error(strprintf("%s: can't select user likers count (bind)\n", func));
                }

                if (sqlite3_step(*stmt) == SQLITE_ROW) {
                    result = GetColumnInt(*stmt, 1);
                }

                FinalizeSqlStatement(*stmt);
                return true;
            });

            return result;
        }
    }; // namespace PocketDb
}
#endif //SRC_RATINGSREPOSITORY_HPP
