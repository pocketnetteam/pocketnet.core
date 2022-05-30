// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETDB_SHORTFORMHELPER_H
#define POCKETDB_SHORTFORMHELPER_H

#include "pocketdb/models/shortform/ShortTxType.h"

#include <string>

namespace PocketHelpers
{
    class ShortTxTypeConvertor
    {
    public:
        static std::string toString(PocketDb::ShortTxType type);
        static PocketDb::ShortTxType strToType(const std::string& typeStr);
    };
}

#endif // POCKETDB_SHORTFORMHELPER_H