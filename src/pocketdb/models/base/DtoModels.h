// Copyright (c) 2018-2022 The Pocketnet developers
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
        TxType ScoreType;
        string ScoreTxHash;
        int ScoreAddressId;
        string ScoreAddressHash;
        int64_t ScoreTime;
        int ScoreValue;

        TxType ContentType;
        string ContentTxHash;
        int ContentId;
        int ContentAddressId;
        string ContentAddressHash;
        int64_t ContentTime;

        string CommentAnswerRootTxHash;

        int64_t ScoresAllCount;
        int64_t ScoresPositiveCount;

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
            ret.pushKV("ScoresAllCount", ScoresAllCount);
            ret.pushKV("ScoresPositiveCount", ScoresPositiveCount);
            return make_shared<UniValue>(ret);
        }

        RatingType LikerType(bool last)
        {
            if (ScoreType == ACTION_SCORE_CONTENT)
                return last ? ACCOUNT_LIKERS_POST_LAST : ACCOUNT_LIKERS_POST;

            if (ScoreType == ACTION_SCORE_COMMENT && CommentAnswerRootTxHash.empty())
                return last ? ACCOUNT_LIKERS_COMMENT_ROOT_LAST : ACCOUNT_LIKERS_COMMENT_ROOT;

            if (ScoreType == ACTION_SCORE_COMMENT && !CommentAnswerRootTxHash.empty())
                return last ? ACCOUNT_LIKERS_COMMENT_ANSWER_LAST : ACCOUNT_LIKERS_COMMENT_ANSWER;

            // You don't have to get to here
            return ACCOUNT_LIKERS;
        }
    };

    typedef shared_ptr<ScoreDataDto> ScoreDataDtoRef;

    class ModerationVoteTxData : BaseReturnDto
    {
    public:
        string AddressHash;
    };

    using ModerationVoteTxDataRef = std::shared_ptr<ModerationVoteTxData>;

} // namespace PocketTx

#endif //POCKETTX_RETURN_DTO_H