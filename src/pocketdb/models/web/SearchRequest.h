// Copyright (c) 2018-2021 Pocketnet developers
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
        int TopBlock;
        int PageStart;
        int PageSize;
        string Address;

        vector<ContentFieldType> FieldTypes;
        vector<TxType> TxTypes;

        SearchRequest() = default;

        SearchRequest(const string& keyword, const vector<ContentFieldType>& fieldTypes, const vector<TxType>& txTypes,
            int topBlock = 0, int pageStart = 0, int pageSize = 10, const string& address = "")
        {
            Keyword = keyword;
            FieldTypes = move(fieldTypes);
            TxTypes = move(txTypes);
            TopBlock = topBlock;
            PageStart = pageStart;
            PageSize = pageSize;
            Address = address;
        }
    };

} // PocketDbWeb

#endif //POCKETDB_MODEL_SEARCH_REQUEST_H
