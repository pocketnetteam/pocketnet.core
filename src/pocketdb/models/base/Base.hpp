// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 Bitcoin developers
// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETTX_BASE_HPP
#define POCKETTX_BASE_HPP

namespace PocketTx
{
    using std::string;
    using std::shared_ptr;
    using std::make_shared;

    enum PocketTxType
    {
        NOT_SUPPORTED = 0,

        USER_ACCOUNT = 100,
        VIDEO_SERVER_ACCOUNT = 101,
        MESSAGE_SERVER_ACCOUNT = 102,

        POST_CONTENT = 200,
        VIDEO_CONTENT = 201,
        TRANSLATE_CONTENT = 202,
        SERVERPING_CONTENT = 203,
        COMMENT_CONTENT = 204,

        SCORE_POST_ACTION = 300,
        SCORE_COMMENT_ACTION = 301,

        SUBSCRIBE_ACTION = 302,
        SUBSCRIBE_PRIVATE_ACTION = 303,
        SUBSCRIBE_CANCEL_ACTION = 304,

        BLOCKING_ACTION = 305,
        BLOCKING_CANCEL_ACTION = 306,

        COMPLAIN_ACTION = 307,
    };

    class Base
    {
    public:
    protected:

        static std::tuple<bool, string> TryGetStr(const UniValue &o, const string &key)
        {
            auto exists = o.exists(key) && o[key].isStr() && o[key].get_str().size() > 0;
            if (exists)
                return std::make_tuple(true, o[key].get_str());

            return std::make_tuple(false, "");
        }

        static std::tuple<bool, int> TryGetInt(const UniValue &o, const string &key)
        {
            auto exists = o.exists(key) && o[key].isNum();
            if (exists)
                return std::make_tuple(true, o[key].get_int());

            return std::make_tuple(false, 0);
        }

        static std::tuple<bool, int64_t> TryGetInt64(const UniValue &o, const string &key)
        {
            auto exists = o.exists(key) && o[key].isNum();
            if (exists)
                return std::make_tuple(true, o[key].get_int64());

            return std::make_tuple(false, 0);
        }


    private:
    };

}

#endif //POCKETTX_BASE_HPP
