// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef SRC_POCKETEXPLORERRPC_H
#define SRC_POCKETEXPLORERRPC_H


namespace PocketWeb
{
    using std::string;
    using std::vector;
    using std::map;

    class PocketExplorerRpc
    {
    public:
        UniValue GetStatistic(const JSONRPCRequest& request);
    };
}


#endif //SRC_POCKETEXPLORERRPC_H
