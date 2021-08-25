// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include "pocketdb/models/base/Base.h"

namespace PocketTx
{
    tuple<bool, string> Base::TryGetStr(const UniValue& o, const string& key)
    {
        if (o.exists(key))
        {
            string val;

            if (o[key].isStr())
                val = o[key].get_str();
            else if (o[key].isArray() || o[key].isObject())
                val = o[key].write();

            if (val != "")
                return make_tuple(true, val);
        }

        return make_tuple(false, "");
    }

    tuple<bool, int> Base::TryGetInt(const UniValue& o, const string& key)
    {
        auto exists = o.exists(key) && o[key].isNum();
        if (exists)
            return make_tuple(true, o[key].get_int());

        return make_tuple(false, 0);
    }

    tuple<bool, int64_t> Base::TryGetInt64(const UniValue& o, const string& key)
    {
        auto exists = o.exists(key) && o[key].isNum();
        if (exists)
            return make_tuple(true, o[key].get_int64());

        return make_tuple(false, 0);
    }
}