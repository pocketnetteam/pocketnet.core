// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETDB_EXPLORERREPOSITORY_H
#define POCKETDB_EXPLORERREPOSITORY_H

#include "pocketdb/helpers/TransactionHelper.hpp"
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

    class ExplorerRepository : public BaseRepository
    {
    public:
        explicit ExplorerRepository(SQLiteDatabase& db) : BaseRepository(db) {}

        void Init() override;
        void Destroy() override;

        map<PocketTxType, map<int, int>> GetStatistic(int bottomHeight, int topHeight);
        map<PocketTxType, int> GetStatistic(int64_t startTime, int64_t endTime);
        tuple<int64_t, int64_t> GetAddressSpent(const string& addressHash);
        UniValue GetAddressTransactions(const string& address, int pageInitBlock, int pageStart, int pageSize);
        UniValue GetBlockTransactions(const string& blockHash, int pageStart, int pageSize);
        UniValue GetTransactions(const vector<string>& transactions, int pageStart, int pageSize);

    private:

        template<typename T>
        UniValue _getTransactions(T stmtOut);
    
    };

} // namespace PocketDb

#endif // POCKETDB_EXPLORERREPOSITORY_H