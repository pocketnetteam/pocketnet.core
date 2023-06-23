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

        // Update transactions set block hash & height
        // Also spent outputs
        void IndexBlock(const string& blockHash, int height, vector<TransactionIndexingInfo>& txs);

        // Precalculate address balances from TxOutputs
        void IndexBalances(int height);

        void RestoreLast(int height);
        void RestoreRatings(int height);
        void RestoreBalances(int height);
        void RestoreChain(int height);
        void RestoreSocialRegistry(int height);

        // Clear all calculated data
        bool ClearDatabase();

        // Erase all calculated data great or equals block
        void RollbackBlockingList(int height);

        void IndexModerationJury(const string& flagTxHash, int flagsDepth, int flagsMinCount, int juryModeratorsCount);
        void RestoreModerationJury(int height);

        void IndexModerationBan(const string& voteTxHash, int votesCount, int ban1Time, int ban2Time, int ban3Time);
        void RestoreModerationBan(int height);

        void IndexBadges(int height, const BadgeConditions& conditions);
        void RestoreBadges(int height);
        
        // Check block exist in db
        tuple<bool, bool> ExistsBlock(const string& blockHash, int height);

    private:

        // void RollbackBlockingList(int height);
        // void ClearBlockingList();

        void SetFirst(const string& txHash);

        // Returns blockId
        void IndexBlockData(const std::string& blockHash);
        void InsertTransactionChainData(const string& blockHash, int blockNumber, int height, const string& txHash, const optional<int64_t>& id);

        void IndexSocialRegistry(const TransactionIndexingInfo& txInfo, int height, bool isFirst);
        void DeleteExpiredSocialRegistry(int height);

        pair<optional<int64_t>, optional<int64_t>> IndexSocial(const TransactionIndexingInfo& txInfo);
        string IndexAccount();
        string IndexAccountSetting();
        string IndexContent();
        string IndexComment();
        string IndexBlocking();
        string IndexSubscribe();
        string IndexAccountBarteron();

    };

} // namespace PocketDb

#endif // POCKETDB_CHAINREPOSITORY_H

