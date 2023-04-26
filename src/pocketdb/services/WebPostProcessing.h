// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETDB_WEB_POST_PROCESSING_H
#define POCKETDB_WEB_POST_PROCESSING_H

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

    class WebPostProcessor
    {
    public:
        WebPostProcessor();
        void Start(boost::thread_group& threadGroup);
        void Stop();
 
        void ProcessTags(int height);
        void ProcessSearchContent(int height);

    private:
        SQLiteDatabaseRef sqliteDbInst;
        WebRepositoryRef webRepoInst;

        bool shutdown = false;
        Mutex _running_mutex;

        void Worker();
        bool ProcessNextHeight();

    };

} // PocketServices

#endif // POCKETDB_WEB_POST_PROCESSING_H