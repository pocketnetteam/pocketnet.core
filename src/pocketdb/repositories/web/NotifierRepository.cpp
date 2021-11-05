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
            select p.String1 Lang
            from Transactions t
            join Payload p on p.TxHash = t.Hash
            where t.Type in (200, 201, 202, 203)
              and t.Hash = ?
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
            select t.Hash,
                   t.String1 Address
            from Transactions t
            join Transactions tRepost on tRepost.String3 = t.Hash
            where tRepost.Type in (200, 201, 202, 203)
              and tRepost.Hash = ?
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
            select String1 as Address
            from Transactions indexed by Transactions_Type_Last_String2_Height
            where Type in (303)
              and Last = 1
              and Height is not null
              and String2 = ?
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
            select String2 as ReferrerAddress
            from Transactions
            where Type in (100)
              and String2 is not null
              and Hash = ?
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
            select score.String2 PostTxHash,
                   score.Int1 Value,
                   post.String1 PostAddress
            from Transactions score
            join Transactions post on post.Hash = score.String2
            where score.Type in (300)
              and score.Hash = ?
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
            select String2 AddressTo
            from Transactions
            where Type in (302, 303, 304)
              and Hash = ?
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
            select
                score.String2 CommentHash,
                score.Int1 Value,
                comment.String1 CommentAddress
            from Transactions score
            join Transactions comment on score.String2 = comment.Hash
            where score.Hash = ?
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
            select
                comment.String3 PostHash,
                comment.String4 ParentHash,
                comment.String5 AnswerHash,
                comment.String2 RootHash,
                content.String1 ContentAddress,
                answer.String1 AnswerAddress
            from Transactions comment -- sqlite_autoindex_Transactions_1 (Hash)
            join Transactions content -- sqlite_autoindex_Transactions_1 (Hash)
                on content.Type in (200, 201) and content.Hash = comment.String3
            left join Transactions answer indexed by Transactions_Type_Last_String2_Height
                on answer.Type in (204, 205) and answer.Last = 1 and answer.String2 = comment.String5
            WHERE comment.Type in (204, 205)
              and comment.Hash = ?
        )sql";

        TryTransactionStep(__func__, [&]()
        {
            auto stmt = SetupSqlStatement(sql);

            TryBindStatementText(stmt, 1, commentHash);

            if (sqlite3_step(*stmt) == SQLITE_ROW)
            {
                if (auto[ok, value] = TryGetColumnString(*stmt, 0); ok) result.pushKV("postHash", value);
                if (auto[ok, value] = TryGetColumnString(*stmt, 1); ok) result.pushKV("parentHash", value); else result.pushKV("parentHash", "");
                if (auto[ok, value] = TryGetColumnString(*stmt, 2); ok) result.pushKV("answerHash", value); else result.pushKV("answerHash", "");
                if (auto[ok, value] = TryGetColumnString(*stmt, 3); ok) result.pushKV("rootHash", value);
                if (auto[ok, value] = TryGetColumnString(*stmt, 4); ok) result.pushKV("postAddress", value);
                if (auto[ok, value] = TryGetColumnString(*stmt, 5); ok) result.pushKV("answerAddress", value);
            }

            FinalizeSqlStatement(*stmt);
        });

        return result;
    }

    UniValue NotifierRepository::GetPostCountFromMySubscribes(const string& address, int height)
    {
        UniValue result(UniValue::VOBJ);

        string sql = R"sql(
            select count(1)
            from Transactions sub
            join Transactions post
                on post.String1 = sub.String2 and post.Type in (200, 201, 202, 203) and post.Last = 1
            where sub.Type in (302, 303)
              and sub.Last = 1
              and post.Height = ?
              and sub.String1 = ?
        )sql";

        TryTransactionStep(__func__, [&]()
        {
            auto stmt = SetupSqlStatement(sql);

            TryBindStatementInt(stmt, 1, height);
            TryBindStatementText(stmt, 2, address);

            if (sqlite3_step(*stmt) == SQLITE_ROW)
            {
                if (auto[ok, value] = TryGetColumnInt(*stmt, 0); ok) result.pushKV("count", value);
            }

            FinalizeSqlStatement(*stmt);
        });

        return result;
    }
}