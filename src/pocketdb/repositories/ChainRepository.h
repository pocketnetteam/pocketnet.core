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
        void IndexBlockData(const std::string& blockHash);
        void InsertTransactionChainData(const string& blockHash, int blockNumber, int height, const string& txHash, const optional<int64_t>& id, bool fIsCreateLast);
        void UpdateTransactionOutputs(const TransactionIndexingInfo& txInfo, int height);

        pair<optional<int64_t>, optional<int64_t>> IndexAccount(const string& txHash);
        pair<optional<int64_t>, optional<int64_t>> IndexAccountSetting(const string& txHash);
        pair<optional<int64_t>, optional<int64_t>> IndexContent(const string& txHash);
        pair<optional<int64_t>, optional<int64_t>> IndexComment(const string& txHash);
        pair<optional<int64_t>, optional<int64_t>> IndexBlocking(const string& txHash);
        pair<optional<int64_t>, optional<int64_t>> IndexSubscribe(const string& txHash);

        void ClearOldLast(const int64_t& txId);

    };

} // namespace PocketDb

#endif // POCKETDB_CHAINREPOSITORY_H

