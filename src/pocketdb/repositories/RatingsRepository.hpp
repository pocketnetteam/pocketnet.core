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

        tuple<bool, int> GetUserReputation(string address, int height)
        {
            int result = 0;
            auto func = __func__;

            bool tryResult = TryTransactionStep([&]()
            {
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

                if (sqlite3_step(*stmt) == SQLITE_ROW)
                {
                    result = GetColumnInt(*stmt, 0);
                }

                FinalizeSqlStatement(*stmt);
                return true;
            });

            return make_tuple(tryResult, result);
        }

        tuple<bool, int> GetUserLikersCount(string address, int height)
        {
            int result = 0;
            auto func = __func__;

            bool tryResult = TryTransactionStep([&]()
            {
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
                if (!bindResult)
                {
                    FinalizeSqlStatement(*stmt);
                    throw runtime_error(strprintf("%s: can't select user likers count (bind)\n", func));
                }

                if (sqlite3_step(*stmt) == SQLITE_ROW)
                {
                    result = GetColumnInt(*stmt, 0);
                }

                FinalizeSqlStatement(*stmt);
                return true;
            });

            return make_tuple(tryResult, result);
        }

        tuple<bool, int> GetScoreContentCount(PocketTxType txType, const string scoreAddress,
            const string contentAddress, int height, const CTransactionRef& tx, const std::vector<int> values,
            int64_t scoresOneToOneDepth)
        {
            int result = 0;
            auto func = __func__;

            bool tryResult = TryTransactionStep([&]()
            {
                string sql = R"sql(
                    select count(1)
                    from vScores s
                    inner join vContents p on p.Hash = s.ContentTxHash
                    where s.AddressHash = ?
                        and p.AddressHash = ?
                        and s.Height is not null
                        and s.Height <= ?
                        and s.Time < ?
                        and s.Time >= ?
                        and s.Hash != ?
                        and s.Type = ?
                        and p.Type = ?
                        and s.Value in )sql";

                sql += "(";
                sql += std::to_string(values[0]);
                for (auto value : values)
                {
                    sql += ',';
                    sql += std::to_string(value);
                }
                sql += ")";

                auto stmt = SetupSqlStatement(sql);

                auto scoreAddressPtr = make_shared<string>(scoreAddress);
                auto postAddressPtr = make_shared<string>(contentAddress);
                auto heightPtr = make_shared<int>(height);

                // TODO: нужна будет перегрузка для изменения отбора - время заменить на блоки
                auto maxTimePtr = make_shared<int64_t>(tx->nTime);
                auto minTimePtr = make_shared<int64_t>((int64_t) tx->nTime - scoresOneToOneDepth);

                //TODO (joni): check - что смущает? Вроде правильно
                auto scoreTxHashPtr = make_shared<string>(tx->GetHash().GetHex());
                auto scoreTypePtr = make_shared<int>(txType);;

                // TODO (joni): проверь пжл наверху - там должен быть тип контента в scoreData или подобном
                shared_ptr<int> contentTypePtr;
                switch (txType)
                {
                    case PocketTx::ACTION_SCORE_POST:
                        contentTypePtr = make_shared<int>(PocketTx::CONTENT_POST);
                        break;
                    case PocketTx::ACTION_SCORE_COMMENT:
                        contentTypePtr = make_shared<int>(PocketTx::CONTENT_COMMENT);
                        break;
                    default:
                        throw runtime_error(strprintf("%s: can't get score count (unsupported txType)\n", func));
                }

                auto bindResult = TryBindStatementText(stmt, 1, scoreAddressPtr);
                bindResult &= TryBindStatementText(stmt, 2, postAddressPtr);
                bindResult &= TryBindStatementInt(stmt, 3, heightPtr);
                bindResult &= TryBindStatementInt64(stmt, 4, maxTimePtr);
                bindResult &= TryBindStatementInt64(stmt, 5, minTimePtr);
                bindResult &= TryBindStatementText(stmt, 6, scoreTxHashPtr);
                bindResult &= TryBindStatementInt(stmt, 7, scoreTypePtr);
                bindResult &= TryBindStatementInt(stmt, 8, contentTypePtr);
                for (auto i = 0; i < values.size(); i++)
                {
                    auto valuePtr = make_shared<int>(values[i]);
                    bindResult &= TryBindStatementInt(stmt, 9 + i, valuePtr);
                }

                if (!bindResult)
                {
                    FinalizeSqlStatement(*stmt);
                    throw runtime_error(strprintf("%s: can't get score count (bind)\n", func));
                }

                if (sqlite3_step(*stmt) == SQLITE_ROW)
                {
                    result = GetColumnInt(*stmt, 0);
                }

                FinalizeSqlStatement(*stmt);
                return true;
            });

            return make_tuple(tryResult, result);
        }
    }; // namespace PocketDb
}
#endif //SRC_RATINGSREPOSITORY_HPP
