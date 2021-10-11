// Copyright (c) 2018-2021 Pocketnet developers
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
        TransactionRepoInst = make_shared<TransactionRepository>(*SQLiteDbInst);
    }

    SQLiteConnection::~SQLiteConnection()
    {
        WebRpcRepoInst->Destroy();
        ExplorerRepoInst->Destroy();
        TransactionRepoInst->Destroy();

        SQLiteDbInst->DetachDatabase("web");
        SQLiteDbInst->Close();
    }


} // namespace PocketDb