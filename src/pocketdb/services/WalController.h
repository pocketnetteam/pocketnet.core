
// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETDB_WAL_CONTROLLER_H
#define POCKETDB_WAL_CONTROLLER_H

#include <boost/thread.hpp>
#include "util/time.h"
#include "sync.h"
#include "util/html.h"

#include "pocketdb/SQLiteDatabase.h"
#include "pocketdb/repositories/web/WebRepository.h"
#include "pocketdb/models/web/WebTag.h"
#include "pocketdb/models/web/WebContent.h"

namespace PocketServices
{
    using namespace PocketDb;
    using namespace PocketDbWeb;

    class WalController
    {
    public:
        WalController();
        void Start(boost::thread_group& threadGroup);
        void Stop();

    private:
        SQLiteDatabaseRef sqliteDbInst;
        WebRepositoryRef webRepoInst;

        bool shutdown = false;

        Mutex _running_mutex;
        Mutex _queue_mutex;
        std::condition_variable _queue_cond;

        void Worker();
    };

} // PocketServices

#endif // POCKETDB_WAL_CONTROLLER_H
