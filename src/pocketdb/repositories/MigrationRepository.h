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
        vector<tuple<int, vector<tuple<string, int>>>> GetAllModTxs();
        void ClearAllJuries();
        int LikersByFlag(const string& txHash, int height);
        int LikersByVote(const string& txHash, int height);
        void IndexModerationJury(const string& flagTxHash, int flagsDepth, int topHeight, int flagsMinCount, int juryModeratorsCount);
        void IndexModerationBan(const string& voteTxHash, int topHeight, int votesCount, int ban1Time, int ban2Time, int ban3Time);
    };

    typedef std::shared_ptr<MigrationRepository> MigrationRepositoryRef;

} // namespace PocketDb

#endif // POCKETDB_MIGRATIONREPOSITORY_H

