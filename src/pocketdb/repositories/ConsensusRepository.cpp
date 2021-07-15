// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 Bitcoin developers
// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include "pocketdb/repositories/ConsensusRepository.h"

namespace PocketDb
{
    void ConsensusRepository::Init() {}

    void ConsensusRepository::Destroy() {}

    bool ConsensusRepository::ExistsAnotherByName(const string& address, const string& name)
    {
        bool result = false;

        // TODO (brangr): implement sql for first user record - exists
        auto stmt = SetupSqlStatement(R"sql(
            SELECT 1
            FROM vUsersPayload ap
            WHERE ap.Name = ?
            and not exists (
                select 1
                from vAccounts ac
                where ac.Hash = ap.TxHash
                  and ac.AddressHash = ?
            )
        )sql");
        TryBindStatementText(stmt, 1, name);
        TryBindStatementText(stmt, 2, address);

        TryTransactionStep([&]()
        {
            result = sqlite3_step(*stmt) == SQLITE_ROW;
            FinalizeSqlStatement(*stmt);
        });

        return result;
    }

    // Select all user profile edit transaction in chain
    // Transactions.Height is not null
    // TODO (brangr) (v0.21.0): change vUser to vAccounts and pass argument type
    tuple<bool, PTransactionRef> ConsensusRepository::GetLastAccount(const string& address)
    {
        PTransactionRef tx = nullptr;

        auto sql = FullTransactionSql;
        sql += R"sql(
            and t.String1 = ?
            and t.Last = 1
            and t.Height is not null
            and t.Type in (100, 101, 102)
        )sql";

        auto stmt = SetupSqlStatement(sql);
        TryBindStatementText(stmt, 1, address);

        TryTransactionStep([&]()
        {
            if (sqlite3_step(*stmt) == SQLITE_ROW)
                if (auto[ok, transaction] = CreateTransactionFromListRow(stmt, true); ok)
                    tx = transaction;

            FinalizeSqlStatement(*stmt);
        });

