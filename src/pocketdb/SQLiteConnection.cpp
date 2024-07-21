// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include "pocketdb/SQLiteConnection.h"

namespace PocketDb
{
    SQLiteConnection::SQLiteConnection(bool timeouted)
    {
        auto dbBasePath = (GetDataDir() / "pocketdb").string();

        SQLiteDbInst = make_shared<SQLiteDatabase>(true);
        SQLiteDbInst->Init(dbBasePath, "main");
        SQLiteDbInst->AttachDatabase("web");

        WebRpcRepoInst = make_shared<WebRpcRepository>(*SQLiteDbInst, true);
        ExplorerRepoInst = make_shared<ExplorerRepository>(*SQLiteDbInst, true);
        SearchRepoInst = make_shared<SearchRepository>(*SQLiteDbInst, true);
        ModerationRepoInst = make_shared<ModerationRepository>(*SQLiteDbInst, true);
        BarteronRepoInst = make_shared<BarteronRepository>(*SQLiteDbInst, true);
        NotifierRepoInst = make_shared<NotifierRepository>(*SQLiteDbInst, true);
        AppRepoInst = make_shared<AppRepository>(*SQLiteDbInst, true);
        TransactionRepoInst = make_shared<TransactionRepository>(*SQLiteDbInst, true);
        ConsensusRepoInst = make_shared<ConsensusRepository>(*SQLiteDbInst, true);
    }

    SQLiteConnection::~SQLiteConnection()
    {
        SQLiteDbInst->m_connection_mutex.lock();

        WebRpcRepoInst->Destroy();
        ExplorerRepoInst->Destroy();
        SearchRepoInst->Destroy();
        ModerationRepoInst->Destroy();
        BarteronRepoInst->Destroy();
        NotifierRepoInst->Destroy();
        AppRepoInst->Destroy();
        TransactionRepoInst->Destroy();
        ConsensusRepoInst->Destroy();

        SQLiteDbInst->DetachDatabase("web");
        SQLiteDbInst->Close();

        SQLiteDbInst->m_connection_mutex.unlock();
    }


} // namespace PocketDb