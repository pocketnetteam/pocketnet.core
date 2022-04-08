// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETTX_BASE_H
#define POCKETTX_BASE_H

#include <memory>
#include "pocketdb/models/base/PocketTypes.h"
#include <univalue/include/univalue.h>

namespace PocketTx
{
    using namespace std;

    class Base
    {
    public:
        Base() = default;
        virtual ~Base() = default;
    protected:
        tuple<bool, string> TryGetStr(const UniValue& o, const string& key);
        tuple<bool, int> TryGetInt(const UniValue& o, const string& key);
        tuple<bool, int64_t> TryGetInt64(const UniValue& o, const string& key);
        tuple<bool, UniValue> TryGetObj(const UniValue& o, const string& key);
    };

}

#endif //POCKETTX_BASE_H
