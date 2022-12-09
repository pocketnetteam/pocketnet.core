// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 Bitcoin developers
// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETDB_CHAINREPOSITORY_H
#define POCKETDB_CHAINREPOSITORY_H

#include "pocketdb/helpers/TransactionHelper.h"
#include "pocketdb/repositories/BaseRepository.h"
#include "pocketdb/models/base/Rating.h"
#include "pocketdb/models/base/PocketTypes.h"
#include "pocketdb/models/base/DtoModels.h"

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

        // Check block exist in db
        tuple<bool, bool> ExistsBlock(const string& blockHash, int height);

    private:

        void RollbackBlockingList(int height);
        void ClearBlockingList();
        void RollbackHeight(int height);
        void RestoreOldLast(int height);

        // Returns blockId
        int64_t UpdateBlockData(const std::string& blockHash, int height);
        void UpdateTransactionChainData(const int64_t& blockId, int blockNumber, int height, const int64_t& txId);
        void UpdateTransactionOutputs(const TransactionIndexingInfo& txInfo, int height);

        void IndexAccount(const int64_t& txId);
        void IndexAccountSetting(const int64_t& txId);
        void IndexContent(const int64_t& txId);
        void IndexComment(const int64_t& txId);
        void IndexBlocking(const int64_t& txId);
        void IndexSubscribe(const int64_t& txId);
        void IndexBoostContent(const int64_t& txId);

        void ClearOldLast(const int64_t& txId);

    };

} // namespace PocketDb

#endif // POCKETDB_CHAINREPOSITORY_H

