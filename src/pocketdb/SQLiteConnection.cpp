// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include "pocketdb/SQLiteConnection.h"

namespace PocketDb
{
    SQLiteConnection::SQLiteConnection()
    {
        LogPrintf("Created SQLiteConnection()\n");

        SQLiteDbInst = new SQLiteDatabase(false, true);
        SQLiteDbInst->Init(
            (GetDataDir() / "pocketdb").string(),
            (GetDataDir() / "pocketdb" / "main.sqlite3").string()
        );

        WebRepoInst = new WebRepository(*SQLiteDbInst);
        WebUserRepoInst = new WebUserRepository(*SQLiteDbInst);
        ExplorerRepoInst = new ExplorerRepository(*SQLiteDbInst);
    }

    SQLiteConnection::~SQLiteConnection()
    {
        SQLiteDbInst->m_connection_mutex.lock();

        WebRepoInst->Destroy();
        WebUserRepoInst->Destroy();
        ExplorerRepoInst->Destroy();

        SQLiteDbInst->m_connection_mutex.unlock();
        SQLiteDbInst->Close();

        delete WebRepoInst;
        delete WebUserRepoInst;
        delete ExplorerRepoInst;

        delete SQLiteDbInst;
    }


} // namespace PocketDb