        return {tx != nullptr, tx};
    }

    tuple<bool, PTransactionRef> ConsensusRepository::GetLastContent(const string& rootHash)
    {
        PTransactionRef tx = nullptr;

        auto sql = FullTransactionSql;
        sql += R"sql(
            and t.String2 = ?
            and t.Last = 1
            and t.Height is not null
            and t.Type in (200, 201, 202, 203, 204, 205, 206)
        )sql";

        auto stmt = SetupSqlStatement(sql);
        TryBindStatementText(stmt, 1, rootHash);

        TryTransactionStep([&]()
        {
            if (sqlite3_step(*stmt) == SQLITE_ROW)
                if (auto[ok, transaction] = CreateTransactionFromListRow(stmt, true); ok)
                    tx = transaction;

            FinalizeSqlStatement(*stmt);
        });

        return {tx != nullptr, tx};
    }

    // TODO (brangr) (v0.21.0): change for vAccounts and pass type as argument
    bool ConsensusRepository::ExistsUserRegistrations(vector<string>& addresses)
    {
        auto result = false;

        if (addresses.empty())
            return result;

        string sql = R"sql(
            SELECT count(distinct(u.AddressHash))
            FROM vUsers u
            WHERE u.AddressHash IN (
        )sql";

        sql += "'";
        sql += addresses[0];
        sql += "'";
        for (size_t i = 1; i < addresses.size(); i++)
        {
            sql += ",'";
            sql += addresses[i];
            sql += "'";
        }
        sql += ")";

        TryTransactionStep([&]()
        {
            auto stmt = SetupSqlStatement(sql);

            if (sqlite3_step(*stmt) == SQLITE_ROW)
                if (auto[ok, value] = TryGetColumnInt(*stmt, 0); ok)
                    result = (value == (int) addresses.size());

            FinalizeSqlStatement(*stmt);
        });

        return result;
    }

    tuple<bool, PocketTxType> ConsensusRepository::GetLastBlockingType(const string& address, const string& addressTo)
    {
        bool blockingExists = false;
        PocketTxType blockingType = PocketTxType::NOT_SUPPORTED;

        auto stmt = SetupSqlStatement(R"sql(
            SELECT b.Type
            FROM vBlockings b
            WHERE b.AddressHash = ?
                AND b.AddressToHash = ?
                AND b.Last = 1
            LIMIT 1
        )sql");
        TryBindStatementText(stmt, 1, address);
        TryBindStatementText(stmt, 2, addressTo);

        TryTransactionStep([&]()
        {
            if (sqlite3_step(*stmt) == SQLITE_ROW)
            {
                if (auto[ok, value] = TryGetColumnInt(*stmt, 0); ok)
                {
                    blockingExists = true;
                    blockingType = (PocketTxType) value;
                }
            }

            FinalizeSqlStatement(*stmt);
        });

        return {blockingExists, blockingType};
    }

    tuple<bool, PocketTxType> ConsensusRepository::GetLastSubscribeType(const string& address,
                                                                        const string& addressTo)
    {
        bool subscribeExists = false;
        PocketTxType subscribeType = PocketTxType::NOT_SUPPORTED;

        auto stmt = SetupSqlStatement(R"sql(
            SELECT s.Type
            FROM vSubscribes s
            WHERE s.AddressHash = ?
                AND s.AddressToHash = ?
                AND s.Last = 1
            LIMIT 1
        )sql");

        TryBindStatementText(stmt, 1, address);
        TryBindStatementText(stmt, 2, addressTo);

        TryTransactionStep([&]()
        {
            if (sqlite3_step(*stmt) == SQLITE_ROW)
            {
                if (auto[ok, value] = TryGetColumnInt(*stmt, 0); ok)
                {
                    subscribeExists = true;
                    subscribeType = (PocketTxType) value;
                }
            }

            FinalizeSqlStatement(*stmt);
        });

        return {subscribeExists, subscribeType};
    }

    // TODO (brangr) (v0.21.0): change for vContents and pass type as argument
    shared_ptr<string> ConsensusRepository::GetPostAddress(const string& postHash)
    {
        shared_ptr<string> result = nullptr;

        auto stmt = SetupSqlStatement(R"sql(
            SELECT p.AddressHash
            FROM vPosts p
            WHERE p.Hash = ?
            LIMIT 1
        )sql");

        TryBindStatementText(stmt, 1, postHash);

        TryTransactionStep([&]()
        {
            if (sqlite3_step(*stmt) == SQLITE_ROW)
                if (auto[ok, value] = TryGetColumnString(*stmt, 0); ok)
                    result = make_shared<string>(value);

            FinalizeSqlStatement(*stmt);
        });

        return result;
    }

    bool ConsensusRepository::ExistsComplain(const string& txHash, const string& postHash, const string& address)
    {
        bool result = false;

        auto stmt = SetupSqlStatement(R"sql(
            SELECT 1
            FROM vComplains c
            WHERE c.AddressHash = ?
                AND c.PostTxHash = ?
                AND c.Hash != ?
            LIMIT 1
        )sql");

        TryBindStatementText(stmt, 1, address);
        TryBindStatementText(stmt, 2, postHash);
        TryBindStatementText(stmt, 3, txHash);

        TryTransactionStep([&]()
        {
            if (sqlite3_step(*stmt) == SQLITE_ROW)
            {
                if (auto[ok, value] = TryGetColumnInt(*stmt, 0); ok)
                    result = true;
            }

            FinalizeSqlStatement(*stmt);
        });

        return result;
    }


    bool ConsensusRepository::ExistsScore(const string& address, const string& contentHash,
        PocketTxType type, bool mempool)
    {
        bool result = false;

        string sql = R"sql(
            SELECT 1
            FROM vScores s
            WHERE s.AddressHash = ?
                and s.ContentTxHash = ?
                and s.Type = ?
        )sql";

        if (!mempool)
            sql += " and s.Height is not null";

        auto stmt = SetupSqlStatement(sql);
        TryBindStatementText(stmt, 1, address);
        TryBindStatementText(stmt, 2, contentHash);
        TryBindStatementInt(stmt, 3, (int) type);

        TryTransactionStep([&]()
        {
            if (sqlite3_step(*stmt) == SQLITE_ROW)
                if (auto[ok, value] = TryGetColumnInt(*stmt, 0); ok)
                    result = true;

            FinalizeSqlStatement(*stmt);
        });

        return result;
    }

    int64_t ConsensusRepository::GetUserBalance(const string& address)
    {
        int64_t result = 0;

        auto sql = R"sql(
            SELECT SUM(o.Value)
            FROM TxOutputs o
            INNER JOIN Transactions t ON o.TxHash == t.Hash
            WHERE o.SpentHeight is null
                AND o.AddressHash = ?
                AND t.Height < ?
            GROUP BY o.AddressHash
        )sql";

        TryTransactionStep([&]()
        {
            auto stmt = SetupSqlStatement(sql);

            TryBindStatementText(stmt, 1, address);

            if (sqlite3_step(*stmt) == SQLITE_ROW)
            {
                if (auto[ok, value] = TryGetColumnInt64(*stmt, 0); ok)
                {
                    result = value;
                }
            }

            FinalizeSqlStatement(*stmt);
        });

        return result;
    }

    int ConsensusRepository::GetUserReputation(const string& address)
    {
        int result = 0;

        auto sql = R"sql(
                select r.Value
                from Ratings r
                where r.Type = ?
                    and r.Id = (SELECT u.Id FROM vUsers u WHERE u.Last = 1 AND u.AddressHash = ? LIMIT 1)
                order by r.Height desc
                limit 1
            )sql";

        TryTransactionStep([&]()
        {
            auto stmt = SetupSqlStatement(sql);

            TryBindStatementInt(stmt, 1, (int) RatingType::RATING_ACCOUNT);
            TryBindStatementText(stmt, 2, address);

            if (sqlite3_step(*stmt) == SQLITE_ROW)
                if (auto[ok, value] = TryGetColumnInt(*stmt, 0); ok)
                    result = value;

            FinalizeSqlStatement(*stmt);
        });

        return result;
    }

    int ConsensusRepository::GetUserReputation(int addressId)
    {
        int result = 0;

        auto stmt = SetupSqlStatement(R"sql(
            select r.Value
            from Ratings r
            where r.Type = ?
                and r.Id = ?
            order by r.Height desc
            limit 1
        )sql");
        TryBindStatementInt(stmt, 1, (int) RatingType::RATING_ACCOUNT);
        TryBindStatementInt(stmt, 2, addressId);

        TryTransactionStep([&]()
        {
            if (sqlite3_step(*stmt) == SQLITE_ROW)
                if (auto[ok, value] = TryGetColumnInt(*stmt, 0); ok)
                    result = value;

            FinalizeSqlStatement(*stmt);
        });

        return result;
    }

    // Selects for get models data
    shared_ptr<ScoreDataDto> ConsensusRepository::GetScoreData(const string& txHash)
    {
        shared_ptr<ScoreDataDto> result = nullptr;

        auto sql = R"sql(
                select
                    s.Hash sTxHash,
                    s.Type sType,
                    s.Time sTime,
                    s.Value sValue,
                    sa.Id saId,
                    sa.AddressHash saHash,
                    c.Hash cTxHash,
                    c.Type cType,
                    c.Time cTime,
                    c.Id cId,
                    ca.Id caId,
                    ca.AddressHash caHash
                from
                    vScores s
                    join vAccounts sa on sa.AddressHash=s.AddressHash
                    join vContents c on c.Hash=s.ContentTxHash
                    join vAccounts ca on ca.AddressHash=c.AddressHash
                where s.Hash = ?
                limit 1
            )sql";

        // ---------------------------------------

        TryTransactionStep([&]()
        {
            auto stmt = SetupSqlStatement(sql);
            TryBindStatementText(stmt, 1, txHash);

            if (sqlite3_step(*stmt) == SQLITE_ROW)
            {
                ScoreDataDto data;

                if (auto[ok, value] = TryGetColumnString(*stmt, 0); ok) data.ScoreTxHash = value;
                if (auto[ok, value] = TryGetColumnInt(*stmt, 1); ok) data.ScoreType = (PocketTxType) value;
                if (auto[ok, value] = TryGetColumnInt64(*stmt, 2); ok) data.ScoreTime = value;
                if (auto[ok, value] = TryGetColumnInt(*stmt, 3); ok) data.ScoreValue = value;
                if (auto[ok, value] = TryGetColumnInt(*stmt, 4); ok) data.ScoreAddressId = value;
                if (auto[ok, value] = TryGetColumnString(*stmt, 5); ok) data.ScoreAddressHash = value;

                if (auto[ok, value] = TryGetColumnString(*stmt, 6); ok) data.ContentTxHash = value;
                if (auto[ok, value] = TryGetColumnInt(*stmt, 7); ok) data.ContentType = (PocketTxType) value;
                if (auto[ok, value] = TryGetColumnInt64(*stmt, 8); ok) data.ContentTime = value;
                if (auto[ok, value] = TryGetColumnInt(*stmt, 9); ok) data.ContentId = value;
                if (auto[ok, value] = TryGetColumnInt(*stmt, 10); ok) data.ContentAddressId = value;
                if (auto[ok, value] = TryGetColumnString(*stmt, 11); ok) data.ContentAddressHash = value;

                result = make_shared<ScoreDataDto>(data);
            }

            FinalizeSqlStatement(*stmt);
        });

        return result;
    }

    // Select many referrers
    shared_ptr<map<string, string>> ConsensusRepository::GetReferrers(const vector<string>& addresses, int minHeight)
    {
        shared_ptr<map<string, string>> result = make_shared<map<string, string>>();

        if (addresses.empty())
            return result;

        string sql = R"sql(
                select a.AddressHash, ifnull(a.ReferrerAddressHash,'')
                from vAccounts a
                where a.Height >= ?
                    and a.Height = (select min(a1.Height) from vAccounts a1 where a1.AddressHash=a.AddressHash)
                    and a.ReferrerAddressHash is not null
                    and a.AddressHash in (
            )sql";

        sql += addresses[0];
        for (size_t i = 1; i < addresses.size(); i++)
        {
            sql += ',';
            sql += addresses[i];
        }
        sql += ")";

        // --------------------------------------------

        TryTransactionStep([&]()
        {
            auto stmt = SetupSqlStatement(sql);
            TryBindStatementInt(stmt, 0, minHeight);

            while (sqlite3_step(*stmt) == SQLITE_ROW)
            {
                if (auto[ok1, value1] = TryGetColumnString(*stmt, 1); ok1 && !value1.empty())
                    if (auto[ok2, value2] = TryGetColumnString(*stmt, 2); ok2 && !value2.empty())
                        result->emplace(value1, value2);
            }

            FinalizeSqlStatement(*stmt);
        });

        return result;
    }

    // Select referrer for one account
    shared_ptr<string> ConsensusRepository::GetReferrer(const string& address, int minTime)
    {
        shared_ptr<string> result;

        auto sql = R"sql(
                select a.ReferrerAddressHash
                from vAccounts a
                where a.Time >= ?
                    and a.AddressHash = ?
                order by a.Height asc
                limit 1
            )sql";

        TryTransactionStep([&]()
        {
            auto stmt = SetupSqlStatement(sql);

            TryBindStatementInt(stmt, 1, minTime);
            TryBindStatementText(stmt, 2, address);

            if (sqlite3_step(*stmt) == SQLITE_ROW)
            {
                if (auto[ok, value] = TryGetColumnString(*stmt, 0); ok && !value.empty())
                    result = make_shared<string>(value);
            }

            FinalizeSqlStatement(*stmt);
        });

        return result;
    }

    int ConsensusRepository::GetUserLikersCount(int addressId)
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
                if (auto[ok, value] = TryGetColumnInt(*stmt, 0); ok)
                    result = value;

            FinalizeSqlStatement(*stmt);
        });

        return result;
    }

    int ConsensusRepository::GetScoreContentCount(PocketTxType scoreType, PocketTxType contentType,
                             const string& scoreAddress, const string& contentAddress,
                             int height, const CTransactionRef& tx,
                             const std::vector<int>& values,
                             int64_t scoresOneToOneDepth)
    {
        string sql = R"sql(
                select count(1)
                from vScores s -- indexed by Transactions_GetScoreContentCount
                join vContents c -- indexed by Transactions_GetScoreContentCount_2
                    on c.Type = ? and c.Hash = s.ContentTxHash and c.AddressHash = ? and c.Height <= ?
                where   s.AddressHash = ?
                    and s.Height <= ?
                    and s.Time < ?
                    and s.Time >= ?
                    and s.Hash != ?
                    and s.Type = ?
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

        auto stmt = SetupSqlStatement(sql);
        TryBindStatementInt(stmt, 1, contentType);
        TryBindStatementText(stmt, 2, contentAddress);
        TryBindStatementInt(stmt, 3, height);
        TryBindStatementText(stmt, 4, scoreAddress);
        TryBindStatementInt(stmt, 5, height);
        TryBindStatementInt64(stmt, 6, tx->nTime);
        TryBindStatementInt64(stmt, 7, (int64_t) tx->nTime - scoresOneToOneDepth);
        TryBindStatementText(stmt, 8, tx->GetHash().GetHex());
        TryBindStatementInt(stmt, 9, scoreType);

        int result = 0;
        TryTransactionStep([&]()
        {
            if (sqlite3_step(*stmt) == SQLITE_ROW)
                if (auto[ok, value] = TryGetColumnInt(*stmt, 0); ok)
                    result = value;

            FinalizeSqlStatement(*stmt);
        });

        return result;
    }
}