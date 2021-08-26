// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETDB_EXPLORERREPOSITORY_H
#define POCKETDB_EXPLORERREPOSITORY_H

#include "pocketdb/helpers/TransactionHelper.h"
#include "pocketdb/repositories/BaseRepository.hpp"

#include <boost/algorithm/string/join.hpp>
#include <boost/range/adaptor/transformed.hpp>

namespace PocketDb
{
    using namespace std;
    using namespace PocketTx;
    using namespace PocketHelpers;

    using boost::algorithm::join;
    using boost::adaptors::transformed;

    enum StatisticDepth
    {
        StatisticDepth_Day = 1,
        StatisticDepth_Month = 2,
        StatisticDepth_Year = 3
    };

    class ExplorerRepository : public BaseRepository
    {
    public:
        explicit ExplorerRepository(SQLiteDatabase& db) : BaseRepository(db) {}

        void Init() override;
        void Destroy() override;

        map<int, map<int, int>> GetStatistic(int bottomHeight, int topHeight);
        UniValue GetStatistic(int64_t startTime, int64_t endTime, StatisticDepth depth);
        tuple<int64_t, int64_t> GetAddressSpent(const string& addressHash);
        UniValue GetAddressTransactions(const string& address, int pageInitBlock, int pageStart, int pageSize);
        UniValue GetBlockTransactions(const string& blockHash, int pageStart, int pageSize);
        UniValue GetTransactions(const vector<string>& transactions, int pageStart, int pageSize);

    private:

        template<typename T>
        UniValue _getTransactions(T stmtOut);
    
    };

    typedef std::shared_ptr<ExplorerRepository> ExplorerRepositoryRef;

} // namespace PocketDb

#endif // POCKETDB_EXPLORERREPOSITORY_H