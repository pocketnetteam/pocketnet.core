// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include "pocketdb/services/WebPostProcessing.h"
#include "pocketdb/consensus/Reputation.h"

namespace PocketServices
{
    WebPostProcessor::WebPostProcessor() = default;

    void WebPostProcessor::Start(boost::thread_group& threadGroup)
    {
        shutdown = false;
        threadGroup.create_thread([this] { Worker(); });
    }

    void WebPostProcessor::Stop()
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

    void WebPostProcessor::Worker()
    {
        LogPrintf("WebPostProcessor: starting thread worker\n");

        LOCK(_running_mutex);

        // Run database
        auto dbBasePath = (GetDataDir() / "pocketdb").string();

        sqliteDbInst = make_shared<SQLiteDatabase>(false);
        sqliteDbInst->Init(dbBasePath, "main");
        sqliteDbInst->AttachDatabase("web");

        webRepoInst = make_shared<WebRepository>(*sqliteDbInst);

        // Start worker infinity loop
        while (true)
        {
            QueueRecord queueRecord;

            {
                WAIT_LOCK(_queue_mutex, lock);

                while (!shutdown && _queue_records.empty())
                    _queue_cond.wait(lock);

                if (shutdown) break;

                queueRecord = std::move(_queue_records.front());
                _queue_records.pop_front();
            }

            switch (queueRecord.Type)
            {
                case QueueRecordType::BlockHash:
                {
                    ProcessTags(queueRecord.BlockHash);
                    ProcessSearchContent(queueRecord.BlockHash);
                    webRepoInst->UpsertBarteronAccounts(queueRecord.BlockHash);
                    ProcessBarteronOffer(queueRecord.BlockHash);
                    break;
                }
                case QueueRecordType::BlockHeight:
                {
                    // queueRecord.Height % N == 0 -> actions
                }
                default:
                    break;
            }
        }

        // Shutdown DB
        sqliteDbInst->m_connection_mutex.lock();

        webRepoInst->Destroy();
        webRepoInst = nullptr;

        sqliteDbInst->DetachDatabase("web");
        sqliteDbInst->Close();

        sqliteDbInst->m_connection_mutex.unlock();
        sqliteDbInst = nullptr;

        LogPrintf("WebPostProcessor: thread worker exit\n");
    }

    void WebPostProcessor::Enqueue(const string& blockHash)
    {
        QueueRecord rcrd = { QueueRecordType::BlockHash, blockHash, -1 };
        LOCK(_queue_mutex);
        _queue_records.emplace_back(rcrd);
        _queue_cond.notify_one();
    }

    void WebPostProcessor::Enqueue(int blockHeight)
    {
        QueueRecord rcrd = { QueueRecordType::BlockHeight, "", blockHeight };
        LOCK(_queue_mutex);
        _queue_records.emplace_back(rcrd);
        _queue_cond.notify_one();
    }

    void WebPostProcessor::ProcessTags(const string& blockHash)
    {
        try
        {
            int64_t nTime1 = GetTimeMicros();

            vector<WebTag> contentTags = webRepoInst->GetContentTags(blockHash);
            if (contentTags.empty())
                return;

            int64_t nTime2 = GetTimeMicros();
            LogPrint(BCLog::BENCH, "    - WebPostProcessor::ProcessTags (Select): %.2fms\n", 0.001 * (double)(nTime2 - nTime1));

            // Decode contentTags before upsert
            for (auto& contentTag : contentTags)
            {
                contentTag.Value = HtmlUtils::UrlDecode(contentTag.Value);
                HtmlUtils::StringToLower(contentTag.Value);
            }

            int64_t nTime3 = GetTimeMicros();
            LogPrint(BCLog::BENCH, "    - WebPostProcessor::ProcessTags (Prepare): %.2fms\n", 0.001 * (double)(nTime3 - nTime2));

            // Insert content tags
            webRepoInst->UpsertContentTags(contentTags);

            int64_t nTime4 = GetTimeMicros();
            LogPrint(BCLog::BENCH, "    - WebPostProcessor::ProcessTags (Upsert): %.2fms\n", 0.001 * (double)(nTime4 - nTime3));
        }
        catch (const std::exception& e)
        {
            LogPrintf("Warning: WebPostProcessor::ProcessTags - %s\n", e.what());
        }
    }

    void WebPostProcessor::ProcessSearchContent(const string& blockHash)
    {
        try
        {
            int64_t nTime1 = GetTimeMicros();

            vector<WebContent> contentList = webRepoInst->GetContent(blockHash);
            if (contentList.empty())
                return;

            int64_t nTime2 = GetTimeMicros();
            LogPrint(BCLog::BENCH, "    - WebPostProcessor::ProcessSearchContent (Select): %.2fms\n", 0.001 * (double)(nTime2 - nTime1));

            // Decode content before upsert
            for (auto& contentItm : contentList)
            {
                if (contentItm.Value.empty())
                    continue;

                switch (contentItm.FieldType)
                {
                    case ContentFieldType_ContentPostCaption:
                    case ContentFieldType_ContentVideoCaption:
                    case ContentFieldType_ContentPostMessage:
                    case ContentFieldType_ContentVideoMessage:
                    case ContentFieldType_AccountUserAbout:
                    case ContentFieldType_AccountUserName:
                        contentItm.Value = HtmlUtils::UrlDecode(contentItm.Value);
                        break;
                    case ContentFieldType_CommentMessage:
                        // TODO (aok): get message from JSON
                        break;
                    default:
                        break;
                }
                
            }

            int64_t nTime3 = GetTimeMicros();
            LogPrint(BCLog::BENCH, "    - WebPostProcessor::ProcessSearchContent (Prepare): %.2fms\n", 0.001 * (double)(nTime3 - nTime2));

            // Insert content
            webRepoInst->UpsertContent(contentList);

            int64_t nTime4 = GetTimeMicros();
            LogPrint(BCLog::BENCH, "    - WebPostProcessor::ProcessSearchContent (Upsert): %.2fms\n", 0.001 * (double)(nTime4 - nTime3));
        }
        catch (const std::exception& e)
        {
            LogPrintf("Warning: WebPostProcessor::ProcessSearchContent - %s\n", e.what());
        }
    }

    void WebPostProcessor::ProcessBarteronOffer(const string& blockHash)
    {

    }

} // PocketServices