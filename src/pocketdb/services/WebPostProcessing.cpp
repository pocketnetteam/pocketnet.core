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

    void WebPostProcessor::Start(boost::thread_group& threadGroup, int startHeight)
    {
        shutdown = false;
        height = startHeight;
        threadGroup.create_thread([this] { Worker(); });
    }

    void WebPostProcessor::Stop()
    {
        shutdown = true;
    }

    void WebPostProcessor::Worker()
    {
        while (!shutdown)
        {
            if (height <= chainActive.Height())
                MilliSleep(sleep);

            height = chainActive.Height();

            ProcessTags();
            ProcessSearchContent();
        }
    }

    void WebPostProcessor::ProcessTags()
    {
        try
        {
            // TODO (brangr): imeplement in repo
            //webRepoInst->ExpandTags(height);
        }
        catch (const std::exception& e)
        {
            LogPrintf("Warning: WebPostProcessor::ProcessTags at %d - %s\n", height, e.what());
        }
    }

    void WebPostProcessor::ProcessSearchContent()
    {
        try
        {
            // TODO (brangr): get data from payload
            // decode
            // place to search tables

            // webRepoInst->ExpandContent(height);
        }
        catch (const std::exception& e)
        {
            LogPrintf("Warning: WebPostProcessor::ProcessTags at %d - %s\n", height, e.what());
        }
    }

} // PocketServices