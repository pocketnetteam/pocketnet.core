// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include "pocketdb/models/base/Base.h"
#include "utilstrencodings.h"

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
        auto exists = o.exists(key);
        if (exists)
        {
            if (o[key].isNum())
                return make_tuple(true, o[key].get_int());
            else
            {
                int val;
                if (ParseInt32(o[key].get_str(), &val))
                    return make_tuple(true, val);
            }
        }

        return make_tuple(false, 0);
    }

    tuple<bool, int64_t> Base::TryGetInt64(const UniValue& o, const string& key)
    {
        auto exists = o.exists(key);
        if (exists)
        {
            if (o[key].isNum())
                return make_tuple(true, o[key].get_int64());
            else
            {
                int64_t val;
                if (ParseInt64(o[key].get_str(), &val))
                    return make_tuple(true, val);
            }
        }

        return make_tuple(false, 0);
    }

    tuple<bool, UniValue> Base::TryGetObj(const UniValue& o, const string& key)
    {
        auto exists = o.exists(key);
        if (exists && o[key].isObject() && !o[key].empty())
            return make_tuple(true, o[key]);

        return make_tuple(false, NullUniValue);
    }
}