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
#include "pocketdb/models/base/SelectModels.hpp"

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

            bool tryResult = TryTransactionStep([&]() {
                auto stmt = SetupSqlStatement(R"sql(
                    select r.Value
                    from Ratings r
                    where r.Type = ? and r.Height <= ? and r.Id = (select t.Id from vUsers t where t.Hash = ?)
                    order by r.Height desc
                    limit 1
                )sql");

                //TODO (joni): check
                auto typePtr = make_shared<int>(RatingType::RATING_ACCOUNT);
                if (!TryBindStatementInt(stmt, 1, typePtr))
                    return false;

                auto heightPtr = make_shared<int>(height);
                if (!TryBindStatementInt(stmt, 2, heightPtr))
                    return false;

                auto idPtr = make_shared<string>(address);
                if (!TryBindStatementText(stmt, 3, idPtr))
                    return false;

                if (sqlite3_step(*stmt) == SQLITE_ROW) {
                    result = GetColumnInt(*stmt, 1);
                }

                return true;
            });

            return make_tuple(tryResult, result);
        }

        tuple<bool, int> GetUserLikersCount(string address, int height)
        {
            int result = 0;

            bool tryResult = TryTransactionStep([&]() {
              auto stmt = SetupSqlStatement(R"sql(
                    select count(1)
                    from Ratings r
                    where r.Type = ? and r.Height <= ? and r.Id = (select t.Id from vUsers t where t.Hash = ?)
                )sql");

              //TODO (joni): check
              auto typePtr = make_shared<int>(RatingType::RATING_ACCOUNT_LIKERS);
              if (!TryBindStatementInt(stmt, 1, typePtr))
                  return false;

              auto heightPtr = make_shared<int>(height);
              if (!TryBindStatementInt(stmt, 2, heightPtr))
                  return false;

              auto idPtr = make_shared<string>(address);
              if (!TryBindStatementText(stmt, 3, idPtr))
                  return false;

              if (sqlite3_step(*stmt) == SQLITE_ROW) {
                  result = GetColumnInt(*stmt, 1);
              }

              return true;
            });

            return make_tuple(tryResult, result);
        }
    }; // namespace PocketDb
}
#endif //SRC_RATINGSREPOSITORY_HPP
