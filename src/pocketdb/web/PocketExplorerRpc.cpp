// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include "PocketExplorerRpc.h"

using namespace PocketWeb;
using namespace PocketDb;


UniValue PocketExplorerRpc::GetStatistic(const JSONRPCRequest& request)
{
    if (request.fHelp)
        throw std::runtime_error(
            "getstatistic (endTime, depth)\n"
            "\nGet statistics.\n"
            "\nArguments:\n"
            "1. \"endTime\"   (int64, optional) End time of period\n"
            "2. \"depth\"     (int32, optional) Day = 1, Month = 2, Year = 3\n");

    int64_t end_time = (int64_t)chainActive.Tip()->nTime;
    if (request.params.size() > 0 && request.params[0].isNum())
        end_time = request.params[0].get_int64();

    StatisticDepth depth = StatisticDepth_Month;
    if (request.params.size() > 1 && request.params[1].isNum() && request.params[1].get_int() >= 1 && request.params[1].get_int() <= 3)
        depth = (StatisticDepth)request.params[1].get_int();

    int64_t start_time;
    switch (depth)
    {
        case StatisticDepth_Day:
            start_time = end_time - (60*60*24);
            break;
        default:
        case StatisticDepth_Month:
            start_time = end_time - (60*60*24*30);
            break;
        case StatisticDepth_Year:
            start_time = end_time - (60*60*24*365);
            break;
    }

    return request.DbConnection()->ExplorerRepoInst->GetStatistic(start_time, end_time, depth);
}