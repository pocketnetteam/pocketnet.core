//
// Created by Joknek on 9/30/2021.
//

#include "NotifierRepository.h"

namespace PocketDb
{
    void NotifierRepository::Init() {}

    void NotifierRepository::Destroy() {}

    UniValue NotifierRepository::GetPostLang(const string &postHash)
    {
        UniValue result(UniValue::VOBJ);

        string sql = R"sql(
            select p.String1 Lang from Transactions t
            inner join Payload p on t.Hash = p.TxHash
            where Type in (200, 201, 202, 203) and t.Hash = ?
        )sql";

        TryTransactionStep(__func__, [&]()
        {
            auto stmt = SetupSqlStatement(sql);

            TryBindStatementText(stmt, 1, postHash);

            if (sqlite3_step(*stmt) == SQLITE_ROW)
            {
                if (auto[ok, value] = TryGetColumnString(*stmt, 0); ok) result.pushKV("lang", value);
            }

            FinalizeSqlStatement(*stmt);
        });

        return result;
    }

    UniValue NotifierRepository::GetOriginalPostAddressByRepost(const string &repostHash)
    {
        UniValue result(UniValue::VOBJ);

        string sql = R"sql(
            SELECT t.Hash, t.String1 Address
            FROM Transactions t
            INNER JOIN Transactions tRepost ON t.Hash = tRepost.String3 --RelayTxHash
            where tRepost.Type in (200, 201, 202, 203) and tRepost.Hash = ?
        )sql";

        TryTransactionStep(__func__, [&]()
        {
            auto stmt = SetupSqlStatement(sql);

            TryBindStatementText(stmt, 1, repostHash);

            if (sqlite3_step(*stmt) == SQLITE_ROW)
            {
                if (auto[ok, value] = TryGetColumnString(*stmt, 0); ok) result.pushKV("hash", value);
                if (auto[ok, value] = TryGetColumnString(*stmt, 1); ok) result.pushKV("address", value);
            }

            FinalizeSqlStatement(*stmt);
        });

        return result;
    }

    UniValue NotifierRepository::GetPrivateSubscribeAddressesByAddressTo(const string &addressTo)
    {
        UniValue result(UniValue::VARR);

        string sql = R"sql(
            SELECT t.String1 Address
            FROM Transactions t
            WHERE t.Type = 303 and t.String2 = ?
        )sql";

        TryTransactionStep(__func__, [&]()
        {
            auto stmt = SetupSqlStatement(sql);

            TryBindStatementText(stmt, 1, addressTo);

            while (sqlite3_step(*stmt) == SQLITE_ROW)
            {
                if (auto[ok, value] = TryGetColumnString(*stmt, 0); ok)
                {
                    UniValue record(UniValue::VOBJ);
                    record.pushKV("address", value);

                    result.push_back(record);
                }
            }

            FinalizeSqlStatement(*stmt);
        });

        return result;
    }

    UniValue NotifierRepository::GetUserReferrerAddress(const string &userHash)
    {
        UniValue result(UniValue::VOBJ);

        string sql = R"sql(
            SELECT t.String2 as ReferrerAddress
            FROM Transactions t
            WHERE t.Type = 100 and t.String2 is not null and t.Hash = ?
        )sql";

        TryTransactionStep(__func__, [&]()
        {
            auto stmt = SetupSqlStatement(sql);

            TryBindStatementText(stmt, 1, userHash);

            if (sqlite3_step(*stmt) == SQLITE_ROW)
            {
                if (auto[ok, value] = TryGetColumnString(*stmt, 0); ok) result.pushKV("referrerAddress", value);
            }

            FinalizeSqlStatement(*stmt);
        });

        return result;
    }

    UniValue NotifierRepository::GetPostInfoAddressByScore(const string &postScoreHash)
    {
        UniValue result(UniValue::VOBJ);

        string sql = R"sql(
            SELECT score.String2 PostTxHash, score.Int1 Value, post.String1 PostAddress
            FROM Transactions score
            INNER JOIN Transactions post ON post.Hash = score.String2
            WHERE score.Type IN (300) and score.Hash = ?
        )sql";

        TryTransactionStep(__func__, [&]()
        {
            auto stmt = SetupSqlStatement(sql);

            TryBindStatementText(stmt, 1, postScoreHash);

            if (sqlite3_step(*stmt) == SQLITE_ROW)
            {
                if (auto[ok, value] = TryGetColumnString(*stmt, 0); ok) result.pushKV("postTxHash", value);
                if (auto[ok, value] = TryGetColumnString(*stmt, 1); ok) result.pushKV("value", value);
                if (auto[ok, value] = TryGetColumnString(*stmt, 2); ok) result.pushKV("postAddress", value);
            }

            FinalizeSqlStatement(*stmt);
        });

        return result;
    }

    UniValue NotifierRepository::GetSubscribeAddressTo(const string &subscribeHash)
    {
        UniValue result(UniValue::VOBJ);

        string sql = R"sql(
            SELECT t.String2 AddressTo
            FROM Transactions t
            WHERE t.Type IN (302, 303, 304) AND t.Hash = ?
        )sql";

        TryTransactionStep(__func__, [&]()
        {
            auto stmt = SetupSqlStatement(sql);

            TryBindStatementText(stmt, 1, subscribeHash);

            if (sqlite3_step(*stmt) == SQLITE_ROW)
            {
                if (auto[ok, value] = TryGetColumnString(*stmt, 0); ok) result.pushKV("addressTo", value);
            }

            FinalizeSqlStatement(*stmt);
        });

        return result;
    }

    UniValue NotifierRepository::GetCommentInfoAddressByScore(const string &commentScoreHash)
    {
        UniValue result(UniValue::VOBJ);

        string sql = R"sql(
            SELECT score.String2 CommentHash, score.Int1 Value, comment.String1 CommentAddress
            FROM Transactions score
            INNER JOIN Transactions comment ON score.String2 = comment.String2
            WHERE score.Type = 301 and comment.Last = 1 and score.Hash = ?
        )sql";

        TryTransactionStep(__func__, [&]()
        {
            auto stmt = SetupSqlStatement(sql);

            TryBindStatementText(stmt, 1, commentScoreHash);

            if (sqlite3_step(*stmt) == SQLITE_ROW)
            {
                if (auto[ok, value] = TryGetColumnString(*stmt, 0); ok) result.pushKV("commentHash", value);
                if (auto[ok, value] = TryGetColumnString(*stmt, 1); ok) result.pushKV("value", value);
                if (auto[ok, value] = TryGetColumnString(*stmt, 2); ok) result.pushKV("commentAddress", value);
            }

            FinalizeSqlStatement(*stmt);
        });

        return result;
    }

    UniValue NotifierRepository::GetFullCommentInfo(const string &commentHash)
    {
        UniValue result(UniValue::VOBJ);

        string sql = R"sql(
            SELECT comment.String3 PostHash, comment.String4 ParentHash, comment.String5 AnswerHash, comment.String2 RootHash, post.String1 PostAddress, answer.String1 AnswerAddress
            FROM Transactions comment
            INNER JOIN Transactions post ON comment.String3 = post.Hash
            LEFT JOIN Transactions answer ON comment.String5 = answer.String2 and answer.Last = 1
            WHERE comment.Type IN (204, 205, 206) and comment.Hash = ?
        )sql";

        TryTransactionStep(__func__, [&]()
        {
            auto stmt = SetupSqlStatement(sql);

            TryBindStatementText(stmt, 1, commentHash);

            if (sqlite3_step(*stmt) == SQLITE_ROW)
            {
                if (auto[ok, value] = TryGetColumnString(*stmt, 0); ok) result.pushKV("postHash", value);
                if (auto[ok, value] = TryGetColumnString(*stmt, 1); ok) result.pushKV("parentHash", value);
                if (auto[ok, value] = TryGetColumnString(*stmt, 2); ok) result.pushKV("answerHash", value);
                if (auto[ok, value] = TryGetColumnString(*stmt, 3); ok) result.pushKV("rootHash", value);
                if (auto[ok, value] = TryGetColumnString(*stmt, 4); ok) result.pushKV("postAddress", value);
                if (auto[ok, value] = TryGetColumnString(*stmt, 5); ok) result.pushKV("answerAddress", value);
            }

            FinalizeSqlStatement(*stmt);
        });

        return result;
    }
}