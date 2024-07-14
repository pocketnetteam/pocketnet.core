// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETDB_EXPLORERREPOSITORY_H
#define POCKETDB_EXPLORERREPOSITORY_H

#include "pocketdb/helpers/TransactionHelper.h"
#include "pocketdb/repositories/BaseRepository.h"

#include <boost/algorithm/string/join.hpp>
#include <boost/range/adaptor/transformed.hpp>

namespace PocketDb
{
    using namespace std;
    using namespace PocketTx;
    using namespace PocketHelpers;

    using boost::algorithm::join;
    using boost::adaptors::transformed;

    class ExplorerRepository : public BaseRepository
    {
    public:
        explicit ExplorerRepository(SQLiteDatabase& db, bool timeouted) : BaseRepository(db, timeouted) {}

        map<int, map<int, int>> GetBlocksStatistic(int bottomHeight, int topHeight);
        UniValue GetTransactionsStatistic(int64_t top, int depth, int period);
        UniValue GetTransactionsStatisticByHours(int topHeight, int depth);
        UniValue GetTransactionsStatisticByDays(int topHeight, int depth);
        UniValue GetContentStatisticByHours(int topHeight, int depth);
        UniValue GetContentStatisticByDays(int topHeight, int depth);
        UniValue GetContentStatistic();
        map<string, tuple<int, int64_t>> GetAddressesInfo(const vector<string>& hashes);
        map<string, int> GetAddressTransactions(const string& address, int topHeight, int pageStart, int pageSize, int direction, const vector<TxType>& types);
        map<string, int> GetBlockTransactions(const string& blockHash, int pageStart, int pageSize);
        UniValue GetBalanceHistory(const vector<string>& addresses, int topHeight, int count);
    };

    typedef shared_ptr<ExplorerRepository> ExplorerRepositoryRef;

} // namespace PocketDb

#endif // POCKETDB_EXPLORERREPOSITORY_H