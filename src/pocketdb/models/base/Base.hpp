// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 Bitcoin developers
// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETTX_BASE_HPP
#define POCKETTX_BASE_HPP

#include "pocketdb/models/base/PocketTypes.hpp"
#include <univalue/include/univalue.h>

namespace PocketTx
{
    using std::string;
    using std::shared_ptr;
    using std::make_shared;
    using std::tuple;
    using std::make_tuple;
    using std::vector;

    class Base
    {
    public:

        Base() = default;
        virtual ~Base() {}

    protected:

        static tuple<bool, string> TryGetStr(const UniValue& o, const string& key)
        {
            if (o.exists(key) && o[key].isStr())
            {
                auto val = o[key].get_str();
                if (val != "")
                    return make_tuple(true, val);
            }

            return make_tuple(false, "");
        }

        static tuple<bool, int> TryGetInt(const UniValue& o, const string& key)
        {
            auto exists = o.exists(key) && o[key].isNum();
            if (exists)
                return make_tuple(true, o[key].get_int());

            return make_tuple(false, 0);
        }

        static tuple<bool, int64_t> TryGetInt64(const UniValue& o, const string& key)
        {
            auto exists = o.exists(key) && o[key].isNum();
            if (exists)
                return make_tuple(true, o[key].get_int64());

            return make_tuple(false, 0);
        }


    private:
    };

}

#endif //POCKETTX_BASE_HPP
