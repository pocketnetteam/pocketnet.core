// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include "pocketdb/services/WebPostProcessing.h"

namespace PocketServices
{
    WebPostProcessor::WebPostProcessor()
    {
        sqliteDbInst = make_shared<SQLiteDatabase>(false);
        sqliteDbInst->Init("main");
        sqliteDbInst->AttachDatabase("web");

        webRepoInst = make_shared<WebRepository>(*sqliteDbInst);
    }

    void WebPostProcessor::Start(boost::thread_group& threadGroup)
    {
        shutdown = false;
        threadGroup.create_thread([this] { Worker(); });
    }

    void WebPostProcessor::Stop()
    {
        LOCK(_queue_mutex);
        shutdown = true;
        _queue_cond.notify_all();
    }

    void WebPostProcessor::Worker()
    {
        while (true)
        {
            string blockHash;

            {
                WAIT_LOCK(_queue_mutex, lock);

                while (!shutdown && _queue_records.empty())
                    _queue_cond.wait(lock);

                if (shutdown) break;

                blockHash = std::move(_queue_records.front());
                _queue_records.pop_front();
            }

            ProcessTags(blockHash);
            ProcessSearchContent(blockHash);
        }
    }

    void WebPostProcessor::Enqueue(const string& blockHash)
    {
        LOCK(_queue_mutex);
        _queue_records.emplace_back(blockHash);
        _queue_cond.notify_one();
    }

    void WebPostProcessor::ProcessTags(const string& blockHash)
    {
        try
        {
            int64_t nTime1 = GetTimeMicros();

            vector<Tag> contentTags = webRepoInst->GetContentTags(blockHash);
            if (contentTags.empty())
                return;

            int64_t nTime2 = GetTimeMicros();
            LogPrint(BCLog::BENCH, "    - WebPostProcessor::ProcessTags (Select): %.2fms\n", 0.001 * (double)(nTime2 - nTime1));

            // Decode contentTags before upsert
            for (auto& contentTag : contentTags)
                contentTag.Value = HtmlUtils::UrlDecode(contentTag.Value);

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
            // TODO (brangr): implement
            // decode
            // place to search tables

            // webRepoInst->ExpandContent(height);
        }
        catch (const std::exception& e)
        {
            LogPrintf("Warning: WebPostProcessor::InsertTags - %s\n", e.what());
        }
    }


} // PocketServices