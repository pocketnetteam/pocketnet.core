// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 Bitcoin developers
// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0


#ifndef POCKETTX_RETURN_DTO_HPP
#define POCKETTX_RETURN_DTO_HPP

#include <univalue.h>

namespace PocketTx
{
    using std::string;
    using std::vector;
    using std::shared_ptr;


    class RetBase
    {
    public:
        virtual UniValue Serialize() = 0;
    };

    template<class T>
    class RetList
    {
    protected:
        vector<T> _lst;
    public:
        void Add(T t) { _lst.push_back(t); }    
        UniValue Serialize()
        {
            UniValue ret(UniValue::VARR);
            for (const auto& itm : _lst) {
                ret.push_back(itm->Serialize());
            }

            return ret;
        }
    };

    class RetAddressInfo : RetBase
    {
    public:
        string Address;
        int64_t Balance;

        virtual UniValue Serialize() override
        {
            UniValue ret(UniValue::VOBJ);
            ret.pushKV("address", Address);
            ret.pushKV("balance", Balance);
            return ret;
        }
    };



} // namespace PocketTx

#endif //POCKETTX_RETURN_DTO_HPP