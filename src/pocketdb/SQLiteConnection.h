// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETDB_SQLITECONNECTION_H
#define POCKETDB_SQLITECONNECTION_H

#include "pocketdb/SQLiteDatabase.h"

#include "pocketdb/repositories/ConsensusRepository.h"
#include "pocketdb/repositories/TransactionRepository.h"
#include "pocketdb/repositories/web/WebRpcRepository.h"
#include "pocketdb/repositories/web/ExplorerRepository.h"
#include "pocketdb/repositories/web/SearchRepository.h"
#include "pocketdb/repositories/web/ModerationRepository.h"
#include "pocketdb/repositories/web/BarteronRepository.h"
#include "pocketdb/repositories/web/NotifierRepository.h"

#include "pocketdb/web/PocketFrontend.h"

namespace PocketDb
{
    using namespace std;
    using namespace PocketWeb;

    class SQLiteConnection
    {
    private:

        SQLiteDatabaseRef SQLiteDbInst;

    public:

        SQLiteConnection(bool timeouted);
        virtual ~SQLiteConnection();

        WebRpcRepositoryRef WebRpcRepoInst;
        ExplorerRepositoryRef ExplorerRepoInst;
        SearchRepositoryRef SearchRepoInst;
        ModerationRepositoryRef ModerationRepoInst;
        BarteronRepositoryRef BarteronRepoInst;
        NotifierRepositoryRef NotifierRepoInst;

        TransactionRepositoryRef TransactionRepoInst;
        ConsensusRepositoryRef ConsensusRepoInst;

    };

} // namespace PocketDb

typedef std::shared_ptr<PocketDb::SQLiteConnection> DbConnectionRef;

#endif // POCKETDB_SQLITECONNECTION_H
