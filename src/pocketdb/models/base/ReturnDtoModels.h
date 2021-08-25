// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETTX_RETURN_DTO_H
#define POCKETTX_RETURN_DTO_H

#include <univalue/include/univalue.h>
#include "pocketdb/models/base/PocketTypes.h"

namespace PocketTx
{
    using namespace std;

    // Base return model
    class BaseReturnDto
    {
    public:
        virtual ~BaseReturnDto() = default;
        virtual shared_ptr<UniValue> Serialize()
        {
            UniValue ret(UniValue::VOBJ);
            return make_shared<UniValue>(ret);
        }
    };

    // Base list ofr return models
    template<class T>
    class ListDto
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
        shared_ptr<UniValue> Serialize()
        {
            UniValue ret(UniValue::VARR);
            for (auto& itm : _lst)
            {
                ret.push_back(*itm.Serialize());
            }

            return make_shared<UniValue>(ret);
        }
    };

    // Base address info, etc balance
    class AddressInfoDto : BaseReturnDto
    {
    public:
        string Address;
        int64_t Balance;

        shared_ptr<UniValue> Serialize() override
        {
            UniValue ret(UniValue::VOBJ);
            ret.pushKV("address", Address);
            ret.pushKV("balance", Balance);
            return make_shared<UniValue>(ret);
        }
    };

    // Score data info for indexing
    class ScoreDataDto : BaseReturnDto
    {
    public:
        PocketTxType ScoreType;
        string ScoreTxHash;
        int ScoreAddressId;
        string ScoreAddressHash;
        int64_t ScoreTime;
        int ScoreValue;

        PocketTxType ContentType;
        string ContentTxHash;
        int ContentId;
        int ContentAddressId;
        string ContentAddressHash;
        int64_t ContentTime;

        shared_ptr<UniValue> Serialize() override
        {
            UniValue ret(UniValue::VOBJ);
            ret.pushKV("ScoreType", (int)ScoreType);
            ret.pushKV("ScoreTxHash", ScoreTxHash);
            ret.pushKV("ScoreAddressId", ScoreAddressId);
            ret.pushKV("ScoreAddressHash", ScoreAddressHash);
            ret.pushKV("ScoreTime", ScoreTime);
            ret.pushKV("ScoreValue", ScoreValue);
            ret.pushKV("ContentType", ContentType);
            ret.pushKV("ContentTxHash", ContentTxHash);
            ret.pushKV("ContentId", ContentId);
            ret.pushKV("ContentAddressId", ContentAddressId);
            ret.pushKV("ContentAddressHash", ContentAddressHash);
            ret.pushKV("ContentTime", ContentTime);
            return make_shared<UniValue>(ret);
        }
    };

} // namespace PocketTx

#endif //POCKETTX_RETURN_DTO_H