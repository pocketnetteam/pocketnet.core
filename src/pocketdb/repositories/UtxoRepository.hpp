// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 Bitcoin developers
// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETDB_UTXOREPOSITORY_HPP
#define POCKETDB_UTXOREPOSITORY_HPP

#include "pocketdb/pocketnet.h"
#include "pocketdb/repositories/BaseRepository.hpp"
#include "pocketdb/models/base/Utxo.hpp"

#include <univalue.h>

namespace PocketDb
{
    using namespace PocketTx;

    class UtxoRepository : public BaseRepository
    {
    public:
        explicit UtxoRepository(SQLiteDatabase &db) : BaseRepository(db) {}

        void Init() override {}

        void Destroy() override {}


        bool Insert(const shared_ptr<Utxo> &utxo)
        {
            if (ShutdownRequested())
                return false;

            auto stmt = SetupSqlStatement(
                " INSERT INTO Utxo ("
                "   TxId,"
                "   Block,"
                "   TxOut,"
                "   TxTime,"
                "   Address,"
                "   BlockSpent,"
                "   Amount"
                " )"
                " SELECT ?,?,?,?,?,?,?"
                " WHERE NOT EXISTS (select 1 from Utxo t where t.TxId = ? and t.TxOut = ?)"
                " ;"
            );

            auto result = TryBindStatementText(stmt, 1, utxo->GetTxId());
            result &= TryBindStatementInt64(stmt, 2, utxo->GetBlock());
            result &= TryBindStatementInt64(stmt, 3, utxo->GetTxOut());
            result &= TryBindStatementInt64(stmt, 4, utxo->GetTxTime());
            result &= TryBindStatementText(stmt, 5, utxo->GetAddress());
            result &= TryBindStatementInt64(stmt, 6, utxo->GetBlockSpent());
            result &= TryBindStatementInt64(stmt, 7, utxo->GetAmount());
            result &= TryBindStatementText(stmt, 8, utxo->GetTxId());
            result &= TryBindStatementInt64(stmt, 9, utxo->GetTxOut());
            if (!result)
                return false;

            if (!TryStepStatement(stmt))
                return false;

            return true;
        }

        bool BulkInsert(const std::vector<shared_ptr<Utxo>> &utxos)
        {
            assert(m_database.m_db);
            if (ShutdownRequested())
                return false;

            if (!m_database.BeginTransaction())
                return false;

            try
            {
                for (const auto &utxo : utxos)
                {
                    if (!Insert(utxo))
                        throw std::runtime_error(strprintf("%s: can't insert in Utxo\n", __func__));
                }

                if (!m_database.CommitTransaction())
                    throw std::runtime_error(strprintf("%s: can't commit insert Utxo\n", __func__));

            } catch (std::exception &ex)
            {
                m_database.AbortTransaction();
                return false;
            }

            return true;
        }


        bool Spent(const shared_ptr<Utxo> &utxo)
        {
            if (ShutdownRequested())
                return false;

            auto stmt = SetupSqlStatement(
                " UPDATE Utxo set"
                "   BlockSpent = ?"
                " WHERE TxId = ?"
                "   AND TxOut = ?"
                " ;"
            );

            auto result = TryBindStatementInt64(stmt, 1, utxo->GetBlockSpent());
            result &= TryBindStatementText(stmt, 2, utxo->GetTxId());
            result &= TryBindStatementInt64(stmt, 3, utxo->GetTxOut());
            if (!result)
                return false;

            if (!TryStepStatement(stmt))
                return false;

            return true;
        }

        bool BulkSpent(const std::vector<shared_ptr<Utxo>> &utxos)
        {
            assert(m_database.m_db);
            if (ShutdownRequested())
                return false;

            if (!m_database.BeginTransaction())
                return false;

            try
            {
                for (const auto &utxo : utxos)
                {
                    if (!Spent(utxo))
                        throw std::runtime_error(strprintf("%s: can't spent Utxo\n", __func__));
                }

                if (!m_database.CommitTransaction())
                    throw std::runtime_error(strprintf("%s: can't commit spent Utxo\n", __func__));

            } catch (std::exception &ex)
            {
                m_database.AbortTransaction();
                return false;
            }

            return true;
        }

        tuple<bool, UniValue> SelectTopAddresses(int count)
        {
            UniValue result(UniValue::VARR);

            assert(m_database.m_db);
            if (ShutdownRequested())
                return make_tuple(false, result);

            try
            {
                auto stmt = SetupSqlStatement(
                    " SELECT"
                    "   u.Address,"
                    "   SUM(u.Amount)Balance"
                    " FROM Utxo u"
                    " WHERE u.BlockSpent is null"
                    " GROUP BY u.Address"
                    " ORDER BY sum(u.Amount) desc"
                    " LIMIT ?"
                    " ;"
                );

                if (!TryBindStatementInt(stmt, 1, count))
                    return make_tuple(false, result);

                while (sqlite3_step(*stmt) == SQLITE_ROW)
                {
                    UniValue utxo(UniValue::VOBJ);
                    utxo.pushKV("address", GetColumnString(*stmt, 0));
                    utxo.pushKV("balance", GetColumnInt64(*stmt, 1));
                    result.push_back(utxo);
                }
            }
            catch (std::exception &ex)
            {
                return make_tuple(false, result);
            }

            return make_tuple(true, result);
        }

    private:

    };

} // namespace PocketDb

#endif // POCKETDB_UTXOREPOSITORY_HPP

