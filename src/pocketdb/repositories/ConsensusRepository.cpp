// Copyright (c) 2018-2022 The Pocketnet developers
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

        auto _name = EscapeValue(name);

        TryTransactionStep(__func__, [&]()
        {
            auto stmt = SetupSqlStatement(R"sql(
                select count(*)
                from Payload ap indexed by Payload_String2_nocase_TxHash
                cross join Transactions t indexed by Transactions_Hash_Height
                  on t.Type in (100, 101, 102) and t.Hash = ap.TxHash and t.Height is not null and t.Last = 1
                where ap.String2 like ? escape '\'
                  and t.String1 != ?
            )sql");

            TryBindStatementText(stmt, 1, _name);
            TryBindStatementText(stmt, 2, address);

            if (sqlite3_step(*stmt) == SQLITE_ROW)
                if (auto[ok, value] = TryGetColumnInt(*stmt, 0); ok)
                    result = (value > 0);

            FinalizeSqlStatement(*stmt);
        });

        return result;
    }

    tuple<bool, PTransactionRef> ConsensusRepository::GetFirstContent(const string& rootHash)
    {
        PTransactionRef tx = nullptr;

        TryTransactionStep(__func__, [&]()
        {
            string sql = R"sql(
                select
                    t.Type,
                    t.Hash,
                    t.Time,
                    t.Last,
                    t.Id,
                    t.String1,
                    t.String2,
                    t.String3,
                    t.String4,
                    t.String5,
                    t.Int1,
                    p.TxHash pHash,
                    p.String1 pString1,
                    p.String2 pString2,
                    p.String3 pString3,
                    p.String4 pString4,
                    p.String5 pString5,
                    p.String6 pString6,
                    p.String7 pString7
                from Transactions t indexed by Transactions_Hash_Height
                left join Payload p on t.Hash = p.TxHash
                where t.Type in (200,201,202,203,204)
                  and t.Hash = ?
                  and t.String2 = ?
                  and t.Height is not null
            )sql";

            auto stmt = SetupSqlStatement(sql);
            TryBindStatementText(stmt, 1, rootHash);
            TryBindStatementText(stmt, 2, rootHash);

            if (sqlite3_step(*stmt) == SQLITE_ROW)
                if (auto[ok, transaction] = CreateTransactionFromListRow(stmt, true); ok)
                    tx = transaction;

            FinalizeSqlStatement(*stmt);
        });

        return {tx != nullptr, tx};
    }

    tuple<bool, PTransactionRef> ConsensusRepository::GetLastContent(const string& rootHash, const vector<TxType>& types)
    {
        PTransactionRef tx = nullptr;

        TryTransactionStep(__func__, [&]()
        {
            string sql = R"sql(
                SELECT
                    t.Type,
                    t.Hash,
                    t.Time,
                    t.Last,
                    t.Id,
                    t.String1,
                    t.String2,
                    t.String3,
                    t.String4,
                    t.String5,
                    t.Int1,
                    p.TxHash pHash,
                    p.String1 pString1,
                    p.String2 pString2,
                    p.String3 pString3,
                    p.String4 pString4,
                    p.String5 pString5,
                    p.String6 pString6,
                    p.String7 pString7
                FROM Transactions t indexed by Transactions_Type_Last_String2_Height
                LEFT JOIN Payload p on t.Hash = p.TxHash
                WHERE t.Type in ( )sql" + join(vector<string>(types.size(), "?"), ",") + R"sql( )
                    and t.String2 = ?
                    and t.Last = 1
                    and t.Height is not null
            )sql";

            auto stmt = SetupSqlStatement(sql);

            int i = 1;
            for (const auto& type: types)
                TryBindStatementInt(stmt, i++, type);
            TryBindStatementText(stmt, i++, rootHash);

            if (sqlite3_step(*stmt) == SQLITE_ROW)
                if (auto[ok, transaction] = CreateTransactionFromListRow(stmt, true); ok)
                    tx = transaction;

            FinalizeSqlStatement(*stmt);
        });

        return {tx != nullptr, tx};
    }

    bool ConsensusRepository::ExistsUserRegistrations(vector<string>& addresses, bool mempool)
    {
        auto result = false;

        if (addresses.empty())
            return result;

        // Build sql string
        string sql = R"sql(
            select count(distinct(String1))
            from Transactions
            where Type in (100, 101, 102)
              and String1 in ( )sql" + join(vector<string>(addresses.size(), "?"), ",") + R"sql( )
              )sql" + (mempool ? "" : " and Height is not null ") + R"sql(
        )sql";

        // Execute
        TryTransactionStep(__func__, [&]()
        {
            auto stmt = SetupSqlStatement(sql);

            for (size_t i = 0; i < addresses.size(); i++)
                TryBindStatementText(stmt, (int)i + 1, addresses[i]);

            if (sqlite3_step(*stmt) == SQLITE_ROW)
                if (auto[ok, value] = TryGetColumnInt(*stmt, 0); ok)
                    result = (value == (int) addresses.size());

            FinalizeSqlStatement(*stmt);
        });

        return result;
    }

    tuple<bool, TxType> ConsensusRepository::GetLastBlockingType(const string& address, const string& addressTo)
    {
        bool blockingExists = false;
        TxType blockingType = TxType::TX_EMPTY;

        TryTransactionStep(__func__, [&]()
        {
            auto stmt = SetupSqlStatement(R"sql(
                SELECT Type
                FROM Transactions indexed by Transactions_Type_Last_String1_String2_Height
                WHERE Type in (305, 306)
                    and String1 = ?
                    and String2 = ?
                    and Height is not null
                    and Last = 1
            )sql");

            TryBindStatementText(stmt, 1, address);
            TryBindStatementText(stmt, 2, addressTo);

            if (sqlite3_step(*stmt) == SQLITE_ROW)
            {
                if (auto[ok, value] = TryGetColumnInt(*stmt, 0); ok)
                {
                    blockingExists = true;
                    blockingType = (TxType) value;
                }
            }

            FinalizeSqlStatement(*stmt);
        });

        return {blockingExists, blockingType};
    }

    tuple<bool, TxType> ConsensusRepository::GetLastSubscribeType(const string& address,
        const string& addressTo)
    {
        bool subscribeExists = false;
        TxType subscribeType = TxType::TX_EMPTY;

        TryTransactionStep(__func__, [&]()
        {
            auto stmt = SetupSqlStatement(R"sql(
                SELECT Type
                FROM Transactions indexed by Transactions_Type_Last_String1_String2_Height
                WHERE Type in (302, 303, 304)
                    and String1 = ?
                    and String2 = ?
                    and Height is not null
                    and Last = 1
            )sql");

            TryBindStatementText(stmt, 1, address);
            TryBindStatementText(stmt, 2, addressTo);

            if (sqlite3_step(*stmt) == SQLITE_ROW)
            {
                if (auto[ok, value] = TryGetColumnInt(*stmt, 0); ok)
                {
                    subscribeExists = true;
                    subscribeType = (TxType) value;
                }
            }

            FinalizeSqlStatement(*stmt);
        });

        return {subscribeExists, subscribeType};
    }

    shared_ptr<string> ConsensusRepository::GetContentAddress(const string& postHash)
    {
        shared_ptr<string> result = nullptr;

        TryTransactionStep(__func__, [&]()
        {
            auto stmt = SetupSqlStatement(R"sql(
                SELECT String1
                FROM Transactions
                WHERE Hash = ?
                  and Height is not null
            )sql");

            TryBindStatementText(stmt, 1, postHash);

            if (sqlite3_step(*stmt) == SQLITE_ROW)
                if (auto[ok, value] = TryGetColumnString(*stmt, 0); ok)
                    result = make_shared<string>(value);

            FinalizeSqlStatement(*stmt);
        });

        return result;
    }

    bool ConsensusRepository::ExistsComplain(const string& postHash, const string& address)
    {
        bool result = false;

        TryTransactionStep(__func__, [&]()
        {
            auto stmt = SetupSqlStatement(R"sql(
                SELECT count(*)
                FROM Transactions
                WHERE Type in (307)
                  and String1 = ?
                  and String2 = ?
                  and Height is not null
            )sql");

            TryBindStatementText(stmt, 1, address);
            TryBindStatementText(stmt, 2, postHash);

            if (sqlite3_step(*stmt) == SQLITE_ROW)
                if (auto[ok, value] = TryGetColumnInt(*stmt, 0); ok)
                    result = (value > 0);

            FinalizeSqlStatement(*stmt);
        });

        return result;
    }


    bool ConsensusRepository::ExistsScore(const string& address, const string& contentHash,
        TxType type, bool mempool)
    {
        bool result = false;

        string sql = R"sql(
            select count(*)
            from Transactions
            where String1 = ?
              and String2 = ?
              and Type = ?
        )sql";

        if (!mempool)
            sql += " and Height is not null";

        TryTransactionStep(__func__, [&]()
        {
            auto stmt = SetupSqlStatement(sql);
            TryBindStatementText(stmt, 1, address);
            TryBindStatementText(stmt, 2, contentHash);
            TryBindStatementInt(stmt, 3, (int) type);

            if (sqlite3_step(*stmt) == SQLITE_ROW)
                if (auto[ok, value] = TryGetColumnInt(*stmt, 0); ok)
                    result = (value > 0);

            FinalizeSqlStatement(*stmt);
        });

        return result;
    }

    int64_t ConsensusRepository::GetUserBalance(const string& address)
    {
        int64_t result = 0;

        auto sql = R"sql(
            select Value
            from Balances
            where AddressHash = ?
              and Last = 1
        )sql";

        TryTransactionStep(__func__, [&]()
        {
            auto stmt = SetupSqlStatement(sql);
            TryBindStatementText(stmt, 1, address);

            if (sqlite3_step(*stmt) == SQLITE_ROW)
                if (auto[ok, value] = TryGetColumnInt64(*stmt, 0); ok)
                    result = value;

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
              and r.Id = (SELECT u.Id FROM Transactions u WHERE u.Type in (100, 101, 102) and u.Height is not null and u.Last = 1 and u.String1 = ? LIMIT 1)
              and r.Last = 1
        )sql";

        TryTransactionStep(__func__, [&]()
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

        string sql = R"sql(
            select r.Value
            from Ratings r
            where r.Type = ?
              and r.Id = ?
              and r.Last = 1
        )sql";

        TryTransactionStep(__func__, [&]()
        {
            auto stmt = SetupSqlStatement(sql);
            TryBindStatementInt(stmt, 1, (int) RatingType::RATING_ACCOUNT);
            TryBindStatementInt(stmt, 2, addressId);

            if (sqlite3_step(*stmt) == SQLITE_ROW)
                if (auto[ok, value] = TryGetColumnInt(*stmt, 0); ok)
                    result = value;

            FinalizeSqlStatement(*stmt);
        });

        return result;
    }

    int ConsensusRepository::GetAccountRegistrationHeight(int addressId)
    {
        int result = 0;

        string sql = R"sql(
            select min(Height)
            from Transactions
            where Type in (100, 101, 102)
              and Id = ?
        )sql";

        TryTransactionStep(__func__, [&]()
        {
            auto stmt = SetupSqlStatement(sql);
            TryBindStatementInt(stmt, 1, addressId);

            if (sqlite3_step(*stmt) == SQLITE_ROW)
                if (auto[ok, value] = TryGetColumnInt(*stmt, 0); ok)
                    result = value;

            FinalizeSqlStatement(*stmt);
        });

        return result;
    }

    int64_t ConsensusRepository::GetAccountRegistrationTime(int addressId)
    {
        int64_t result = 0;

        string sql = R"sql(
            select Time
            from Transactions indexed by Transactions_Id
            where Type in (100)
              and Height is not null
              and Id = ?
            order by Height asc
            limit 1
        )sql";

        TryTransactionStep(__func__, [&]()
        {
            auto stmt = SetupSqlStatement(sql);
            TryBindStatementInt(stmt, 1, addressId);

            if (sqlite3_step(*stmt) == SQLITE_ROW)
                if (auto[ok, value] = TryGetColumnInt64(*stmt, 0); ok)
                    result = value;

            FinalizeSqlStatement(*stmt);
        });

        return result;
    }

    // Selects for get models data
    ScoreDataDtoRef ConsensusRepository::GetScoreData(const string& txHash)
    {
        shared_ptr<ScoreDataDto> result = nullptr;

        string sql = R"sql(
            select
            
                s.Hash sTxHash,
                s.Type sType,
                s.Time sTime,
                s.Int1 sValue,
                sa.Id saId,
                sa.String1 saHash,
                c.Hash cTxHash,
                c.Type cType,
                c.Time cTime,
                c.Id cId,
                ca.Id caId,
                ca.String1 caHash

            from Transactions s indexed by Transactions_Hash_Height

            -- Score Address
            join Transactions sa indexed by Transactions_Type_Last_String1_Height_Id
                on sa.Type in (100,101,102) and sa.Height > 0 and sa.String1 = s.String1 and sa.Last = 1

            -- Content
            join Transactions c indexed by Transactions_Hash_Height
                on c.Type in (200,201,202,203,204,205,206,207) and c.Height > 0 and c.Hash = s.String2

            -- Content Address
            join Transactions ca indexed by Transactions_Type_Last_String1_Height_Id
                on ca.Type in (100,101,102) and ca.Height > 0 and ca.String1 = c.String1 and ca.Last = 1

            where s.Hash = ?
        )sql";

        TryTransactionStep(__func__, [&]()
        {
            auto stmt = SetupSqlStatement(sql);
            TryBindStatementText(stmt, 1, txHash);

            if (sqlite3_step(*stmt) == SQLITE_ROW)
            {
                ScoreDataDto data;

                if (auto[ok, value] = TryGetColumnString(*stmt, 0); ok) data.ScoreTxHash = value;
                if (auto[ok, value] = TryGetColumnInt(*stmt, 1); ok) data.ScoreType = (TxType) value;
                if (auto[ok, value] = TryGetColumnInt64(*stmt, 2); ok) data.ScoreTime = value;
                if (auto[ok, value] = TryGetColumnInt(*stmt, 3); ok) data.ScoreValue = value;
                if (auto[ok, value] = TryGetColumnInt(*stmt, 4); ok) data.ScoreAddressId = value;
                if (auto[ok, value] = TryGetColumnString(*stmt, 5); ok) data.ScoreAddressHash = value;

                if (auto[ok, value] = TryGetColumnString(*stmt, 6); ok) data.ContentTxHash = value;
                if (auto[ok, value] = TryGetColumnInt(*stmt, 7); ok) data.ContentType = (TxType) value;
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

        // Build sql string
        string sql = R"sql(
            select u.String1, u.String2
            from Transactions u
            where u.Type in (100, 101, 102)
              and u.Height is not null
              and u.Height >= ?
              and u.Height = (select min(u1.Height) from Transactions u1 where u1.Type in (100, 101, 102) and u1.Height is not null and u1.String1=u.String1)
              and u.String2 is not null
        )sql";

        sql += " and u.AddressHash in ( ";
        sql += join(vector<string>(addresses.size(), "?"), ",");
        sql += " ) ";

        // Execute
        TryTransactionStep(__func__, [&]()
        {
            auto stmt = SetupSqlStatement(sql);

            TryBindStatementInt(stmt, 1, minHeight);
            for (size_t i = 0; i < addresses.size(); i++)
                TryBindStatementText(stmt, (int)i + 2, addresses[i]);

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
    tuple<bool, string> ConsensusRepository::GetReferrer(const string& address)
    {
        bool result = false;
        string referrer;

        string sql = R"sql(
            select String2
            from Transactions indexed by Transactions_Type_String1_Height_Time_Int1
            where Type in (100)
              and Height is not null
              and String1 = ?
            order by Height asc
            limit 1
        )sql";

        TryTransactionStep(__func__, [&]()
        {
            auto stmt = SetupSqlStatement(sql);
            TryBindStatementText(stmt, 1, address);

            if (sqlite3_step(*stmt) == SQLITE_ROW)
            {
                if (auto[ok, value] = TryGetColumnString(*stmt, 0); ok && !value.empty())
                {
                    result = true;
                    referrer = value;
                }
            }

            FinalizeSqlStatement(*stmt);
        });

        return {result, referrer};
    }

    int ConsensusRepository::GetUserLikersCount(int addressId)
    {
        int result = 0;

        string sql = R"sql(
            select count(1)
            from Ratings r
            where r.Type = ?
              and r.Id = ?
        )sql";

        TryTransactionStep(__func__, [&]()
        {
            auto stmt = SetupSqlStatement(sql);
            TryBindStatementInt(stmt, 1, (int) RatingType::RATING_ACCOUNT_LIKERS);
            TryBindStatementInt(stmt, 2, addressId);

            if (sqlite3_step(*stmt) == SQLITE_ROW)
                if (auto[ok, value] = TryGetColumnInt(*stmt, 0); ok)
                    result = value;

            FinalizeSqlStatement(*stmt);
        });

        return result;
    }

    int ConsensusRepository::GetScoreContentCount(
        int height,
        const shared_ptr<ScoreDataDto>& scoreData,
        const std::vector<int>& values,
        int64_t scoresOneToOneDepth)
    {
        int result = 0;

        if (values.empty())
            return result;

        // Build sql string
        string sql = R"sql(
            select count(1)
            from Transactions c indexed by Transactions_Type_Last_String1_String2_Height
            join Transactions s indexed by Transactions_Type_String1_String2_Height
                on  s.String2 = c.String2
                and s.Type in (300)
                and s.Height <= ?
                and s.String1 = ?
                and s.Time < ?
                and s.Time >= ?
                and s.Int1 in ( )sql" + join(values | transformed(static_cast<std::string(*)(int)>(std::to_string)), ",") + R"sql( )
                and s.Hash != ?
            where c.Type in (200,201,202,207)
              and c.String1 = ?
              and c.Height is not null
              and c.Last = 1
        )sql";

        // Execute
        TryTransactionStep(__func__, [&]()
        {
            auto stmt = SetupSqlStatement(sql);

            int i = 1;
            TryBindStatementInt(stmt, i++, height);
            TryBindStatementText(stmt, i++, scoreData->ScoreAddressHash);
            TryBindStatementInt64(stmt, i++, scoreData->ScoreTime);
            TryBindStatementInt64(stmt, i++, scoreData->ScoreTime - scoresOneToOneDepth);
            TryBindStatementText(stmt, i++, scoreData->ScoreTxHash);
            TryBindStatementText(stmt, i++, scoreData->ContentAddressHash);

            if (sqlite3_step(*stmt) == SQLITE_ROW)
                if (auto[ok, value] = TryGetColumnInt(*stmt, 0); ok)
                    result = value;

            FinalizeSqlStatement(*stmt);
        });

        return result;
    }

    int ConsensusRepository::GetScoreCommentCount(
        int height,
        const shared_ptr<ScoreDataDto>& scoreData,
        const std::vector<int>& values,
        int64_t scoresOneToOneDepth)
    {
        int result = 0;

        if (values.empty())
            return result;

        // Build sql string
        string sql = R"sql(
            select count(1)
            from Transactions c indexed by Transactions_Type_Last_String1_Height_Id
            join Transactions s indexed by Transactions_Type_String1_String2_Height
                on  s.String2 = c.String2
                and s.Type in (301)
                and s.Height <= ?
                and s.String1 = ?
                and s.Height is not null
                and s.Time < ?
                and s.Time >= ?
                and s.Int1 in ( )sql" + join(values | transformed(static_cast<std::string(*)(int)>(std::to_string)), ",") + R"sql( )
                and s.Hash != ?
            where c.Type in (204, 205, 206)
              and c.Height is not null
              and c.String1 = ?
              and c.Last = 1
        )sql";

        // Execute
        TryTransactionStep(__func__, [&]()
        {
            auto stmt = SetupSqlStatement(sql);

            int i = 1;
            TryBindStatementInt(stmt, i++, height);
            TryBindStatementText(stmt, i++, scoreData->ScoreAddressHash);
            TryBindStatementInt64(stmt, i++, scoreData->ScoreTime);
            TryBindStatementInt64(stmt, i++, (int64_t) scoreData->ScoreTime - scoresOneToOneDepth);
            TryBindStatementText(stmt, i++, scoreData->ScoreTxHash);
            TryBindStatementText(stmt, i++, scoreData->ContentAddressHash);

            if (sqlite3_step(*stmt) == SQLITE_ROW)
                if (auto[ok, value] = TryGetColumnInt(*stmt, 0); ok)
                    result = value;

            FinalizeSqlStatement(*stmt);
        });

        return result;
    }

    tuple<bool, int64_t> ConsensusRepository::GetLastAccountTime(const string& address)
    {
        tuple<bool, int64_t> result = {false, 0};

        TryTransactionStep(__func__, [&]()
        {
            auto stmt = SetupSqlStatement(R"sql(
                select Time
                from Transactions indexed by Transactions_Type_Last_String1_Height_Id
                where Type in (100, 101, 102)
                  and String1 = ?
                  and Last = 1
                  and Height is not null
            )sql");

            TryBindStatementText(stmt, 1, address);

            if (sqlite3_step(*stmt) == SQLITE_ROW)
                if (auto[ok, value] = TryGetColumnInt64(*stmt, 0); ok)
                    result = {true, value};

            FinalizeSqlStatement(*stmt);
        });

        return result;
    }

    tuple<bool, int64_t> ConsensusRepository::GetLastAccountHeight(const string& address)
    {
        tuple<bool, int64_t> result = {false, 0};

        TryTransactionStep(__func__, [&]()
        {
            auto stmt = SetupSqlStatement(R"sql(
                select Height
                from Transactions indexed by Transactions_Type_Last_String1_Height_Id
                where Type in (100, 101, 102)
                  and String1 = ?
                  and Last = 1
                  and Height is not null
            )sql");

            TryBindStatementText(stmt, 1, address);

            if (sqlite3_step(*stmt) == SQLITE_ROW)
                if (auto[ok, value] = TryGetColumnInt64(*stmt, 0); ok)
                    result = {true, value};

            FinalizeSqlStatement(*stmt);
        });

        return result;
    }

    tuple<bool, int64_t> ConsensusRepository::GetTransactionHeight(const string& hash)
    {
        tuple<bool, int64_t> result = {false, 0};

        TryTransactionStep(__func__, [&]()
        {
            auto stmt = SetupSqlStatement(R"sql(
                select t.Height
                from Transactions t
                where t.Hash = ?
                  and t.Height is not null
            )sql");

            TryBindStatementText(stmt, 1, hash);

            if (sqlite3_step(*stmt) == SQLITE_ROW)
                if (auto[ok, value] = TryGetColumnInt64(*stmt, 0); ok)
                    result = {true, value};

            FinalizeSqlStatement(*stmt);
        });

        return result;
    }


    // Mempool counts

    int ConsensusRepository::CountMempoolBlocking(const string& address, const string& addressTo)
    {
        int result = 0;

        TryTransactionStep(__func__, [&]()
        {
            auto stmt = SetupSqlStatement(R"sql(
                select count(*)
                from Transactions
                where Type in (305, 306)
                  and Height is null
                  and String1 = ?
                  and String2 = ?
            )sql");

            TryBindStatementText(stmt, 1, address);
            TryBindStatementText(stmt, 2, addressTo);

            if (sqlite3_step(*stmt) == SQLITE_ROW)
                if (auto[ok, value] = TryGetColumnInt(*stmt, 0); ok)
                    result = value;

            FinalizeSqlStatement(*stmt);
        });

        return result;
    }
    int ConsensusRepository::CountMempoolSubscribe(const string& address, const string& addressTo)
    {
        int result = 0;

        TryTransactionStep(__func__, [&]()
        {
            auto stmt = SetupSqlStatement(R"sql(
                select count(*)
                from Transactions
                where Type in (302, 303, 304)
                  and Height is null
                  and String1 = ?
                  and String2 = ?
            )sql");

            TryBindStatementText(stmt, 1, address);
            TryBindStatementText(stmt, 2, addressTo);

            if (sqlite3_step(*stmt) == SQLITE_ROW)
                if (auto[ok, value] = TryGetColumnInt(*stmt, 0); ok)
                    result = value;

            FinalizeSqlStatement(*stmt);
        });

        return result;
    }

    int ConsensusRepository::CountMempoolComment(const string& address)
    {
        int result = 0;

        TryTransactionStep(__func__, [&]()
        {
            auto stmt = SetupSqlStatement(R"sql(
                select count(*)
                from Transactions indexed by Transactions_Type_String1_Height_Time_Int1
                where Type in (204)
                  and Height is null
                  and String1 = ?
                  and Hash = String2
            )sql");

            TryBindStatementText(stmt, 1, address);

            if (sqlite3_step(*stmt) == SQLITE_ROW)
                if (auto[ok, value] = TryGetColumnInt(*stmt, 0); ok)
                    result = value;

            FinalizeSqlStatement(*stmt);
        });

        return result;
    }
    int ConsensusRepository::CountChainCommentTime(const string& address, int64_t time)
    {
        int result = 0;

        TryTransactionStep(__func__, [&]()
        {
            auto stmt = SetupSqlStatement(R"sql(
                select count(*)
                from Transactions indexed by Transactions_Type_String1_Height_Time_Int1
                where Type in (204)
                  and Height is not null
                  and Time >= ?
                  and String1 = ?
                  and Hash = String2
            )sql");

            TryBindStatementInt64(stmt, 1, time);
            TryBindStatementText(stmt, 2, address);

            if (sqlite3_step(*stmt) == SQLITE_ROW)
                if (auto[ok, value] = TryGetColumnInt(*stmt, 0); ok)
                    result = value;

            FinalizeSqlStatement(*stmt);
        });

        return result;
    }
    int ConsensusRepository::CountChainCommentHeight(const string& address, int height)
    {
        int result = 0;

        TryTransactionStep(__func__, [&]()
        {
            auto stmt = SetupSqlStatement(R"sql(
                select count(*)
                from Transactions indexed by Transactions_Type_Last_String1_Height_Id
                where Type in (204,205,206)
                  and Last = 1
                  and Height is not null
                  and Height >= ?
                  and String1 = ?
            )sql");

            TryBindStatementInt(stmt, 1, height);
            TryBindStatementText(stmt, 2, address);

            if (sqlite3_step(*stmt) == SQLITE_ROW)
                if (auto[ok, value] = TryGetColumnInt(*stmt, 0); ok)
                    result = value;

            FinalizeSqlStatement(*stmt);
        });

        return result;
    }

    int ConsensusRepository::CountMempoolComplain(const string& address)
    {
        int result = 0;

        TryTransactionStep(__func__, [&]()
        {
            auto stmt = SetupSqlStatement(R"sql(
                select count(*)
                from Transactions
                where Type in (307)
                  and Height is null
                  and String1 = ?
                  and Hash = String2
            )sql");

            TryBindStatementText(stmt, 1, address);

            if (sqlite3_step(*stmt) == SQLITE_ROW)
                if (auto[ok, value] = TryGetColumnInt(*stmt, 0); ok)
                    result = value;

            FinalizeSqlStatement(*stmt);
        });

        return result;
    }
    int ConsensusRepository::CountChainComplainTime(const string& address, int64_t time)
    {
        int result = 0;

        TryTransactionStep(__func__, [&]()
        {
            auto stmt = SetupSqlStatement(R"sql(
                select count(*)
                from Transactions
                where Type in (307)
                  and Height is not null
                  and Time >= ?
                  and String1 = ?
                  and Hash = String2
            )sql");

            TryBindStatementInt64(stmt, 1, time);
            TryBindStatementText(stmt, 2, address);

            if (sqlite3_step(*stmt) == SQLITE_ROW)
                if (auto[ok, value] = TryGetColumnInt(*stmt, 0); ok)
                    result = value;

            FinalizeSqlStatement(*stmt);
        });

        return result;
    }
    int ConsensusRepository::CountChainComplainHeight(const string& address, int height)
    {
        int result = 0;

        TryTransactionStep(__func__, [&]()
        {
            auto stmt = SetupSqlStatement(R"sql(
                select count(*)
                from Transactions
                where Type in (307)
                  and Height is not null
                  and Height >= ?
                  and String1 = ?
                  and Hash = String2
            )sql");

            TryBindStatementInt(stmt, 1, height);
            TryBindStatementText(stmt, 2, address);

            if (sqlite3_step(*stmt) == SQLITE_ROW)
                if (auto[ok, value] = TryGetColumnInt(*stmt, 0); ok)
                    result = value;

            FinalizeSqlStatement(*stmt);
        });

        return result;
    }

    int ConsensusRepository::CountMempoolPost(const string& address)
    {
        int result = 0;

        TryTransactionStep(__func__, [&]()
        {
            auto stmt = SetupSqlStatement(R"sql(
                select count(*)
                from Transactions indexed by Transactions_Type_String1_Height_Time_Int1
                where Type in (200)
                  and Height is null
                  and String1 = ?
                  and Hash = String2
            )sql");

            TryBindStatementText(stmt, 1, address);

            if (sqlite3_step(*stmt) == SQLITE_ROW)
                if (auto[ok, value] = TryGetColumnInt(*stmt, 0); ok)
                    result = value;

            FinalizeSqlStatement(*stmt);
        });

        return result;
    }
    int ConsensusRepository::CountChainPostTime(const string& address, int64_t time)
    {
        int result = 0;

        TryTransactionStep(__func__, [&]()
        {
            auto stmt = SetupSqlStatement(R"sql(
                select count(*)
                from Transactions indexed by Transactions_Type_String1_Height_Time_Int1
                where Type in (200)
                  and Height is not null
                  and String1 = ?
                  and Time >= ?
                  and Hash = String2
            )sql");

            TryBindStatementText(stmt, 1, address);
            TryBindStatementInt64(stmt, 2, time);

            if (sqlite3_step(*stmt) == SQLITE_ROW)
                if (auto[ok, value] = TryGetColumnInt(*stmt, 0); ok)
                    result = value;

            FinalizeSqlStatement(*stmt);
        });

        return result;
    }
    int ConsensusRepository::CountChainPostHeight(const string& address, int height)
    {
        int result = 0;

        TryTransactionStep(__func__, [&]()
        {
            auto stmt = SetupSqlStatement(R"sql(
                select count(*)
                from Transactions indexed by Transactions_Type_String1_Height_Time_Int1
                where Type in (200)
                  and String1 = ?
                  and Height >= ?
                  and Hash = String2
            )sql");

            TryBindStatementText(stmt, 1, address);
            TryBindStatementInt(stmt, 2, height);

            if (sqlite3_step(*stmt) == SQLITE_ROW)
                if (auto[ok, value] = TryGetColumnInt(*stmt, 0); ok)
                    result = value;

            FinalizeSqlStatement(*stmt);
        });

        return result;
    }

    int ConsensusRepository::CountMempoolVideo(const string& address)
    {
        int result = 0;

        TryTransactionStep(__func__, [&]()
        {
            auto stmt = SetupSqlStatement(R"sql(
                select count(*)
                from Transactions indexed by Transactions_Type_String1_Height_Time_Int1
                where Type in (201)
                  and Height is null
                  and String1 = ?
                  and Hash = String2
            )sql");

            TryBindStatementText(stmt, 1, address);

            if (sqlite3_step(*stmt) == SQLITE_ROW)
                if (auto[ok, value] = TryGetColumnInt(*stmt, 0); ok)
                    result = value;

            FinalizeSqlStatement(*stmt);
        });

        return result;
    }
    int ConsensusRepository::CountChainVideo(const string& address, int height)
    {
        int result = 0;

        TryTransactionStep(__func__, [&]()
        {
            auto stmt = SetupSqlStatement(R"sql(
                select count(*)
                from Transactions indexed by Transactions_Type_String1_Height_Time_Int1
                where Type in (201)
                  and String1 = ?
                  and Height >= ?
                  and Hash = String2
            )sql");

            TryBindStatementText(stmt, 1, address);
            TryBindStatementInt(stmt, 2, height);

            if (sqlite3_step(*stmt) == SQLITE_ROW)
                if (auto[ok, value] = TryGetColumnInt(*stmt, 0); ok)
                    result = value;

            FinalizeSqlStatement(*stmt);
        });

        return result;
    }

    int ConsensusRepository::CountMempoolArticle(const string& address)
    {
        int result = 0;

        TryTransactionStep(__func__, [&]()
        {
            auto stmt = SetupSqlStatement(R"sql(
                select count(*)
                from Transactions indexed by Transactions_Type_String1_Height_Time_Int1
                where Type in (202)
                  and Height is null
                  and String1 = ?
                  and Hash = String2
            )sql");

            TryBindStatementText(stmt, 1, address);

            if (sqlite3_step(*stmt) == SQLITE_ROW)
                if (auto[ok, value] = TryGetColumnInt(*stmt, 0); ok)
                    result = value;

            FinalizeSqlStatement(*stmt);
        });

        return result;
    }
    int ConsensusRepository::CountChainArticle(const string& address, int height)
    {
        int result = 0;

        TryTransactionStep(__func__, [&]()
        {
            auto stmt = SetupSqlStatement(R"sql(
                select count(*)
                from Transactions indexed by Transactions_Type_String1_Height_Time_Int1
                where Type in (202)
                  and String1 = ?
                  and Height >= ?
                  and Hash = String2
            )sql");

            TryBindStatementText(stmt, 1, address);
            TryBindStatementInt(stmt, 2, height);

            if (sqlite3_step(*stmt) == SQLITE_ROW)
                if (auto[ok, value] = TryGetColumnInt(*stmt, 0); ok)
                    result = value;

            FinalizeSqlStatement(*stmt);
        });

        return result;
    }

    int ConsensusRepository::CountMempoolScoreComment(const string& address)
    {
        int result = 0;

        TryTransactionStep(__func__, [&]()
        {
            auto stmt = SetupSqlStatement(R"sql(
                select count(*)
                from Transactions indexed by Transactions_Type_String1_Height_Time_Int1
                where Type in (301)
                  and Height is null
                  and String1 = ?
            )sql");

            TryBindStatementText(stmt, 1, address);

            if (sqlite3_step(*stmt) == SQLITE_ROW)
                if (auto[ok, value] = TryGetColumnInt(*stmt, 0); ok)
                    result = value;

            FinalizeSqlStatement(*stmt);
        });

        return result;
    }
    int ConsensusRepository::CountChainScoreCommentTime(const string& address, int64_t time)
    {
        int result = 0;

        TryTransactionStep(__func__, [&]()
        {
            auto stmt = SetupSqlStatement(R"sql(
                select count(*)
                from Transactions indexed by Transactions_Type_String1_Height_Time_Int1
                where Type in (301)
                  and Height is not null
                  and String1 = ?
                  and Time >= ?
            )sql");

            TryBindStatementText(stmt, 1, address);
            TryBindStatementInt64(stmt, 2, time);

            if (sqlite3_step(*stmt) == SQLITE_ROW)
                if (auto[ok, value] = TryGetColumnInt(*stmt, 0); ok)
                    result = value;

            FinalizeSqlStatement(*stmt);
        });

        return result;
    }
    int ConsensusRepository::CountChainScoreCommentHeight(const string& address, int height)
    {
        int result = 0;

        TryTransactionStep(__func__, [&]()
        {
            auto stmt = SetupSqlStatement(R"sql(
                select count(*)
                from Transactions indexed by Transactions_Type_String1_Height_Time_Int1
                where Type in (301)
                  and Height is not null
                  and Height >= ?
                  and String1 = ?
            )sql");

            TryBindStatementInt(stmt, 1, height);
            TryBindStatementText(stmt, 2, address);

            if (sqlite3_step(*stmt) == SQLITE_ROW)
                if (auto[ok, value] = TryGetColumnInt(*stmt, 0); ok)
                    result = value;

            FinalizeSqlStatement(*stmt);
        });

        return result;
    }

    int ConsensusRepository::CountMempoolScoreContent(const string& address)
    {
        int result = 0;

        TryTransactionStep(__func__, [&]()
        {
            auto stmt = SetupSqlStatement(R"sql(
                select count(*)
                from Transactions indexed by Transactions_Type_String1_Height_Time_Int1
                where Type in (300)
                  and Height is null
                  and String1 = ?
            )sql");

            TryBindStatementText(stmt, 1, address);

            if (sqlite3_step(*stmt) == SQLITE_ROW)
                if (auto[ok, value] = TryGetColumnInt(*stmt, 0); ok)
                    result = value;

            FinalizeSqlStatement(*stmt);
        });

        return result;
    }
    int ConsensusRepository::CountChainScoreContentTime(const string& address, int64_t time)
    {
        int result = 0;

        TryTransactionStep(__func__, [&]()
        {
            auto stmt = SetupSqlStatement(R"sql(
                select count(*)
                from Transactions indexed by Transactions_Type_String1_Height_Time_Int1
                where Type in (300)
                  and Height is not null
                  and String1 = ?
                  and Time >= ?
            )sql");

            TryBindStatementText(stmt, 1, address);
            TryBindStatementInt64(stmt, 2, time);

            if (sqlite3_step(*stmt) == SQLITE_ROW)
                if (auto[ok, value] = TryGetColumnInt(*stmt, 0); ok)
                    result = value;

            FinalizeSqlStatement(*stmt);
        });

        return result;
    }
    int ConsensusRepository::CountChainScoreContentHeight(const string& address, int height)
    {
        int result = 0;

        TryTransactionStep(__func__, [&]()
        {
            auto stmt = SetupSqlStatement(R"sql(
                select count(*)
                from Transactions indexed by Transactions_Type_String1_Height_Time_Int1
                where Type in (300)
                  and Height is not null
                  and Height >= ?
                  and String1 = ?
            )sql");

            TryBindStatementInt(stmt, 1, height);
            TryBindStatementText(stmt, 2, address);

            if (sqlite3_step(*stmt) == SQLITE_ROW)
                if (auto[ok, value] = TryGetColumnInt(*stmt, 0); ok)
                    result = value;

            FinalizeSqlStatement(*stmt);
        });

        return result;
    }

    int ConsensusRepository::CountMempoolUser(const string& address)
    {
        int result = 0;

        TryTransactionStep(__func__, [&]()
        {
            auto stmt = SetupSqlStatement(R"sql(
                select count(*)
                from Transactions
                where Type in (100)
                  and Height is null
                  and String1 = ?
            )sql");

            TryBindStatementText(stmt, 1, address);

            if (sqlite3_step(*stmt) == SQLITE_ROW)
                if (auto[ok, value] = TryGetColumnInt(*stmt, 0); ok)
                    result = value;

            FinalizeSqlStatement(*stmt);
        });

        return result;
    }

    int ConsensusRepository::CountMempoolAccountSetting(const string& address)
    {
        int result = 0;

        TryTransactionStep(__func__, [&]()
        {
            auto stmt = SetupSqlStatement(R"sql(
                select count(*)
                from Transactions
                where Type in (103)
                  and Height is null
                  and String1 = ?
            )sql");

            TryBindStatementText(stmt, 1, address);

            if (sqlite3_step(*stmt) == SQLITE_ROW)
                if (auto[ok, value] = TryGetColumnInt(*stmt, 0); ok)
                    result = value;

            FinalizeSqlStatement(*stmt);
        });

        return result;
    }
    int ConsensusRepository::CountChainAccountSetting(const string& address, int height)
    {
        int result = 0;

        TryTransactionStep(__func__, [&]()
        {
            auto stmt = SetupSqlStatement(R"sql(
                select count(*)
                from Transactions indexed by Transactions_Type_String1_Height_Time_Int1
                where Type in (103)
                  and Height is not null
                  and Height >= ?
                  and String1 = ?
            )sql");

            TryBindStatementText(stmt, 1, address);
            TryBindStatementInt64(stmt, 2, height);

            if (sqlite3_step(*stmt) == SQLITE_ROW)
                if (auto[ok, value] = TryGetColumnInt(*stmt, 0); ok)
                    result = value;

            FinalizeSqlStatement(*stmt);
        });

        return result;
    }

    int ConsensusRepository::CountChainAccount(TxType txType, const string& address, int height)
    {
        int result = 0;

        TryTransactionStep(__func__, [&]()
        {
            auto stmt = SetupSqlStatement(R"sql(
                select count(*)
                from Transactions indexed by Transactions_Type_String1_Height_Time_Int1
                where Type in (?)
                  and Height is not null
                  and Height >= ?
                  and String1 = ?
            )sql");

            TryBindStatementInt(stmt, 1, (int)txType);
            TryBindStatementInt(stmt, 2, height);
            TryBindStatementText(stmt, 3, address);

            if (sqlite3_step(*stmt) == SQLITE_ROW)
                if (auto[ok, value] = TryGetColumnInt(*stmt, 0); ok)
                    result = value;

            FinalizeSqlStatement(*stmt);
        });

        return result;
    }

    // EDITS

    int ConsensusRepository::CountMempoolCommentEdit(const string& address, const string& rootTxHash)
    {
        int result = 0;

        TryTransactionStep(__func__, [&]()
        {
            auto stmt = SetupSqlStatement(R"sql(
                select count(*)
                from Transactions indexed by Transactions_Type_String1_String2_Height
                where Type in (204,205,206)
                  and Height is null
                  and String1 = ?
                  and String2 = ?
            )sql");

            TryBindStatementText(stmt, 1, address);
            TryBindStatementText(stmt, 2, rootTxHash);

            if (sqlite3_step(*stmt) == SQLITE_ROW)
                if (auto[ok, value] = TryGetColumnInt(*stmt, 0); ok)
                    result = value;

            FinalizeSqlStatement(*stmt);
        });

        return result;
    }
    int ConsensusRepository::CountChainCommentEdit(const string& address, const string& rootTxHash)
    {
        int result = 0;

        TryTransactionStep(__func__, [&]()
        {
            auto stmt = SetupSqlStatement(R"sql(
                select count(*)
                from Transactions indexed by Transactions_Type_String1_String2_Height
                where Type in (204,205,206)
                  and Height is not null
                  and Hash != String2
                  and String1 = ?
                  and String2 = ?
            )sql");

            TryBindStatementText(stmt, 1, address);
            TryBindStatementText(stmt, 2, rootTxHash);

            if (sqlite3_step(*stmt) == SQLITE_ROW)
                if (auto[ok, value] = TryGetColumnInt(*stmt, 0); ok)
                    result = value;

            FinalizeSqlStatement(*stmt);
        });

        return result;
    }

    int ConsensusRepository::CountMempoolPostEdit(const string& address, const string& rootTxHash)
    {
        int result = 0;

        TryTransactionStep(__func__, [&]()
        {
            auto stmt = SetupSqlStatement(R"sql(
                select count(*)
                from Transactions indexed by Transactions_Type_String1_String2_Height
                where Type in (200,207)
                  and Height is null
                  and String1 = ?
                  and String2 = ?
            )sql");

            TryBindStatementText(stmt, 1, address);
            TryBindStatementText(stmt, 2, rootTxHash);

            if (sqlite3_step(*stmt) == SQLITE_ROW)
                if (auto[ok, value] = TryGetColumnInt(*stmt, 0); ok)
                    result = value;

            FinalizeSqlStatement(*stmt);
        });

        return result;
    }
    int ConsensusRepository::CountChainPostEdit(const string& address, const string& rootTxHash)
    {
        int result = 0;

        TryTransactionStep(__func__, [&]()
        {
            auto stmt = SetupSqlStatement(R"sql(
                select count(*)
                from Transactions indexed by Transactions_Type_String1_String2_Height
                where Type in (200)
                  and Height is not null
                  and Hash != String2
                  and String1 = ?
                  and String2 = ?
            )sql");

            TryBindStatementText(stmt, 1, address);
            TryBindStatementText(stmt, 2, rootTxHash);

            if (sqlite3_step(*stmt) == SQLITE_ROW)
                if (auto[ok, value] = TryGetColumnInt(*stmt, 0); ok)
                    result = value;

            FinalizeSqlStatement(*stmt);
        });

        return result;
    }

    int ConsensusRepository::CountMempoolVideoEdit(const string& address, const string& rootTxHash)
    {
        int result = 0;

        TryTransactionStep(__func__, [&]()
        {
            auto stmt = SetupSqlStatement(R"sql(
                select count(*)
                from Transactions indexed by Transactions_Type_String1_String2_Height
                where Type in (201,207)
                  and Height is null
                  and String1 = ?
                  and String2 = ?
            )sql");

            TryBindStatementText(stmt, 1, address);
            TryBindStatementText(stmt, 2, rootTxHash);

            if (sqlite3_step(*stmt) == SQLITE_ROW)
                if (auto[ok, value] = TryGetColumnInt(*stmt, 0); ok)
                    result = value;

            FinalizeSqlStatement(*stmt);
        });

        return result;
    }
    int ConsensusRepository::CountChainVideoEdit(const string& address, const string& rootTxHash)
    {
        int result = 0;

        TryTransactionStep(__func__, [&]()
        {
            auto stmt = SetupSqlStatement(R"sql(
                select count(*)
                from Transactions indexed by Transactions_Type_String1_String2_Height
                where Type in (201)
                  and Height is not null
                  and Hash != String2
                  and String1 = ?
                  and String2 = ?
            )sql");

            TryBindStatementText(stmt, 1, address);
            TryBindStatementText(stmt, 2, rootTxHash);

            if (sqlite3_step(*stmt) == SQLITE_ROW)
                if (auto[ok, value] = TryGetColumnInt(*stmt, 0); ok)
                    result = value;

            FinalizeSqlStatement(*stmt);
        });

        return result;
    }

    int ConsensusRepository::CountMempoolArticleEdit(const string& address, const string& rootTxHash)
    {
        int result = 0;

        TryTransactionStep(__func__, [&]()
        {
            auto stmt = SetupSqlStatement(R"sql(
                select count(*)
                from Transactions indexed by Transactions_Type_String1_String2_Height
                where Type in (202,207)
                  and Height is null
                  and String1 = ?
                  and String2 = ?
            )sql");

            TryBindStatementText(stmt, 1, address);
            TryBindStatementText(stmt, 2, rootTxHash);

            if (sqlite3_step(*stmt) == SQLITE_ROW)
                if (auto[ok, value] = TryGetColumnInt(*stmt, 0); ok)
                    result = value;

            FinalizeSqlStatement(*stmt);
        });

        return result;
    }
    int ConsensusRepository::CountChainArticleEdit(const string& address, const string& rootTxHash)
    {
        int result = 0;

        TryTransactionStep(__func__, [&]()
        {
            auto stmt = SetupSqlStatement(R"sql(
                select count(*)
                from Transactions indexed by Transactions_Type_String1_String2_Height
                where Type in (202)
                  and Height is not null
                  and Hash != String2
                  and String1 = ?
                  and String2 = ?
            )sql");

            TryBindStatementText(stmt, 1, address);
            TryBindStatementText(stmt, 2, rootTxHash);

            if (sqlite3_step(*stmt) == SQLITE_ROW)
                if (auto[ok, value] = TryGetColumnInt(*stmt, 0); ok)
                    result = value;

            FinalizeSqlStatement(*stmt);
        });

        return result;
    }

    int ConsensusRepository::CountMempoolContentDelete(const string& address, const string& rootTxHash)
    {
        int result = 0;

        TryTransactionStep(__func__, [&]()
        {
            auto stmt = SetupSqlStatement(R"sql(
                select count(*)
                from Transactions indexed by Transactions_Type_String1_String2_Height
                where Type in (200,201,202,207)
                  and Height is null
                  and String1 = ?
                  and String2 = ?
            )sql");

            TryBindStatementText(stmt, 1, address);
            TryBindStatementText(stmt, 2, rootTxHash);

            if (sqlite3_step(*stmt) == SQLITE_ROW)
                if (auto[ok, value] = TryGetColumnInt(*stmt, 0); ok)
                    result = value;

            FinalizeSqlStatement(*stmt);
        });

        return result;
    }

}