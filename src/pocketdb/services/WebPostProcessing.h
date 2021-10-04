// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETDB_WEB_POST_PROCESSING_H
#define POCKETDB_WEB_POST_PROCESSING_H

#include <boost/thread.hpp>
#include "validation.h"
#include "utiltime.h"

#include "pocketdb/SQLiteDatabase.h"
#include "pocketdb/repositories/web/WebRepository.h"

namespace PocketServices
{
    using namespace PocketDb;

    class WebPostProcessor
    {
    public:
        WebPostProcessor();
        void Start(boost::thread_group& threadGroup, int startHeight);
        void Stop();

    private:
        SQLiteDatabaseRef sqliteDbInst;
        WebRepositoryRef webRepoInst;

        uint32_t sleep = 5 * 1000;
        bool shutdown = false;
        int height;

        void Worker();

        void ProcessTags();
        void ProcessSearchContent();
    };

} // PocketServices

#endif // POCKETDB_WEB_POST_PROCESSING_H