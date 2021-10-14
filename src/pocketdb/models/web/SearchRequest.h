// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETDB_MODEL_SEARCH_REQUEST_H
#define POCKETDB_MODEL_SEARCH_REQUEST_H

#include <vector>
#include <string>

namespace PocketDbWeb
{
    using namespace std;

    struct SearchRequest
    {
        vector<string> Keywords;
        int StartBlock;
        int PageStart;
        int PageSize;
        string Address;

        SearchRequest() = default;

        SearchRequest(const vector<string>& keywords, int startBlock = 0, int pageStart = 0, int pageSize = 10, const string& address = "")
        {
            Keywords = keywords;
            StartBlock = startBlock;
            PageStart = pageStart;
            PageSize = pageSize;
            Address = address;
        }
    };

} // PocketDbWeb

#endif //POCKETDB_MODEL_SEARCH_REQUEST_H
