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

    enum QueueRecordType
    {
        BlockHash = 0,
        BlockHeight = 1,
    };

    struct QueueRecord
    {
        QueueRecordType Type;
        string BlockHash;
        int BlockHeight;
    };

    class WebPostProcessor
    {
    public:
        WebPostProcessor();
        void Start(boost::thread_group& threadGroup);
        void Stop();

        void Enqueue(const string& blockHash);
        void Enqueue(int blockHeight);
                
        void ProcessTags(const string& blockHash);
        void ProcessSearchContent(const string& blockHash);

    private:
        // SQLiteDatabaseRef sqliteDbInst;
        // WebRepositoryRef webRepoInst;

        bool shutdown = false;

        Mutex _running_mutex;
        Mutex _queue_mutex;
        std::condition_variable _queue_cond;
        deque<QueueRecord> _queue_records;

        void Worker();

    };

} // PocketServices

#endif // POCKETDB_WEB_POST_PROCESSING_H