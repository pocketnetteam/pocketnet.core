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
        auto funcName = __func__;

        auto sql = R"sql(
            SELECT 1
            FROM vAccountsPayload ap
            WHERE ap.Name = ?
            and not exists (
                select 1
                from vAccounts ac
                where ac.Hash = ap.TxHash
                  and ac.AddressHash = ?
            )
        )sql";

        return TryTransactionStep([&]() {
            auto stmt = SetupSqlStatement(sql);

            auto bindResult = TryBindStatementText(stmt, 1, name);
            bindResult &= TryBindStatementText(stmt, 2, address);
            if (!bindResult) {
                FinalizeSqlStatement(*stmt);
                throw runtime_error(strprintf("%s: bind error\n", funcName));
            }

            bool result = sqlite3_step(*stmt) == SQLITE_ROW;
            FinalizeSqlStatement(*stmt);
            return result;
        });
    }

    // Select all user profile edit transaction in chain
    // Transactions.Height is not null
    // TODO (brangr): change vUser to vAccounts and pass argument type
    tuple<bool, shared_ptr<Transaction>> ConsensusRepository::GetLastAccountTransaction(const string& address)
    {
        auto funcName = __func__;
        shared_ptr<Transaction> tx;

        auto sql = R"sql(
            SELECT u.Type, u.Hash, u.Time, u.Height, u.AddressHash, u.ReferrerAddressHash, u.String3, u.String4, u.String5, u.Int1,
                p.TxHash pTxHash, p.String1 pString1, p.String2 pString2, p.String3 pString3, p.String4 pString4, p.String5 pString5, p.String6 pString6, p.String7 pString7
            FROM vUsers u
            LEFT JOIN Payload p on p.TxHash = u.Hash
            WHERE u.AddressHash = ?
            order by u.Height desc
            limit 1
        )sql";

        bool tryResult = TryTransactionStep([&]()
        {
            auto stmt = SetupSqlStatement(sql);

            auto bindResult = TryBindStatementText(stmt, 1, address);
            if (!bindResult)
            {
                FinalizeSqlStatement(*stmt);
                throw runtime_error(strprintf("%s bind failed\n", funcName));
            }

            if (sqlite3_step(*stmt) == SQLITE_ROW)
                if (auto[ok, transaction] = CreateTransactionFromListRow(stmt, true); ok)
                    tx = transaction;

            FinalizeSqlStatement(*stmt);
            return true;
        });

        return make_tuple(tryResult,tx);
    }

    tuple<bool, bool> PocketDb::ConsensusRepository::ExistsUserRegistrations(vector<string>& addresses, int height)
    {
        auto funcName = __func__;
        auto result = false;

        string sql = R"sql(
            SELECT COUNT(DISTINCT(u.AddressHash))
            FROM vUsers u
            WHERE 1 = 1
        )sql";
        sql += " AND u.AddressHash IN ( '";
        sql += addresses[0];
        sql += "'";
        for (size_t i = 1; i < addresses.size(); i++) {
            sql += ",'";
            sql += addresses[i];
            sql += "'";
        }
        sql += ")";

        if (height > 0)
        {
            sql += "AND u.Height < ";
            sql += std::to_string(height);
        }

        bool tryResult = TryTransactionStep([&]() {
            auto stmt = SetupSqlStatement(sql);

            if (sqlite3_step(*stmt) == SQLITE_ROW)
            {
                result = GetColumnInt(*stmt, 0) == (int) addresses.size();
            }

            FinalizeSqlStatement(*stmt);
            return true;
        });

        return make_tuple(tryResult, result);
    }

    tuple<bool, bool, PocketTxType> PocketDb::ConsensusRepository::GetLastBlockingType(string& address, string& addressTo, int height)
    {
        auto funcName = __func__;
        bool blockingExists;
        PocketTxType blockingType;

        auto sql = R"sql(
            SELECT t.Type
            FROM vBlockings u
            WHERE u.AddressHash = ?
                AND u.AddressToHash = ?
                AND u.Height < ?
            ORDER BY u.Height DESC, u.Time DESC
            LIMIT 1
        )sql";

        bool tryResult = TryTransactionStep([&]() {
            auto stmt = SetupSqlStatement(sql);

            auto bindResult = TryBindStatementText(stmt, 1, address);
            bindResult &= TryBindStatementText(stmt, 2, addressTo);
            bindResult &= TryBindStatementInt(stmt, 3, height);
            if (!bindResult) {
                FinalizeSqlStatement(*stmt);
                return false;
            }

            if (sqlite3_step(*stmt) == SQLITE_ROW)
            {
                auto type = GetColumnInt(*stmt, 0);
                blockingExists = true;
                blockingType = static_cast<PocketTxType>(type);
            }
            else
            {
                blockingExists = false;
                blockingType = static_cast<PocketTxType>(0);
            }

            FinalizeSqlStatement(*stmt);
            return true;
        });

        return make_tuple(tryResult, blockingExists, blockingType);
    }
}