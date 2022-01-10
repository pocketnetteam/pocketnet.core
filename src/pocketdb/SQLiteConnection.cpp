// Copyright (c) 2018-2022 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include "pocketdb/SQLiteConnection.h"

namespace PocketDb
{
    SQLiteConnection::SQLiteConnection()
    {
        auto dbBasePath = (GetDataDir() / "pocketdb").string();

        SQLiteDbInst = make_shared<SQLiteDatabase>(true);
        SQLiteDbInst->Init(dbBasePath, "main");
        SQLiteDbInst->AttachDatabase("web");

        WebRpcRepoInst = make_shared<WebRpcRepository>(*SQLiteDbInst);
        ExplorerRepoInst = make_shared<ExplorerRepository>(*SQLiteDbInst);
        SearchRepoInst = make_shared<SearchRepository>(*SQLiteDbInst);
        TransactionRepoInst = make_shared<TransactionRepository>(*SQLiteDbInst);
    }

    SQLiteConnection::~SQLiteConnection()
    {
        SQLiteDbInst->m_connection_mutex.lock();

        WebRpcRepoInst->Destroy();
        ExplorerRepoInst->Destroy();
        SearchRepoInst->Destroy();
        TransactionRepoInst->Destroy();

        SQLiteDbInst->DetachDatabase("web");
        SQLiteDbInst->Close();

        SQLiteDbInst->m_connection_mutex.unlock();
    }


} // namespace PocketDb