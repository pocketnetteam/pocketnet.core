// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETDB_SQLITECONNECTION_H
#define POCKETDB_SQLITECONNECTION_H

#include "pocketdb/SQLiteDatabase.h"

#include "pocketdb/repositories/web/WebRpcRepository.h"
#include "pocketdb/repositories/web/ExplorerRepository.h"
#include "pocketdb/repositories/web/SearchRepository.h"
#include "pocketdb/repositories/TransactionRepository.h"

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

        SQLiteConnection();
        virtual ~SQLiteConnection();

        WebRpcRepositoryRef WebRpcRepoInst;
        ExplorerRepositoryRef ExplorerRepoInst;
        SearchRepositoryRef SearchRepoInst;
        TransactionRepositoryRef TransactionRepoInst;

    };

} // namespace PocketDb

typedef std::shared_ptr<PocketDb::SQLiteConnection> DbConnectionRef;

#endif // POCKETDB_SQLITECONNECTION_H
