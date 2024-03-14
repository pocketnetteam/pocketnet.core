// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include "pocketdb/services/WebPostProcessing.h"
#include "pocketdb/consensus/Reputation.h"
#include "init.h"

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
        shutdown = true;

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

        webRepoInst = make_shared<WebRepository>(*sqliteDbInst, false);

        // Start worker infinity loop
        while (true)
        {
            if (shutdown)
                break;

            if (!ProcessNextHeight())
                UninterruptibleSleep(std::chrono::milliseconds{10000});
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

    bool WebPostProcessor::ProcessNextHeight()
    {
        int currHeight = webRepoInst->GetCurrentHeight();
        gStatEngineInstance.HeightWeb = currHeight;

        // Return 'false' for non found work
        if (ChainActive().Height() - currHeight <= 0)
            return false;

        // Proccess next block height
        currHeight += 1;

        // Launch all needed processes
        ProcessTags(currHeight);
        ProcessSearchContent(currHeight);
        
        webRepoInst->UpsertBarteronAccounts(currHeight);
        webRepoInst->UpsertBarteronOffers(currHeight);

        int period = 60;
        if (gArgs.GetChainName() == CBaseChainParams::REGTEST)
            period = 1;
            
        if (currHeight % period == 0 && pindexBestHeader && (pindexBestHeader->nHeight - currHeight) < period)
            webRepoInst->CollectAccountStatistic();

        webRepoInst->SetCurrentHeight(currHeight);
        return true;
    }


    void WebPostProcessor::ProcessTags(int height)
    {
        try
        {
            int64_t nTime1 = GetTimeMicros();

            vector<WebTag> contentTags = webRepoInst->GetContentTags(height);
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

    void WebPostProcessor::ProcessSearchContent(int height)
    {
        try
        {
            int64_t nTime1 = GetTimeMicros();

            vector<WebContent> contentList = webRepoInst->GetContent(height);
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

} // PocketServices