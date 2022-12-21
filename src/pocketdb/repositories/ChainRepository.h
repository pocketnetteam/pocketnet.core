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
        void UpdateTransactionChainData(const int64_t& blockId, int blockNumber, int height, const int64_t& txId, const optional<int64_t>& id, bool fIsCreateLast);
        void UpdateTransactionOutputs(const TransactionIndexingInfo& txInfo, int height);

        pair<int64_t, int64_t> IndexAccount(const int64_t& txId);
        pair<int64_t, int64_t> IndexAccountSetting(const int64_t& txId);
        pair<int64_t, int64_t> IndexContent(const int64_t& txId);
        pair<int64_t, int64_t> IndexComment(const int64_t& txId);
        pair<int64_t, int64_t> IndexBlocking(const int64_t& txId);
        pair<int64_t, int64_t> IndexSubscribe(const int64_t& txId);
        void IndexBoostContent(const int64_t& txId);

        void ClearOldLast(const int64_t& txId);

    };

} // namespace PocketDb

#endif // POCKETDB_CHAINREPOSITORY_H

