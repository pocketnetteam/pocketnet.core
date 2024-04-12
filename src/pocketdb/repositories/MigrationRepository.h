// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETDB_MIGRATIONREPOSITORY_H
#define POCKETDB_MIGRATIONREPOSITORY_H

#include "pocketdb/repositories/BaseRepository.h"

#include <boost/algorithm/string/join.hpp>
#include <boost/range/adaptor/transformed.hpp>

namespace PocketDb
{
    using std::runtime_error;
    using boost::algorithm::join;
    using boost::adaptors::transformed;

    using namespace std;

    class MigrationRepository : public BaseRepository
    {
    public:
        explicit MigrationRepository(SQLiteDatabase& db, bool timeouted) : BaseRepository(db, timeouted) {}

        bool NeedMigrate0_22();
        void Migrate0_21__0_22();
    protected:
        void FulfillRegistry();
        void FulfillChain();
        void FulfillLists();
        void FulfillFirst();
        void FulfillLast();
        void FulfillTransactions();
        void FulfillTxOutputs();
        void FulfillRatings();
        void FulfillBalances();
        void FulfillTxInputs();
        void FulfillPayload();
        void FulfillOthers();

        bool CheckNeedCreateBlockingList();

        bool TableExists(const string& tableName);
        bool ColumnExists(const string& tableName, const string& columnName);
    };

    typedef std::shared_ptr<MigrationRepository> MigrationRepositoryRef;

} // namespace PocketDb

#endif // POCKETDB_MIGRATIONREPOSITORY_H

