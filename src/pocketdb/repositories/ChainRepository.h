// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 Bitcoin developers
// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETDB_CHAINREPOSITORY_H
#define POCKETDB_CHAINREPOSITORY_H

#include "pocketdb/helpers/TransactionHelper.h"
#include "pocketdb/repositories/BaseRepository.h"
#include "pocketdb/models/base/Rating.h"
#include "pocketdb/models/base/PocketTypes.h"
#include "pocketdb/models/base/ReturnDtoModels.h"

#include <boost/algorithm/string/join.hpp>
#include <boost/range/adaptor/transformed.hpp>

namespace PocketDb
{
    using std::runtime_error;
    using boost::algorithm::join;
    using boost::adaptors::transformed;

    using namespace PocketTx;

    class ChainRepository : public BaseRepository
    {
    public:
        explicit ChainRepository(SQLiteDatabase& db) : BaseRepository(db) {}

        void Init() override {}
        void Destroy() override {}

        // Update transactions set block hash & height
        // Also spent outputs
        void IndexBlock(const string& blockHash, int height, vector<TransactionIndexingInfo>& txs);

        // Precalculate address balances from TxOutputs
        void IndexBalances(int height);

        // Clear all calculated data
        bool ClearDatabase();

        // Erase all calculated data great or equals block
        bool Rollback(int height);

    private:

        void RollbackHeight(int height);
        void RestoreOldLast(int height);

        void UpdateTransactionHeight(const string& blockHash, int blockNumber, int height, const string& txHash);
        void UpdateTransactionOutputs(const TransactionIndexingInfo& txInfo, int height);

        void IndexAccount(const string& txHash, TxType txType);
        void IndexContent(const string& txHash, TxType txType);
        void IndexComment(const string& txHash);
        void IndexBlocking(const string& txHash);
        void IndexSubscribe(const string& txHash);
        void ClearOldLast(const string& txHash);

    };

} // namespace PocketDb

#endif // POCKETDB_CHAINREPOSITORY_H

