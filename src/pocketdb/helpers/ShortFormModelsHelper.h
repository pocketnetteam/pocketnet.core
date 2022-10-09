// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETDB_SHORTFORMMODELSHELPER_H
#define POCKETDB_SHORTFORMMODELSHELPER_H

#include "pocketdb/models/shortform/ShortForm.h"

#include <string>

namespace PocketHelpers
{
    using namespace PocketDb;

    class ShortTxTypeConvertor
    {
    public:
        static std::string toString(ShortTxType type);
        static ShortTxType strToType(const std::string& typeStr);
    };

    class ShortTxFilterValidator
    {
    public:
        class Notifications
        {
        public:
            static bool IsFilterAllowed(ShortTxType type);
        };

        class NotificationsSummary
        {
        public:
            static bool IsFilterAllowed(ShortTxType type);
        };

        class Activities
        {
        public:
            static bool IsFilterAllowed(ShortTxType type);
        };
        
        class Events
        {
        public:
            static bool IsFilterAllowed(ShortTxType type);
        };
    };
}

#endif // POCKETDB_SHORTFORMMODELSHELPER_H
