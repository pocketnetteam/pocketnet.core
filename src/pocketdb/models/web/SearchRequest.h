// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETDB_MODEL_SEARCH_REQUEST_H
#define POCKETDB_MODEL_SEARCH_REQUEST_H

#include <vector>
#include <string>
#include "pocketdb/models/base/PocketTypes.h"

namespace PocketDbWeb
{
    using namespace std;
    using namespace PocketTx;

    struct SearchRequest
    {
        string Keyword;
        int TopBlock = 0;
        int PageStart = 0;
        int PageSize = 10;
        string Address;
        bool OrderByRank = false;

        vector<ContentFieldType> FieldTypes;
        vector<TxType> TxTypes;

        SearchRequest() = default;

        SearchRequest(const string& keyword, const vector<ContentFieldType>& fieldTypes, const vector<TxType>& txTypes,
            int topBlock = 0, int pageStart = 0, int pageSize = 10, const string& address = "",
            bool orderByRank = false)
        {
            Keyword = keyword;
            FieldTypes = fieldTypes;
            TxTypes = txTypes;
            TopBlock = topBlock;
            PageStart = pageStart;
            PageSize = pageSize;
            Address = address;
            OrderByRank = orderByRank;
        }
    };

} // PocketDbWeb

#endif //POCKETDB_MODEL_SEARCH_REQUEST_H
