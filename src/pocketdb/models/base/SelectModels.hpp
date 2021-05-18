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
    using std::map;
    using std::shared_ptr;


    // Base return model
    class SelectBase
    {
    public:
        virtual ~SelectBase() {}
        virtual UniValue Serialize() {}
    };

    // Base list ofr return models
    template<class T>
    class SelectList
    {
    protected:
        vector<T> _lst;
    public:
        vector<T>& List() const
        {
            return _lst;
        }
        void Add(T t)
        {
            _lst.push_back(t);
        }
        UniValue Serialize()
        {
            UniValue ret(UniValue::VARR);
            for (auto& itm : _lst)
            {
                ret.push_back(itm.Serialize());
            }

            return ret;
        }
    };

    // Base address info, etc balance
    class SelectAddressInfo : SelectBase
    {
    public:
        string Address;
        int64_t Balance;

        UniValue Serialize() override
        {
            UniValue ret(UniValue::VOBJ);
            ret.pushKV("address", Address);
            ret.pushKV("balance", Balance);
            return ret;
        }
    };

} // namespace PocketTx

#endif //POCKETTX_RETURN_DTO_HPP