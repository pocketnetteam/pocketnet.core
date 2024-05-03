
// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include "pocketdb/services/WalController.h"
#include <fs.h>

namespace {
    size_t GetFileSize(std::string path)
    {
        return fs::file_size(path);
    }
    
    const static size_t MAX_WAL_BYTES_SIZE = 100242880;


    bool TruncateWAL(PocketDb::SQLiteDatabase& connection)
    {
        auto ret = sqlite3_wal_checkpoint_v2(connection.m_db, nullptr, SQLITE_CHECKPOINT_TRUNCATE, nullptr, nullptr);
        return ret == SQLITE_OK;
    }
}

namespace PocketServices
{
    WalController::WalController() = default;

    void WalController::Start(boost::thread_group& threadGroup)
    {
        shutdown = false;
        threadGroup.create_thread([this] { Worker(); });
    }

    void WalController::Stop()
    {
        // Signal for complete all tasks
        {
            LOCK(_queue_mutex);

            shutdown = true;
            _queue_cond.notify_all();
        }

        // Wait all tasks completed
        LOCK(_running_mutex);
    }

    void WalController::Worker()
    {
        LogPrintf("WalController: starting thread worker\n");

        LOCK(_running_mutex);

        // Run database
        auto dbBasePath = (GetDataDir() / "pocketdb").string();

        sqliteDbInst = make_shared<SQLiteDatabase>(false);
        sqliteDbInst->Init(dbBasePath, "main");
        sqliteDbInst->AttachDatabase("web");

        webRepoInst = make_shared<WebRepository>(*sqliteDbInst, false);

        // Start worker infinity loop
        while (true)
        { 
            if (shutdown) break;
            auto size = GetFileSize((GetDataDir()/"pocketdb/main.sqlite3-wal").string());
            if (size > MAX_WAL_BYTES_SIZE) {
                TruncateWAL(*sqliteDbInst);
            }
            UninterruptibleSleep(std::chrono::seconds(1));
        }

        // Shutdown DB
        sqliteDbInst->m_connection_mutex.lock();

        webRepoInst->Destroy();
        webRepoInst = nullptr;

        sqliteDbInst->DetachDatabase("web");
        sqliteDbInst->Close();

        sqliteDbInst->m_connection_mutex.unlock();
        sqliteDbInst = nullptr;

        LogPrintf("WalController: thread worker exit\n");
    }
} // PocketServices
