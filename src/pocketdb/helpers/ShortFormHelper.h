// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETDB_SHORTFORMHELPER_H
#define POCKETDB_SHORTFORMHELPER_H

#include "pocketdb/models/shortform/ShortTxType.h"

#include <string>
#include <functional>

namespace PocketHelpers
{
    class ShortTxTypeConvertor
    {
    public:
        static std::string toString(PocketDb::ShortTxType type);
        static PocketDb::ShortTxType strToType(const std::string& typeStr);
    };

    class ShortTxFilterValidator
    {
    public:
        class Notifications
        {
        public:
            static bool IsFilterAllowed(PocketDb::ShortTxType type);
        };

        class NotificationsSummary
        {
        public:
            static bool IsFilterAllowed(PocketDb::ShortTxType type);
        };

        class Activities
        {
        public:
            static bool IsFilterAllowed(PocketDb::ShortTxType type);
        };
        
        class Events
        {
        public:
            static bool IsFilterAllowed(PocketDb::ShortTxType type);
        };
    };

    // STMT here is used to avoid including here any of sqlite3 headers, however
    // it is expected that STMT is a correct sqlite3_stmt ptr.
    // QueryPrarams can differ between queries.
    template <class STMT, class QueryParams>
    struct ShortFormSqlEntry
    {
        std::string query; // The query that will be a part of big "union" request.
        /**
         * A functor that should bind the query above with given QueryParams
         * @param 1 - sqlite3_stmt ptr
         * @param 2 - current column index
         * @param 3 - query parameters to be binded to stmt
         */
        std::function<void (STMT, int&, QueryParams const&)> binding;
    };
}

#endif // POCKETDB_SHORTFORMHELPER_H